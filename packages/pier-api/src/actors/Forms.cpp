/** actors/Forms.cpp: forms with an asynchronous result callback.
 * The only entry point whose callback fires after the calling frame returns. The lifetime
 * discipline matches commands: an ll::form callback captures a weak_ptr<HostedMod> plus a ticket
 * into a host-side pending table, and when the player answers, a mod that has been unloaded or
 * disabled sees the callback dropped silently. Unload clears every pending ticket. Result encoding
 * is resolved at this layer. CustomFormElementResult is variant<monostate, uint64, double,
 * string>, and depending on the LL version a dropdown or step_slider returns either the selected
 * index or the text. Reading it as an index alone falls back to 0 on a string, which shows up as
 * always receiving the first item no matter what was selected. The options of every choice control
 * are recorded when the form is built, an integer coming back is taken as an index, a string is
 * looked up back to an index, first exactly and then with the §x color codes removed, and both the
 * index and the text are always emitted. The parameter shapes that make a client fail to render
 * the whole form are clamped here as well: a dropdown with empty options, an out-of-range default
 * index, and a slider default outside [min,max] or off the step grid. PIER_TRACE_FORM=1 prints the
 * element list and, on the way back, the variant kind and raw value of every key. / */
#ifndef PIER_BUILD_CLIENT

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "ll/api/form/CustomForm.h"
#include "ll/api/form/FormBase.h"
#include "ll/api/form/ModalForm.h"
#include "ll/api/form/SimpleForm.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/world/actor/player/Player.h"

#include "sdk/abi.h"

#include "pier/api/bridge.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/spi.h"
#include "pier/support/guard.h"
#include "pier/support/log.h"
#include "pier/support/snbt.h"
#include "pier/support/str.h"

namespace pier::api_impl
{
    namespace
    {
        struct PendingForm
        {
            HostedMod* mod = nullptr; // Identity comparison only, never dereferenced
            PierFormResultCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gFormMutex;
        std::unordered_map<uint64_t, PendingForm> gPendingForms;
        uint64_t gNextTicket = 1;

        /** Ledger of name to options for the choice controls, dropdown and
         *  step_slider. */
        using ChoiceTable = std::unordered_map<std::string, std::vector<std::string>>;

        /** Ledger of name to {min,max,step} for sliders. The returned value comes from
         *  a client and Slider::parseResult in LL is a bare get<double>() with no range
         *  check, so a host that normalized the spec on the way out must clamp to the
         *  same spec on the way back. */
        struct SliderSpec
        {
            double min, max, step;
        };
        using SliderTable = std::unordered_map<std::string, SliderSpec>;

        /** Clamps a returned value to the spec: out of range snaps to the bound, and a
         *  step grid snaps to the nearest step. Returns whether it changed, which means
         *  the client sent a value the form could not produce. */
        bool clampSliderValue(SliderSpec const& spec, double& v)
        {
            double const before = v;
            if (!(v == v)) v = spec.min; // NaN
            v = std::clamp(v, spec.min, spec.max);
            if (spec.step > 0.0)
            {
                double k = std::round((v - spec.min) / spec.step);
                v = std::clamp(spec.min + k * spec.step, spec.min, spec.max);
            }
            return std::abs(before - v) > 1e-9 || !(before == before);
        }

        /** PIER_TRACE_FORM=1 turns on form tracing. Read once. */
        bool formTrace()
        {
            static bool const on = []
            {
                auto const* v = std::getenv("PIER_TRACE_FORM");
                return v && v[0] && v[0] != '0';
            }();
            return on;
        }

        uint64_t registerTicket(HostedMod* mod, PierFormResultCb cb, void* user)
        {
            std::lock_guard lock(gFormMutex);
            uint64_t ticket = gNextTicket++;
            gPendingForms[ticket] = PendingForm{mod, cb, user};
            return ticket;
        }

