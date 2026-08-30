/** actors/Forms.cpp —— 带异步结果回调的表单。
 *
 * 回调在调用帧之后才触发的唯一入口。生命周期纪律与命令一致：ll::form 的
 * 回调只捕获 weak_ptr<HostedMod> 加一张宿主侧待决表的票据；玩家应答时模
 * 组若已卸载（票据已清）或已禁用（静音），回调被静默丢弃，卸载清空全部
 * 待决票据。
 *
 * ── 结果编码 ─────────────────────────────────────────────────────────
 * `ll::form::CustomFormElementResult` 是 variant<monostate, uint64, double,
 * string>，而 **dropdown / step_slider 在不同 LL 版本上回传的东西不一样**：
 * 有的版本给选中项的下标（uint64），有的给选中项的文本（string）。上层
 * 只按下标解读，于是碰到字符串就静默回退成 0 —— 表现为「不管选哪一项，
 * 拿到的永远是第一项」。
 *
 * 这里在桥这一层把它收口：构表时记住每个选择型控件的 options，回传时
 *   - 拿到整数 → 直接当下标；
 *   - 拿到字符串 → 在 options 里查回下标（先精确匹配，再去掉 §x 颜色码
 *     匹配一次）。
 * 序列化出去的结果永远是 **下标（values）+ 文本（texts）两份**，上层不用
 * 再猜 LL 这一版是哪种行为。
 *
 * 同时把「会让客户端整个表单渲染失败」的几种参数在这里钳住：空 options 的
 * 下拉框、越界的默认下标、不在 [min,max] 内或没落在步进点上的滑块默认值。
 *
 * 调试：`PIER_TRACE_FORM=1` 打印每个表单的元素清单，以及回传时每个键的
 * variant 类型和原始值 —— 表单类问题基本一眼就能定位。
 */
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
            HostedMod* mod = nullptr; // 只作身份比对；永不盲目解引用
            PierFormResultCb cb = nullptr;
            void* user = nullptr;
        };

        std::mutex gFormMutex;
        std::unordered_map<uint64_t, PendingForm> gPendingForms;
        uint64_t gNextTicket = 1;

        /** 选择型控件（dropdown / step_slider）的 name → options 台账。 */
        using ChoiceTable = std::unordered_map<std::string, std::vector<std::string>>;

        /** PIER_TRACE_FORM=1 打开表单追踪。只读一次。 */
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
         * 把票据取出表并恰好触发一次回调 —— 除非它的模组已卸载（票据已被
         * 清掉）或已禁用（静音）。跑在服务器线程上（ll::form 保证）。
         */
        void completeTicket(std::weak_ptr<HostedMod> weakMod, uint64_t ticket, std::string const& resultSnbt)
        {
            if (formTrace()) hostLogger().info("[form] ticket={} -> {}", ticket, resultSnbt);
            PendingForm pending;
            {
                std::lock_guard lock(gFormMutex);
                auto it = gPendingForms.find(ticket);
                if (it == gPendingForms.end()) return; // 卸载时清掉了
                pending = it->second;
                gPendingForms.erase(it);
            }
            auto mod = weakMod.lock();
            if (!mod || mod.get() != pending.mod) return; // 模组没了
            if (!mod->isEnabled()) return;                // 禁用期间静音
            if (pending.cb) pending.cb(pending.user, ps(resultSnbt));
        }

        std::string cancelledSnbt(ll::form::FormCancelReason reason)
        {
            int code = reason.has_value() ? static_cast<int>(*reason) : -1;
            return "{cancelled:1b,reason:" + snbtNum(code) + "}";
        }

        /** 取字符串字段，带默认。 */
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

        // ── 选择型控件的下标 / 文本互查 ──────────────────────────────────

        /** 去掉 Minecraft 的 §x 颜色控制码，用于第二轮宽松匹配。 */
        std::string stripFormatCodes(std::string_view s)
        {
            std::string out;
            out.reserve(s.size());
            for (size_t i = 0; i < s.size();)
            {
                // § 在 UTF-8 里是 0xC2 0xA7
                if (i + 2 < s.size() && static_cast<unsigned char>(s[i]) == 0xC2
                    && static_cast<unsigned char>(s[i + 1]) == 0xA7)
                {
                    i += 3; // 跳过 §、控制码本身
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
            // 客户端可能把颜色码吃掉再回传，宽松再比一次
            std::string bare = stripFormatCodes(text);
            for (size_t i = 0; i < opts.size(); ++i)
                if (stripFormatCodes(opts[i]) == bare) return i;
            return std::nullopt;
        }

        // ── SNBT 拼装 ───────────────────────────────────────────────────

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

        /** variant 的类型名，只给 trace 用。 */
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

        // ── SimpleForm ──────────────────────────────────────────────────

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
                // 没有按钮的 SimpleForm 客户端点不动，只能靠关闭退出 —— 上层
                // 多半是列表拼空了，这属于逻辑错误，值得留一行。
                hostLogger().warn("[form] SimpleForm \"{}\" 一个按钮都没有", strField(spec, "title"));
            }
            if (formTrace())
            {
                hostLogger().info("[form] simple ticket={} 按钮 {} 个", ticket, buttons);
            }
            form->sendTo(p, [form, weakMod, ticket](Player&, int button, ll::form::FormCancelReason reason)
            {
                if (button < 0)
                {
                    completeTicket(weakMod, ticket, cancelledSnbt(reason));
                }
                else
                {
                    completeTicket(weakMod, ticket, "{button:" + snbtNum(button) + "}");
                }
            });
            return true;
        }

        // ── CustomForm ──────────────────────────────────────────────────

        /**
         * 把滑块参数归一化到客户端能接受的形状。
         *
         * 基岩客户端对 slider 的容忍度很低：默认值越界、或者 (max-min) 不是
         * step 的整数倍，整个表单可能直接不渲染（玩家看到的是「打开就没
         * 了」）。上层配置是可以被人手改成任意值的，所以这个钳制必须在桥做。
         */
        void normalizeSlider(double& mn, double& mx, double& step, double& def, std::string const& name)
        {
            if (!(mn <= mx)) std::swap(mn, mx); // 顺手接住 NaN
            if (!(step > 0.0)) step = 0.0;

            if (step > 0.0)
            {
                double span = mx - mn;
                if (span < step)
                {
                    // 步长比整个范围还大 —— 退化成只有一格，客户端会算出空刻度
                    step = span > 0.0 ? span : 0.0;
                }
                else
                {
                    // 把 max 收到最后一个落在步进点上的值
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
                    "[form] 滑块 \"{}\" 的默认值 {} 不在 [{}, {}] 的步进点上，已调整为 {}",
                    name, before, mn, mx, def);
            }
        }

        bool sendCustom(
            Player& p, CompoundTag const& spec, std::weak_ptr<HostedMod> weakMod, uint64_t ticket)
        {
            auto form = std::make_shared<ll::form::CustomForm>(strField(spec, "title"));
            auto choices = std::make_shared<ChoiceTable>();
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

                    // 有值的控件必须有唯一的 name —— 结果是按 name 索引的
                    // unordered_map，重名会互相覆盖，上层拿到的是「其中一个」。
                    bool valued = (kind == "input" || kind == "toggle" || kind == "dropdown"
                        || kind == "step_slider" || kind == "slider");
                    if (valued)
                    {
                        if (name.empty())
                        {
                            hostLogger().warn("[form] {} 控件没有 name，结果里取不到它", kind);
                        }
                        else if (!seenNames.insert(name).second)
                        {
                            hostLogger().warn("[form] name \"{}\" 重复，后一个会覆盖前一个", name);
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
                            // 空的下拉框会让客户端渲染整个表单失败，宁可少一个控件
                            hostLogger().warn("[form] {} \"{}\" 没有任何选项，已跳过", kind, name);
                            continue;
                        }

                        double rawDef = numField(e, "default", 0.0);
                        size_t defIdx = 0;
                        if (rawDef > 0.0) defIdx = static_cast<size_t>(rawDef + 0.5);
                        if (defIdx >= options.size())
                        {
                            hostLogger().warn(
                                "[form] {} \"{}\" 的默认下标 {} 越界（共 {} 项），已回到 0",
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
                hostLogger().info("[form] custom ticket={} 选择型控件: [{}]", ticket,
                                  names.empty() ? std::string{"无"} : names);
            }

            form->sendTo(
                p,
                [form, choices, weakMod, ticket](
                    Player&, ll::form::CustomFormResult const& result, ll::form::FormCancelReason reason)
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
                            // 选择型：无论 LL 这一版给的是下标还是文本，出去的
                            // 都是下标 + 文本
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
                                        "[form] \"{}\" 回传的文本 \"{}\" 不在选项里，下标交给上层兜底",
                                        key, text);
                                }
                            }

                            if (idx && *idx < options.size())
                            {
                                text = options[*idx];
                            }
                            else if (idx)
                            {
                                hostLogger().warn("[form] \"{}\" 回传的下标 {} 越界（共 {} 项）",
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
                            values.put(key, snbtNum(std::get<double>(value)) + "d");
                        }
                        else if (std::holds_alternative<std::string>(value))
                        {
                            auto const& s = std::get<std::string>(value);
                            values.putString(key, s);
                            texts.putString(key, s);
                        }
                        // monostate（label / divider 这类没有值的元素）：整键略
                        // 过，不要伪造成空字符串 —— 上层的 bool()/int() 会把它
                        // 当成「玩家填了个空值」而不是「这里没有值」。
                    }

                    completeTicket(weakMod, ticket,
                                   "{values:" + values.wrap() + ",texts:" + texts.wrap() + "}");
                }
            );
            return true;
        }

        // ── ModalForm ───────────────────────────────────────────────────

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
                    mod->getLogger().error("form_send：表单 SNBT 不合法");
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
                    hostLogger().error("form_send：构建表单时抛异常: {}", e.what());
                    ok = false;
                }
                catch (...)
                {
                    hostLogger().error("form_send：构建表单时抛了未知异常");
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

        /** 拆除（stage 60）：清空该模组的全部待决表单票据。 */
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
