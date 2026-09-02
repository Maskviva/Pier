/** runtime/Commands.cpp: command execution and registration, including parameterized
 *  commands and the enum family.
 * A Bedrock command registration cannot be withdrawn, so every executor closure is
 * owned by the host and consults a mutable binding table. Once a mod is unloaded its
 * binding is cleared and the command answers with an error instead of dangling.
 * Teardown at stage 50 clears bindings only and leaves the Bedrock side alone, since
 * it cannot be touched. The family is compiled into both targets, and CommandRegistrar
 * is available on the client as well.
 */
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/command/runtime/ParamKind.h"
#include "ll/api/command/runtime/RuntimeCommand.h"
#include "ll/api/command/runtime/RuntimeOverload.h"
#include "ll/api/utils/ErrorUtils.h"

#include "mc/deps/core/math/Vec3.h"
#include "mc/deps/json/Value.h"
#include "mc/deps/nbt/CompoundTag.h"
#include "mc/platform/UUID.h"
#include "mc/server/ServerLevel.h"
#include "mc/server/commands/CommandFilePath.h"
#include "mc/server/commands/CommandItem.h"
#include "mc/server/commands/CommandMessage.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandOutputMessage.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/server/commands/CommandPosition.h"
#include "mc/server/commands/CommandPositionFloat.h"
#include "mc/server/commands/CommandRawText.h"
#include "mc/server/commands/CommandSelector.h"
#include "mc/server/commands/CommandSelectorResults.h"
#include "mc/server/commands/GenerateMessageResult.h"
#include "mc/server/commands/RelativeFloat.h"
#include "mc/server/commands/ServerCommandOrigin.h"
#include "mc/world/actor/Actor.h"
#include "mc/world/actor/ActorDefinitionIdentifier.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/effect/MobEffect.h"
#include "mc/world/item/Item.h"
#include "mc/world/item/registry/ItemRegistryManager.h"
#include "mc/world/item/registry/ItemRegistryRef.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/Level.h"

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
        bool api_execute_command(PierStr cmd, void* ctx, PierCmdOutputSink sink)
        {
            PIER_API_GUARD_BEGIN
                auto* level = bridge::levelReady();
                if (!level) return false;

                ServerCommandOrigin origin{
                    "Server",
                    static_cast<ServerLevel&>(*level),
                    CommandPermissionLevel::Owner,
                    0 // Overworld. Selectors and coordinates in the command can reach elsewhere.
                };
                auto output =
                    ll::command::CommandRegistrar::getServerInstance().executeCommand(toString(cmd), origin);
                if (sink)
                {
                    // Output is assembled from the localized message id plus its
                    // parameters, which is simple and stable. mSuccessCount decides
                    // success.
                    std::string text;
                    for (auto const& msg : output.mMessages)
                    {
                        if (!text.empty()) text += '\n';
                        text += msg.mMessageId;
                        for (auto const& param : msg.mParams)
                        {
                            text += ' ';
                            text += param;
                        }
                    }
                    sink(ctx, output.mSuccessCount > 0, ps(text));
                }
                return true;
            PIER_API_GUARD_END
        }

        struct CommandBinding
        {
            HostedMod* mod = nullptr;
            PierCommandCb cb = nullptr;
            void* user = nullptr;
            /** The permission level, 0..4, declared by the most recent registration.
             *  A Bedrock command cannot change its level once built, so the executor
             *  closure rechecks against this value, and a re-registration can only
             *  tighten it, never loosen it. */
            int32_t permission = 0;
            /** The level and the overload shape digest used when the Bedrock command
             *  was built. A re-registration that disagrees is refused rather than
             *  silently keeping the earlier declaration. */
            int32_t bedrockPermission = -1;
            std::string shape;
        };

        std::mutex gCmdMutex;
        std::unordered_map<std::string, std::shared_ptr<CommandBinding>> gCommands;

        /** Claims `cmdName` for `mod`. Returns nullptr when a live mod already holds
         *  it. `freshlyRegistered` means the Bedrock command has not been built yet,
         *  so this is the first time the name is seen. */
        std::shared_ptr<CommandBinding> claimBinding(
            std::string const& cmdName, HostedMod* mod, PierCommandCb cb, void* user,
            int32_t permission, std::string const& shape, bool& freshlyRegistered)
        {
            std::lock_guard lock(gCmdMutex);
            auto [it, inserted] = gCommands.try_emplace(cmdName, std::make_shared<CommandBinding>());
            auto binding = it->second;
            bool rebind = !inserted && (binding->mod == nullptr || binding->mod == mod);
            if (!inserted && !rebind) return nullptr; // Held by another live mod
            if (!inserted && binding->bedrockPermission >= 0)
            {
                // A Bedrock command can neither be unregistered nor have its level or
                // shape changed. When a re-registration declares a different level or
                // overload set, keeping the first declaration silently would make a
                // hot fix for a wrong permission fail without any sign of it.
                if (binding->bedrockPermission != permission || binding->shape != shape)
                {
                    if (mod)
                    {
                        mod->getLogger().error(
                            "[api] register_command('{}') disagrees with the first "
                            "registration of this session, level {} to {}, shape {}; a "
                            "Bedrock command cannot change once built, so the rebind is "
                            "refused, restart the server to adopt the new declaration",
                            cmdName, binding->bedrockPermission, permission,
                            binding->shape == shape ? "same" : "different"
                        );
                    }
                    return nullptr;
                }
            }
            binding->mod = mod;
            binding->cb = cb;
            binding->user = user;
            binding->permission = permission;
            if (inserted)
            {
                binding->bedrockPermission = permission;
                binding->shape = shape;
            }
            freshlyRegistered = inserted;
            return binding;
        }

        /** Rechecks the origin permission against the currently declared level before
         *  execution. The Bedrock-side level was fixed at first registration, so this
         *  can only be stricter, never looser. */
        bool originAllowed(CommandOrigin const& origin, CommandBinding const& b)
        {
            return static_cast<int32_t>(origin.getPermissionsLevel()) >= b.permission;
        }

        std::string originIdentity(CommandOrigin const& origin)
        {
            if (auto* entity = origin.getEntity(); entity && entity->isPlayer())
            {
                return static_cast<Player*>(entity)->getRealName();
            }
            return origin.getName();
        }

        /** Identity and position of the command origin: {name,type,dim,x,y,z}. */
        std::string originSnbt(CommandOrigin const& origin)
        {
            std::string out = "{name:\"" + snbtEscape(originIdentity(origin)) + "\"";
            out += ",type:" + snbtNum(static_cast<int>(origin.getOriginType()));
            if (auto* entity = origin.getEntity())
            {
                auto pos = entity->getPosition();
                out += ",dim:" + snbtNum(static_cast<int>(entity->getDimensionId()));
                out += ",x:" + snbtDouble(pos.x) + ",y:" + snbtDouble(pos.y) + ",z:" + snbtDouble(pos.z);
            }
            out += "}";
            return out;
        }

        bool api_register_command(
            PierModHandle modHandle,
            PierStr name,
            PierStr description,
            int32_t permission,
            PierCommandCb cb,
            void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return false;
                std::string cmdName = toString(name);

                int32_t const perm = std::clamp<int32_t>(permission, 0, 4);
                bool fresh = false;
                auto binding = claimBinding(cmdName, mod, cb, user, perm, "raw", fresh);
                if (!binding) return false;
                // Already built on the Bedrock side. A rebind after a reload, with the
                // declaration already verified as identical.
                if (!fresh) return true;

                try
                {
                    using namespace ll::command;
                    // The runtime overload is deliberately owned by the host through
                    // NativeMod::current() and not by the mod. A Bedrock command cannot
                    // be unregistered, so the executor must outlive any mod. Muting it
                    // through the binding table makes post-unload behavior
                    // predictable.
                    auto& handle = CommandRegistrar::getServerInstance().getOrCreateCommand(
                        cmdName,
                        toString(description),
                        static_cast<CommandPermissionLevel>(std::clamp<int32_t>(permission, 0, 4))
                    );
                    handle.runtimeOverload().optional("args", ParamKind::RawText).execute(
                        [binding, cmdName](
                            CommandOrigin const& origin, CommandOutput& output, RuntimeCommand const& rt)
                        {
                            CommandBinding local;
                            {
                                std::lock_guard lock(gCmdMutex);
                                local = *binding;
                            }
                            if (!local.mod || local.mod->commandsMuted || !local.cb)
                            {
                                output.error("command '" + cmdName + "' is currently unavailable, its mod is disabled");
                                return;
                            }
                            if (!originAllowed(origin, local))
                            {
                                output.error("command '" + cmdName + "' requires a higher permission level");
                                return;
                            }
                            std::string args;
                            if (auto const& p = rt["args"]; p.hold(ParamKind::RawText))
                            {
                                args = p.get<ParamKind::RawText>().mText;
                            }
                            std::string originName = originIdentity(origin);
                            CallbackScope scope{local.mod}; // Veto unload during the callback
                            local.cb(
                                local.user,
                                ps(args),
                                ps(originName),
                                &output,
                                [](void* c, PierStr s)
                                { static_cast<CommandOutput*>(c)->success(toString(s)); },
                                [](void* c, PierStr s)
                                { static_cast<CommandOutput*>(c)->error(toString(s)); }
                            );
                        }
                    );
                    return true;
                }
                catch (...)
                {
                    ll::error_utils::printCurrentException(mod->getLogger());
                    std::lock_guard lock(gCmdMutex);
                    gCommands.erase(cmdName);
                    return false;
                }
            PIER_API_GUARD_END
        }

        /*  Parameterized commands  */

        /** A declared parameter, decoded from the overloads SNBT. */
        struct ParamDecl
        {
            std::string name;
            ll::command::ParamKind::Kind kind;
            std::string enumName; // For Enum and SoftEnum
            bool optional = false;
        };

        std::optional<ll::command::ParamKind::Kind> kindFromString(std::string_view s)
        {
            using K = ll::command::ParamKind::Kind;
            if (s == "int") return K::Int;
            if (s == "bool") return K::Bool;
            if (s == "float") return K::Float;
            if (s == "dimension") return K::Dimension;
            if (s == "string") return K::String;
            if (s == "enum") return K::Enum;
            if (s == "soft_enum") return K::SoftEnum;
            if (s == "actor") return K::Actor;
            if (s == "player") return K::Player;
            if (s == "block_pos") return K::BlockPos;
            if (s == "vec3") return K::Vec3;
            if (s == "raw_text") return K::RawText;
            if (s == "message") return K::Message;
            if (s == "json") return K::JsonValue;
            if (s == "item") return K::Item;
            if (s == "block_name") return K::BlockName;
            if (s == "effect") return K::Effect;
            if (s == "actor_type") return K::ActorType;
            if (s == "command") return K::Command;
            if (s == "relative_float") return K::RelativeFloat;
            if (s == "file_path") return K::FilePath;
            return std::nullopt;
        }

        /** Serializes one parsed parameter into `out` as "name":value, and skips it
         *  when it was not supplied. */
        void appendParsedParam(
            std::string& out,
            ParamDecl const& decl,
            ll::command::RuntimeCommand const& rt,
            CommandOrigin const& origin)
        {
            using K = ll::command::ParamKind::Kind;
            auto const& p = rt[decl.name];
            if (!p.has_value()) return; // An optional parameter was not supplied

            auto key = [&](std::string const& v)
            {
                out += "\"" + snbtEscape(decl.name) + "\":" + v + ",";
            };

            switch (decl.kind)
            {
            case K::Int:
                if (p.hold(K::Int)) key(snbtNum(p.get<K::Int>()));
                break;
            case K::Bool:
                if (p.hold(K::Bool)) key(p.get<K::Bool>() ? "1b" : "0b");
                break;
            case K::Float:
                if (p.hold(K::Float)) key(snbtNum(p.get<K::Float>()) + "f");
                break;
            case K::Dimension:
                if (p.hold(K::Dimension)) key(snbtNum(static_cast<int>(p.get<K::Dimension>())));
                break;
            case K::String:
                if (p.hold(K::String)) key("\"" + snbtEscape(p.get<K::String>()) + "\"");
                break;
            case K::Enum:
                if (p.hold(K::Enum)) key("\"" + snbtEscape(p.get<K::Enum>().name) + "\"");
                break;
            case K::SoftEnum:
                if (p.hold(K::SoftEnum)) key("\"" + snbtEscape(p.get<K::SoftEnum>()) + "\"");
                break;
            case K::RawText:
                if (p.hold(K::RawText)) key("\"" + snbtEscape(p.get<K::RawText>().mText) + "\"");
                break;
            case K::Player:
                if (p.hold(K::Player))
                {
                    std::string list = "[";
                    for (auto* pl : p.get<K::Player>().results(origin))
                    {
                        if (!pl) continue;
                        list += "{name:\"" + snbtEscape(pl->getRealName())
                            + "\",xuid:\"" + snbtEscape(pl->getXuid())
                            + "\",uuid:\"" + snbtEscape(pl->getUuid().asString())
                            + "\",id:" + snbtNum(pl->getOrCreateUniqueID().rawID) + "l},";
                    }
                    if (list.back() == ',') list.pop_back();
                    list += "]";
                    key(list);
                }
                break;
            case K::Actor:
                if (p.hold(K::Actor))
                {
                    std::string list = "[";
                    for (auto* a : p.get<K::Actor>().results(origin))
                    {
                        if (!a) continue;
                        list += "{id:" + snbtNum(a->getOrCreateUniqueID().rawID)
                            + "l,type:\"" + snbtEscape(a->getTypeName()) + "\"},";
                    }
                    if (list.back() == ',') list.pop_back();
                    list += "]";
                    key(list);
                }
                break;
            case K::BlockPos:
                if (p.hold(K::BlockPos))
                {
                    // 0x7FFFFFFF selects the newest coordinate semantics.
                    auto bp = p.get<K::BlockPos>().getBlockPos(0x7FFFFFFF, origin, Vec3::ZERO());
                    key("{x:" + snbtNum(bp.x) + ",y:" + snbtNum(bp.y) + ",z:" + snbtNum(bp.z) + "}");
                }
                break;
            case K::BlockName:
                if (p.hold(K::BlockName))
                {
                    key("\"" + snbtEscape(p.get<K::BlockName>().getBlockName()) + "\"");
                }
                break;
            case K::Vec3:
                if (p.hold(K::Vec3))
                {
                    auto v = p.get<K::Vec3>().getPosition(0x7FFFFFFF, origin, Vec3::ZERO());
                    key("{x:" + snbtNum(v.x) + "d,y:" + snbtNum(v.y) + "d,z:" + snbtNum(v.z) + "d}");
                }
                break;
            case K::Message:
                if (p.hold(K::Message))
                {
                    auto res = p.get<K::Message>().generateMessage(origin, 1024);
                    if (res.mIsValid) key("\"" + snbtEscape(res.mMessage.get()) + "\"");
                }
                break;
            case K::RelativeFloat:
                if (p.hold(K::RelativeFloat))
                {
                    auto const& rf = p.get<K::RelativeFloat>();
                    key("{offset:" + snbtNum(rf.mOffset) + "f,relative:"
                        + (rf.mRelative ? "1b" : "0b") + "}");
                }
                break;
            case K::FilePath:
                if (p.hold(K::FilePath))
                    key("\"" + snbtEscape(p.get<K::FilePath>().mText.get()) + "\"");
                break;
            case K::JsonValue:
                if (p.hold(K::JsonValue))
                    key("\"" + snbtEscape(p.get<K::JsonValue>().toStyledString()) + "\"");
                break;
            case K::Effect:
                if (auto const* eff = p.hold(K::Effect) ? p.get<K::Effect>() : nullptr)
                    key("{id:" + snbtNum(static_cast<int>(eff->mId))
                        + ",name:\"" + snbtEscape(eff->mResourceName.get()) + "\"}");
                break;
            case K::ActorType:
                if (auto const* ad = p.hold(K::ActorType) ? p.get<K::ActorType>() : nullptr)
                    key("\"" + snbtEscape(ad->mFullName.get()) + "\"");
                break;
            case K::Item:
                if (p.hold(K::Item))
                {
                    auto const& ci = p.get<K::Item>();
                    std::string itemName;
                    if (auto it = ItemRegistryManager::getItemRegistry()
                                      .getItem(static_cast<short>(ci.mId)))
                        itemName = it->getFullItemName();
                    key("{id:" + snbtNum(static_cast<int>(ci.mId))
                        + ",aux:" + snbtNum(static_cast<int>(ci.mVersion))
                        + ",name:\"" + snbtEscape(itemName) + "\"}");
                }
                break;
            case K::Command:
                if (p.hold(K::Command) && p.get<K::Command>()) key("1b");
                break;
            default:
                // Every kind kindFromString can produce has a branch above, so
                // reaching here means a new kind was added to ParamDecl without a
                // serialization branch. It is reported rather than dropping the field
                // silently (§5.1).
                hostLogger().warn(
                    "[api] command parameter '{}': Bedrock parsed kind {} but it has no serialization branch",
                    decl.name, static_cast<int>(decl.kind));
                break;
            }
        }

        bool api_register_command_ex(
            PierModHandle modHandle,
            PierStr name,
            PierStr description,
            int32_t permission,
            PierStr overloadsSnbt,
            PierCommandCb cb,
            void* user)
        {
            PIER_API_GUARD_BEGIN
                auto* mod = asMod(modHandle);
                if (!mod || !cb) return false;
                std::string cmdName = toString(name);

                // The whole {overloads:[[{name,kind,enum?,optional?},...],...]} is
                // decoded first, so a malformed declaration fails before anything is
                // registered with Bedrock.
                auto tag = CompoundTag::fromSnbt(sv(overloadsSnbt));
                if (!tag || !tag->contains("overloads") || !tag->at("overloads").is_array())
                {
                    mod->getLogger().error("[api] register_command_ex('{}'): the overloads SNBT is not valid", cmdName);
                    return false;
                }
                std::vector<std::vector<ParamDecl>> overloads;
                for (auto const& ovlPtr : tag->at("overloads").get<ListTag>())
                {
                    if (!ovlPtr || ovlPtr->getId() != Tag::Type::List) continue;
                    std::vector<ParamDecl> decls;
                    for (auto const& paramPtr : static_cast<ListTag const&>(*ovlPtr))
                    {
                        if (!paramPtr || paramPtr->getId() != Tag::Type::Compound) continue;
                        auto const& po = static_cast<CompoundTag const&>(*paramPtr);
                        if (!po.contains("name") || !po.contains("kind")) continue;
                        ParamDecl d;
                        d.name = std::string_view{po.at("name")};
                        auto kind = kindFromString(std::string_view{po.at("kind")});
                        if (!kind)
                        {
                            mod->getLogger().error(
                                "[api] register_command_ex('{}'): unknown parameter kind '{}'",
                                cmdName,
                                std::string_view{po.at("kind")}
                            );
                            return false;
                        }
                        d.kind = *kind;
                        if (po.contains("enum")) d.enumName = std::string_view{po.at("enum")};
                        if (po.contains("optional"))
                            d.optional = static_cast<int64_t>(po.at("optional")) != 0;
                        decls.push_back(std::move(d));
                    }
                    overloads.push_back(std::move(decls));
                }
                if (overloads.empty())
                {
                    mod->getLogger().error("[api] register_command_ex('{}'): no overload was declared", cmdName);
                    return false;
                }

                // The overload shape digest: the "name:kind[?]" sequence of each
                // overload.
                std::string shape;
                for (auto const& decls : overloads)
                {
                    shape += '[';
                    for (auto const& d : decls)
                    {
                        shape += d.name + ':' + std::to_string(static_cast<int>(d.kind))
                            + (d.optional ? "?" : "") + ',';
                    }
                    shape += ']';
                }
                int32_t const perm = std::clamp<int32_t>(permission, 0, 4);
                bool fresh = false;
                auto binding = claimBinding(cmdName, mod, cb, user, perm, shape, fresh);
                if (!binding) return false;
                // Already exists on the Bedrock side. A rebind after a reload, with the
                // declaration already verified as identical.
                if (!fresh) return true;

                try
                {
                    using namespace ll::command;
                    auto& handle = CommandRegistrar::getServerInstance().getOrCreateCommand(
                        cmdName,
                        toString(description),
                        static_cast<CommandPermissionLevel>(std::clamp<int32_t>(permission, 0, 4))
                    );
                    for (size_t idx = 0; idx < overloads.size(); ++idx)
                    {
                        auto const& decls = overloads[idx];
                        auto ovl = handle.runtimeOverload();
                        for (auto const& d : decls)
                        {
                            bool isEnum = d.kind == ParamKind::Enum || d.kind == ParamKind::SoftEnum;
                            // required() and optional() return RuntimeOverload&, which
                            // is `ovl` itself for chaining, and are marked
                            // [[nodiscard]]. RuntimeOverload has no operator=, only a
                            // move constructor and a destructor, so the result cannot
                            // be assigned back to `ovl`. static_cast<void> discards it
                            // explicitly and silences C4834 and -Wunused-result without
                            // changing behavior.
                            if (isEnum)
                            {
                                if (d.optional)
                                    static_cast<void>(ovl.optional(d.name, d.kind, d.enumName));
                                else
                                    static_cast<void>(ovl.required(d.name, d.kind, d.enumName));
                            }
                            else
                            {
                                if (d.optional) static_cast<void>(ovl.optional(d.name, d.kind));
                                else static_cast<void>(ovl.required(d.name, d.kind));
                            }
                        }
                        ovl.execute(
                            [binding, cmdName, decls, idx](
                                CommandOrigin const& origin,
                                CommandOutput& output,
                                RuntimeCommand const& rt)
                            {
                                CommandBinding local;
                                {
                                    std::lock_guard lock(gCmdMutex);
                                    local = *binding;
                                }
                                if (!local.mod || local.mod->commandsMuted || !local.cb)
                                {
                                    output.error("command '" + cmdName + "' is currently unavailable, its mod is disabled");
                                    return;
                                }
                                if (!originAllowed(origin, local))
                                {
                                    output.error("command '" + cmdName + "' requires a higher permission level");
                                    return;
                                }
                                CallbackScope scope{local.mod}; // Veto unload during the callback
                                std::string args = "{overload:" + snbtNum(idx) + ",args:{";
                                for (auto const& d : decls)
                                {
                                    appendParsedParam(args, d, rt, origin);
                                }
                                if (args.back() == ',') args.pop_back();
                                args += "}}";
                                std::string origin_ = originSnbt(origin);
                                local.cb(
                                    local.user,
                                    ps(args),
                                    ps(origin_),
                                    &output,
                                    [](void* c, PierStr s)
                                    { static_cast<CommandOutput*>(c)->success(toString(s)); },
                                    [](void* c, PierStr s)
                                    { static_cast<CommandOutput*>(c)->error(toString(s)); }
                                );
                            }
                        );
                    }
                    return true;
                }
                catch (...)
                {
                    ll::error_utils::printCurrentException(mod->getLogger());
                    std::lock_guard lock(gCmdMutex);
                    gCommands.erase(cmdName);
                    return false;
                }
            PIER_API_GUARD_END
        }

        /** Decodes {values:[...]} where every element is a string. */
        std::optional<std::vector<std::string>> decodeStringValues(PierStr snbt)
        {
            auto tag = CompoundTag::fromSnbt(sv(snbt));
            if (!tag || !tag->contains("values") || !tag->at("values").is_array()) return std::nullopt;
            std::vector<std::string> out;
            for (auto const& p : tag->at("values").get<ListTag>())
            {
                if (!p || p->getId() != Tag::Type::String) continue;
                out.emplace_back(static_cast<std::string const&>(static_cast<StringTag const&>(*p)));
            }
            return out;
        }

        bool api_register_command_enum(PierStr name, PierStr valuesSnbt)
        {
            PIER_API_GUARD_BEGIN
                // {values:[["name",1L],...]} is a list of (display name, ordinal) pairs.
                auto tag = CompoundTag::fromSnbt(sv(valuesSnbt));
                if (!tag || !tag->contains("values") || !tag->at("values").is_array()) return false;
                std::vector<std::pair<std::string, uint64_t>> values;
                for (auto const& p : tag->at("values").get<ListTag>())
                {
                    if (!p || p->getId() != Tag::Type::List) continue;
                    auto const& pair = static_cast<ListTag const&>(*p);
                    if (pair.size() < 1) continue;
                    auto const& namePtr = pair[0];
                    if (!namePtr || namePtr->getId() != Tag::Type::String) continue;
                    uint64_t idx = values.size();
                    if (pair.size() >= 2 && pair[1] && pair[1]->getId() == Tag::Type::Int64)
                    {
                        idx = static_cast<uint64_t>(static_cast<Int64Tag const&>(*pair[1]).data);
                    }
                    values.emplace_back(
                        std::string{
                            static_cast<std::string const&>(static_cast<StringTag const&>(*namePtr))},
                        idx);
                }
                if (values.empty()) return false;
                try
                {
                    return ll::command::CommandRegistrar::getServerInstance().tryRegisterRuntimeEnum(
                        toString(name),
                        std::move(values)
                    );
                }
                catch (...)
                {
                    // The caller only sees false; the reason goes to the log.
                    ll::error_utils::printCurrentException(hostLogger());
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_register_command_soft_enum(PierStr name, PierStr valuesSnbt)
        {
            PIER_API_GUARD_BEGIN
                auto values = decodeStringValues(valuesSnbt);
                if (!values) return false;
                try
                {
                    return ll::command::CommandRegistrar::getServerInstance().tryRegisterSoftEnum(
                        toString(name), std::move(*values));
                }
                catch (...)
                {
                    // The caller only sees false; the reason goes to the log.
                    ll::error_utils::printCurrentException(hostLogger());
                    return false;
                }
            PIER_API_GUARD_END
        }

        bool api_update_command_soft_enum(PierStr name, int32_t op, PierStr valuesSnbt)
        {
            PIER_API_GUARD_BEGIN
                auto values = decodeStringValues(valuesSnbt);
                if (!values) return false;
                try
                {
                    auto& reg = ll::command::CommandRegistrar::getServerInstance();
                    switch (op)
                    {
                    case 0:
                        return reg.setSoftEnumValues(toString(name), std::move(*values));
                    case 1:
                        return reg.addSoftEnumValues(toString(name), std::move(*values));
                    case 2:
                        return reg.removeSoftEnumValues(toString(name), std::move(*values));
                    default:
                        return false;
                    }
                }
                catch (...)
                {
                    // Logged like the other two entry points of this family. Swallowing
                    // it silently would leave one of the three without any trace.
                    ll::error_utils::printCurrentException(hostLogger());
                    return false;
                }
            PIER_API_GUARD_END
        }

        /** Teardown at stage 50. Clears every binding of this mod. The Bedrock command
         *  stays, since it cannot be unregistered, and answers that its mod is disabled
         *  instead of jumping into a freed dylib. */
        void teardown(HostedMod* mod)
        {
            std::lock_guard lock(gCmdMutex);
            for (auto& [name, binding] : gCommands)
            {
                if (binding->mod == mod)
                {
                    binding->mod = nullptr;
                    binding->cb = nullptr;
                    binding->user = nullptr;
                }
            }
        }

        void fill(PierApi& api)
        {
            api.execute_command = &api_execute_command;
            api.register_command = &api_register_command;
            api.register_command_ex = &api_register_command_ex;
            api.register_command_enum = &api_register_command_enum;
            api.register_command_soft_enum = &api_register_command_soft_enum;
            api.update_command_soft_enum = &api_update_command_soft_enum;
        }

        spi::SlotPackReg regSlots{{"commands", &fill}};
        spi::TeardownReg regDown{{50, "commands", &teardown}};
    } // namespace
} // namespace pier::api_impl