        /**
         * Takes the ticket out of the table and fires the callback exactly once, unless
         * its mod has been unloaded, in which case the ticket is already cleared, or
         * disabled, in which case it is muted. Runs on the server thread, which
         * ll::form guarantees.
         */
        void completeTicket(std::weak_ptr<HostedMod> weakMod, uint64_t ticket, std::string const& resultSnbt)
        {
            if (formTrace()) hostLogger().info("[form] ticket={} -> {}", ticket, resultSnbt);
            PendingForm pending;
            {
                std::lock_guard lock(gFormMutex);
                auto it = gPendingForms.find(ticket);
                if (it == gPendingForms.end()) return; // Cleared at unload
                pending = it->second;
                gPendingForms.erase(it);
            }
            auto mod = weakMod.lock();
            if (!mod || mod.get() != pending.mod) return; // Mod is gone
            if (!mod->acceptsCallbacks()) return;                // Muted while disabled
            CallbackScope scope{mod.get()};               // Veto unload during callback
            if (pending.cb) pending.cb(pending.user, ps(resultSnbt));
        }

        std::string cancelledSnbt(ll::form::FormCancelReason reason)
        {
            int code = reason.has_value() ? static_cast<int>(*reason) : -1;
            return "{cancelled:1b,reason:" + snbtNum(code) + "}";
        }

        /** Reads a string field, with a default. */
        std::string strField(CompoundTag const& o, char const* key, std::string def = {})
        {
            if (o.contains(key) && o.at(key).is_string())
                return std::string{std::string_view{o.at(key)}};
            return def;
        }

        double numField(CompoundTag const& o, char const* key, double def)
        {
            if (o.contains(key)) return bridge::nbtToDouble(o.at(key), def);
            return def;
        }

        //  Index and text lookup for choice controls

        /** Strips the Minecraft §x color codes, for the second, lenient pass. */
        std::string stripFormatCodes(std::string_view s)
        {
            std::string out;
            out.reserve(s.size());
            for (size_t i = 0; i < s.size();)
            {
                // § is 0xC2 0xA7 in UTF-8
                if (i + 2 < s.size() && static_cast<unsigned char>(s[i]) == 0xC2
                    && static_cast<unsigned char>(s[i + 1]) == 0xA7)
                {
                    i += 3; // Skip § and the code byte itself
                    continue;
                }
                out.push_back(s[i]);
                ++i;
            }
            return out;
        }

        std::optional<size_t> findOption(std::vector<std::string> const& opts, std::string const& text)
        {
            for (size_t i = 0; i < opts.size(); ++i)
                if (opts[i] == text) return i;
            // A client may swallow the color codes before returning, so compare once
            // more leniently
            std::string bare = stripFormatCodes(text);
            for (size_t i = 0; i < opts.size(); ++i)
                if (stripFormatCodes(opts[i]) == bare) return i;
            return std::nullopt;
        }

        //  SNBT assembly

        struct SnbtObject
        {
            std::string body;

            void put(std::string const& key, std::string const& rawValue)
            {
                if (!body.empty()) body += ',';
                body += '"';
                body += snbtEscape(key);
                body += "\":";
                body += rawValue;
            }

            void putString(std::string const& key, std::string const& text)
            {
                put(key, "\"" + snbtEscape(text) + "\"");
            }

            std::string wrap() const { return "{" + body + "}"; }
        };

        /** The variant kind name, for tracing only. */
        char const* variantKind(ll::form::CustomFormElementResult const& v)
        {
            if (std::holds_alternative<uint64_t>(v)) return "uint64";
            if (std::holds_alternative<double>(v)) return "double";
            if (std::holds_alternative<std::string>(v)) return "string";
            return "monostate";
        }

        std::string variantText(ll::form::CustomFormElementResult const& v)
        {
            if (std::holds_alternative<uint64_t>(v)) return snbtNum(std::get<uint64_t>(v));
            if (std::holds_alternative<double>(v)) return snbtNum(std::get<double>(v));
            if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
            return "<none>";
        }

        //  SimpleForm

        bool sendSimple(
            Player& p, CompoundTag const& spec, std::weak_ptr<HostedMod> weakMod, uint64_t ticket)
        {
            auto form =
                std::make_shared<ll::form::SimpleForm>(strField(spec, "title"), strField(spec, "content"));
            int buttons = 0;
            if (spec.contains("elements") && spec.at("elements").is_array())
            {
                for (auto const& ePtr : spec.at("elements").get<ListTag>())
                {
                    if (!ePtr || ePtr->getId() != Tag::Type::Compound) continue;
                    auto const& e = static_cast<CompoundTag const&>(*ePtr);
                    std::string kind = strField(e, "kind");
                    if (kind == "button")
                    {
                        std::string image = strField(e, "image");
                        if (image.empty())
                        {
                            form->appendButton(strField(e, "text"));
                        }
                        else
                        {
                            form->appendButton(
                                strField(e, "text"), image, strField(e, "image_type", "path"));
                        }
                        ++buttons;
                    }
                    else if (kind == "header")
                    {
                        form->appendHeader(strField(e, "text"));
                    }
                    else if (kind == "label")
                    {
                        form->appendLabel(strField(e, "text"));
                    }
                    else if (kind == "divider")
                    {
                        form->appendDivider();
                    }
                }
            }
            if (buttons == 0)
            {
                // A SimpleForm without buttons cannot be clicked and can only be
                // closed. The caller most likely assembled an empty list, which is a
                // logic error worth one line.
                hostLogger().warn("[form] SimpleForm \"{}\" has no buttons", strField(spec, "title"));
            }
            if (formTrace())
            {
                hostLogger().info("[form] simple ticket={} with {} button(s)", ticket, buttons);
            }
            form->sendTo(p, [form, weakMod, ticket, buttons](Player& who, int button, ll::form::FormCancelReason reason)
            {
                if (button < 0)
                {
                    completeTicket(weakMod, ticket, cancelledSnbt(reason));
                }
                else if (button >= buttons)
                {
                    // LL hands back the index the client returned without a range
                    // check. A modified client can answer 999 to a three-button form,
                    // and passing that to a mod is an out-of-range index, meaning a
                    // Rust panic and abort or an out-of-bounds read in C. It is
                    // treated as a cancel and the player's name is recorded.
                    hostLogger().warn(
                        "[form] player {} returned button index {} out of range of {}, treated as a cancel",
                        who.getRealName(), button, buttons);
                    completeTicket(weakMod, ticket, "{cancelled:1b,reason:-2,invalid:1b}");
                }
                else
                {
                    completeTicket(weakMod, ticket, "{button:" + snbtNum(button) + "}");
                }
            });
            return true;
        }

        //  CustomForm

        /**
         * Normalizes slider parameters into a shape the client accepts.
         *
         * The Bedrock client tolerates very little here. A default outside the range,
         * or a (max-min) that is not a whole multiple of step, can make the entire form
         * fail to render, which the player experiences as the form vanishing on open.
         * The values come from configuration a human can edit to anything, so the clamp
         * belongs in the bridge.
         */
        void normalizeSlider(double& mn, double& mx, double& step, double& def, std::string const& name)
        {
            if (!(mn <= mx)) std::swap(mn, mx); // Catches NaN as well
            if (!(step > 0.0)) step = 0.0;

            if (step > 0.0)
            {
                double span = mx - mn;
                if (span < step)
                {
                    // A step larger than the whole range degenerates to a single
                    // notch, and the client computes an empty scale
                    step = span > 0.0 ? span : 0.0;
                }
                else
                {
                    // Pull max down to the last value that lands on the step grid
                    double steps = std::floor(span / step + 1e-9);
                    mx = mn + steps * step;
                }
            }

            double before = def;
            def = std::clamp(def, mn, mx);
            if (step > 0.0)
            {
                double k = std::round((def - mn) / step);
                def = std::clamp(mn + k * step, mn, mx);
            }
            if (std::abs(before - def) > 1e-9)
            {
                hostLogger().warn(
                    "[form] slider \"{}\" default {} is not on the step grid of [{}, {}], adjusted to {}",
                    name, before, mn, mx, def);
            }
        }

        bool sendCustom(
            Player& p, CompoundTag const& spec, std::weak_ptr<HostedMod> weakMod, uint64_t ticket)
        {
            auto form = std::make_shared<ll::form::CustomForm>(strField(spec, "title"));
            auto choices = std::make_shared<ChoiceTable>();
            auto sliders = std::make_shared<SliderTable>();
            std::unordered_set<std::string> seenNames;

            if (spec.contains("submit")) form->setSubmitButton(strField(spec, "submit"));
            if (spec.contains("elements") && spec.at("elements").is_array())
            {
                for (auto const& ePtr : spec.at("elements").get<ListTag>())
                {
                    if (!ePtr || ePtr->getId() != Tag::Type::Compound) continue;
                    auto const& e = static_cast<CompoundTag const&>(*ePtr);
                    std::string kind = strField(e, "kind");
                    std::string name = strField(e, "name");

                    // A control carrying a value needs a unique name. The result is an
                    // unordered_map keyed by name, so duplicates overwrite each other
                    // and the caller receives one of them.
                    bool valued = (kind == "input" || kind == "toggle" || kind == "dropdown"
                        || kind == "step_slider" || kind == "slider");
                    if (valued)
                    {
                        if (name.empty())
                        {
                            hostLogger().warn("[form] a {} control has no name and cannot be read from the result", kind);
                        }
                        else if (!seenNames.insert(name).second)
                        {
                            hostLogger().warn("[form] name \"{}\" is duplicated, the later control overwrites the earlier", name);
                        }
                    }

                    if (kind == "header")
                    {
                        form->appendHeader(strField(e, "text"));
                    }
                    else if (kind == "label")
                    {
                        form->appendLabel(strField(e, "text"));
                    }
                    else if (kind == "divider")
                    {
                        form->appendDivider();
                    }
                    else if (kind == "input")
                    {
                        form->appendInput(
                            name,
                            strField(e, "text"),
                            strField(e, "placeholder"),
                            strField(e, "default"),
                            strField(e, "tooltip")
                        );
                    }
                    else if (kind == "toggle")
                    {
                        form->appendToggle(
                            name, strField(e, "text"), numField(e, "default", 0.0) != 0.0,
                            strField(e, "tooltip"));
                    }
                    else if (kind == "dropdown" || kind == "step_slider")
                    {
                        std::vector<std::string> options;
                        if (e.contains("options") && e.at("options").is_array())
                        {
                            for (auto const& oPtr : e.at("options").get<ListTag>())
                            {
                                if (!oPtr || oPtr->getId() != Tag::Type::String) continue;
                                options.emplace_back(
                                    static_cast<std::string const&>(static_cast<StringTag const&>(*oPtr)));
                            }
                        }
                        if (options.empty())
                        {
                            // An empty dropdown makes the client fail to render the
                            // whole form, so dropping one control is the better trade
                            hostLogger().warn("[form] {} \"{}\" has no options and was skipped", kind, name);
                            continue;
                        }

                        double rawDef = numField(e, "default", 0.0);
                        size_t defIdx = 0;
                        if (rawDef > 0.0) defIdx = static_cast<size_t>(rawDef + 0.5);
                        if (defIdx >= options.size())
                        {
                            hostLogger().warn(
                                "[form] {} \"{}\" default index {} is out of range of {}, reset to 0",
                                kind, name, defIdx, options.size());
                            defIdx = 0;
                        }

                        if (!name.empty()) (*choices)[name] = options;

                        if (kind == "dropdown")
                        {
                            form->appendDropdown(
                                name, strField(e, "text"), options, defIdx, strField(e, "tooltip"));
                        }
                        else
                        {
                            form->appendStepSlider(
                                name, strField(e, "text"), options, defIdx, strField(e, "tooltip"));
                        }
                    }
                    else if (kind == "slider")
                    {
                        double mn = numField(e, "min", 0.0);
                        double mx = numField(e, "max", 100.0);
                        double step = numField(e, "step", 0.0);
                        double def = numField(e, "default", mn);
                        normalizeSlider(mn, mx, step, def, name);
                        if (!name.empty()) (*sliders)[name] = SliderSpec{mn, mx, step};
                        form->appendSlider(
                            name, strField(e, "text"), mn, mx, step, def, strField(e, "tooltip"));
                    }
                }
            }

            if (formTrace())
            {
                std::string names;
                for (auto const& [k, v] : *choices)
                {
                    if (!names.empty()) names += ", ";
                    names += k + "(" + snbtNum(v.size()) + ")";
                }
                hostLogger().info("[form] custom ticket={} choice controls: [{}]", ticket,
                                  names.empty() ? std::string{"none"} : names);
            }

            form->sendTo(
                p,
                [form, choices, sliders, weakMod, ticket](
                    Player& who, ll::form::CustomFormResult const& result, ll::form::FormCancelReason reason)
                {
                    if (!result)
                    {
                        completeTicket(weakMod, ticket, cancelledSnbt(reason));
                        return;
                    }

                    SnbtObject values;
                    SnbtObject texts;

                    for (auto const& [key, value] : *result)
                    {
                        if (formTrace())
                        {
                            hostLogger().info("[form]   {} = <{}> {}", key, variantKind(value),
                                              variantText(value));
                        }

                        auto choiceIt = choices->find(key);
                        if (choiceIt != choices->end())
                        {
                            // Choice controls emit both an index and the text, whatever
                            // this LL version hands back
                            auto const& options = choiceIt->second;
                            std::optional<size_t> idx;
                            std::string text;

                            if (std::holds_alternative<uint64_t>(value))
                            {
                                idx = static_cast<size_t>(std::get<uint64_t>(value));
                            }
                            else if (std::holds_alternative<double>(value))
                            {
                                double d = std::get<double>(value);
                                if (d >= 0.0) idx = static_cast<size_t>(d + 0.5);
                            }
                            else if (std::holds_alternative<std::string>(value))
                            {
                                text = std::get<std::string>(value);
                                idx = findOption(options, text);
                                if (!idx)
                                {
                                    hostLogger().warn(
                                        "[form] \"{}\" returned text \"{}\" that is not among the options, the index is left to the caller",
                                        key, text);
                                }
                            }

                            if (idx && *idx < options.size())
                            {
                                text = options[*idx];
                            }
                            else if (idx)
                            {
                                hostLogger().warn("[form] \"{}\" returned index {} out of range of {}",
                                                  key, *idx, options.size());
                                idx.reset();
                            }

                            if (idx) values.put(key, snbtNum(*idx) + "l");
                            if (!text.empty()) texts.putString(key, text);
                            continue;
                        }

                        if (std::holds_alternative<uint64_t>(value))
                        {
                            values.put(key, snbtNum(std::get<uint64_t>(value)) + "l");
                        }
                        else if (std::holds_alternative<double>(value))
                        {
                            double d = std::get<double>(value);
                            if (auto sl = sliders->find(key); sl != sliders->end())
                            {
                                // Clamped to the spec that was sent. Out of range means
                                // the client sent a value the form could not produce.
                                if (clampSliderValue(sl->second, d))
                                {
                                    hostLogger().warn(
                                        "[form] player {} returned slider \"{}\" value {} outside [{}, {}] step {}, clamped to {}",
                                        who.getRealName(), key, std::get<double>(value),
                                        sl->second.min, sl->second.max, sl->second.step, d);
                                }
                            }
                            values.put(key, snbtNum(d) + "d");
                        }
                        else if (std::holds_alternative<std::string>(value))
                        {
                            auto const& s = std::get<std::string>(value);
                            values.putString(key, s);
                            texts.putString(key, s);
                        }
                        // monostate covers elements with no value, such as label and
                        // divider. The key is skipped entirely rather than faked as an
                        // empty string, because bool() or int() on the caller's side
                        // would read that as the player entering an empty value rather
                        // than as no value existing here.
                    }

                    completeTicket(weakMod, ticket,
                                   "{values:" + values.wrap() + ",texts:" + texts.wrap() + "}");
                }
            );
            return true;
        }

        //  ModalForm

        bool sendModal(
            Player& p, CompoundTag const& spec, std::weak_ptr<HostedMod> weakMod, uint64_t ticket)
        {
            auto form = std::make_shared<ll::form::ModalForm>(
                strField(spec, "title"),
                strField(spec, "content"),
                strField(spec, "upper", "OK"),
                strField(spec, "lower", "Cancel")
            );
            return form->sendTo(
                p,
                [form, weakMod, ticket](
                    Player&, ll::form::ModalFormResult result, ll::form::FormCancelReason reason)
                {
                    if (!result)
                    {
                        completeTicket(weakMod, ticket, cancelledSnbt(reason));
                        return;
                    }
                    bool upper = (*result == ll::form::ModalFormSelectedButton::Upper);
                    completeTicket(weakMod, ticket, upper ? "{button:\"upper\"}" : "{button:\"lower\"}");
                }
            );
        }

        bool api_form_send(
            PierModHandle modHandle,
            PierPlayerSel sel,
            int32_t kind,
            PierStr formSnbt,
            PierFormResultCb cb,
            void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return false;
                Player* p = bridge::resolvePlayer(sel);
                if (!p) return false;

                auto spec = CompoundTag::fromSnbt(sv(formSnbt));
                if (!spec)
                {
                    mod->getLogger().error("[form] form_send: the form SNBT is not valid");
                    return false;
                }

                std::weak_ptr<HostedMod> weakMod = mod->shared_from_this();
                uint64_t ticket = registerTicket(mod, cb, user);

                bool ok = false;
                try
                {
                    switch (kind)
                    {
                    case 0:
                        ok = sendSimple(*p, *spec, weakMod, ticket);
                        break;
                    case 1:
                        ok = sendCustom(*p, *spec, weakMod, ticket);
                        break;
                    case 2:
                        ok = sendModal(*p, *spec, weakMod, ticket);
                        break;
                    default:
                        ok = false;
                        break;
                    }
                }
                catch (std::exception const& e)
                {
                    hostLogger().error("[form] form_send: building the form threw: {}", e.what());
                    ok = false;
                }
                catch (...)
                {
                    hostLogger().error("[form] form_send: building the form threw an unknown exception");
                    ok = false;
                }
                if (!ok)
                {
                    std::lock_guard lock(gFormMutex);
                    gPendingForms.erase(ticket);
                }
                return ok;
            PIER_API_GUARD_END
        }

        /** Teardown at stage 60. Clears every pending form ticket of this mod. */
        void teardown(HostedMod* mod)
        {
            std::lock_guard lock(gFormMutex);
            for (auto it = gPendingForms.begin(); it != gPendingForms.end();)
            {
                if (it->second.mod == mod) it = gPendingForms.erase(it);
                else ++it;
            }
        }

        void fill(PierApi& api) { api.form_send = &api_form_send; }

        spi::SlotPackReg regSlots{{"forms", &fill}};
        spi::TeardownReg regDown{{60, "forms", &teardown}};
    } // namespace
} // namespace pier::api_impl

#endif // !PIER_BUILD_CLIENT
