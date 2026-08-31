/** runtime/Commands.cpp —— 命令执行与注册（含参数化命令与枚举族）。
 *
 * Bedrock 的命令注册不可撤销，所以每个执行器闭包都归宿主所有、查一张
 * 可变的绑定表；模组卸载后它的绑定被置空，命令回答一句错误而不是悬垂。
 * 拆除（stage 50）只清绑定，不动 Bedrock 侧 —— 动不了。
  * 命令族双目标编入（与旧构建矩阵一致；客户端上 CommandRegistrar 同样可用）。
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
                    0 // overworld；命令里的选择器/坐标自己能指到别的维度
                };
                auto output =
                    ll::command::CommandRegistrar::getServerInstance().executeCommand(toString(cmd), origin);
                if (sink)
                {
                    // 拼合输出走本地化消息的 id + 参数 —— 简单、稳定；
                    // mSuccessCount 是成功判据。
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
            /** 最近一次注册声明的权限等级（0..4）。Bedrock 侧的命令建好
             *  后改不了等级，所以执行闭包按这个值再复核一次 —— 重注册只能
             *  收紧，不能放宽。 */
            int32_t permission = 0;
            /** Bedrock 侧建命令时的等级与 overload 形状摘要；重注册若不
             *  一致就拒绝，绝不静默沿用旧声明。 */
            int32_t bedrockPermission = -1;
            std::string shape;
        };

        std::mutex gCmdMutex;
        std::unordered_map<std::string, std::shared_ptr<CommandBinding>> gCommands;

        /** 给 `mod` 占下 `cmdName`；被别的活模组占着返回 nullptr。
         *  `freshlyRegistered` = Bedrock 侧的命令还没建过（头一次见这个名字）。 */
        std::shared_ptr<CommandBinding> claimBinding(
            std::string const& cmdName, HostedMod* mod, PierCommandCb cb, void* user,
            int32_t permission, std::string const& shape, bool& freshlyRegistered)
        {
            std::lock_guard lock(gCmdMutex);
            auto [it, inserted] = gCommands.try_emplace(cmdName, std::make_shared<CommandBinding>());
            auto binding = it->second;
            bool rebind = !inserted && (binding->mod == nullptr || binding->mod == mod);
            if (!inserted && !rebind) return nullptr; // 被别的活模组占着
            if (!inserted && binding->bedrockPermission >= 0)
            {
                // Bedrock 侧的命令不能注销也不能改等级/形状。重注册声明
                // 的等级或 overload 与首次不一致时，旧行为是静默沿用首次声明
                // —— 热修一个 permission 写错的 bug 会毫无提示地失效。
                if (binding->bedrockPermission != permission || binding->shape != shape)
                {
                    if (mod)
                    {
                        mod->getLogger().error(
                            "register_command('{}')：与本会话首次注册的声明不一致"
                            "（等级 {} → {}，形状 {}），Bedrock 命令一旦建好就改不了。"
                            "拒绝重绑定 —— 重启服务器以采用新声明。",
                            cmdName, binding->bedrockPermission, permission,
                            binding->shape == shape ? "相同" : "不同"
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

        /** 执行前按当前声明的等级复核一次来源权限。Bedrock 侧的等级
         *  是首次注册时定下的；这里只可能比它更严，不可能更松。 */
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

        /** 命令来源的身份 + 位置：{name,type,dim,x,y,z}。 */
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
                if (!fresh) return true; // Bedrock 侧已建过（重载后重新绑定，声明已核对一致）

                try
                {
                    using namespace ll::command;
                    // 运行时重载刻意归宿主（NativeMod::current()）所有，不归
                    // 那个模组 —— Bedrock 命令注销不了，执行器必须活得比任何
                    // 模组久。经绑定表静音让卸载后的行为可预测。
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
                                output.error("命令 '" + cmdName + "' 当前不可用（模组已禁用）");
                                return;
                            }
                            if (!originAllowed(origin, local))
                            {
                                output.error("命令 '" + cmdName + "' 需要更高的权限等级");
                                return;
                            }
                            std::string args;
                            if (auto const& p = rt["args"]; p.hold(ParamKind::RawText))
                            {
                                args = p.get<ParamKind::RawText>().mText;
                            }
                            std::string originName = originIdentity(origin);
                            CallbackScope scope{local.mod}; // 回调期间否决卸载
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

        /*  参数化命令  */

        /** 声明的参数，从 overloads SNBT 解码而来。 */
        struct ParamDecl
        {
            std::string name;
            ll::command::ParamKind::Kind kind;
            std::string enumName; // Enum / SoftEnum 用
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

        /** 把一个解析好的参数序列化进 `out`（"name":value,），没给就跳过。 */
        void appendParsedParam(
            std::string& out,
            ParamDecl const& decl,
            ll::command::RuntimeCommand const& rt,
            CommandOrigin const& origin)
        {
            using K = ll::command::ParamKind::Kind;
            auto const& p = rt[decl.name];
            if (!p.has_value()) return; // 可选参数没给

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
                    // 0x7FFFFFFF 选中最新的坐标语义。
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
                // kindFromString 吐出的每一种上面都有分支 —— 走到这里说明有人
                // 给 ParamDecl 加了新 kind 却没加序列化。吼出来而不是静默丢字
                // 段（§5.1）。旧注释声称 json/message/item 等「未序列化」，而
                // 它们分明都有分支 —— 注释撒谎（§5.4），已订正。
                hostLogger().warn(
                    "命令参数 '{}'：kind {} Bedrock 解析了但没有序列化分支",
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

                // 先把 {overloads:[[{name,kind,enum?,optional?},…],…]} 解完 ——
                // 畸形声明要在任何东西注册进 Bedrock 之前失败。
                auto tag = CompoundTag::fromSnbt(sv(overloadsSnbt));
                if (!tag || !tag->contains("overloads") || !tag->at("overloads").is_array())
                {
                    mod->getLogger().error("register_command_ex('{}')：overloads SNBT 不合法", cmdName);
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
                                "register_command_ex('{}')：不认识的参数 kind '{}'",
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
                    mod->getLogger().error("register_command_ex('{}')：没有声明任何 overload", cmdName);
                    return false;
                }

                // overload 形状摘要：每个 overload 的「name:kind[?]」序列。
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
                if (!fresh) return true; // Bedrock 侧已存在（重载后重新绑定，声明已核对一致）

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
                            // required()/optional() 返回 RuntimeOverload&（就是
                            // `ovl` 自己，为链式调用），且标了 [[nodiscard]]。
                            // RuntimeOverload 没有 operator=（只声明了移动构造
                            // 和析构），没法把返回值赋回 `ovl` —— 用
                            // static_cast<void> 显式丢弃，压掉 C4834 /
                            // -Wunused-result 而不改行为。
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
                                    output.error("命令 '" + cmdName + "' 当前不可用（模组已禁用）");
                                    return;
                                }
                                if (!originAllowed(origin, local))
                                {
                                    output.error("命令 '" + cmdName + "' 需要更高的权限等级");
                                    return;
                                }
                                CallbackScope scope{local.mod}; // 回调期间否决卸载
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

        /** 解码 {values:[…]}，元素全是字符串。 */
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
                // {values:[["name",1L],…]} —— (展示名, 序号) 对。
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
                    // 调用方只看得到 false；原因进日志。
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
                    // 调用方只看得到 false；原因进日志。
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
                    // 旧版这里静默吞掉（连日志都没有）—— 三兄弟里唯一的哑巴，
                    // 是遗漏不是设计。补上，与其余入口口径一致。
                    ll::error_utils::printCurrentException(hostLogger());
                    return false;
                }
            PIER_API_GUARD_END
        }

        /** 拆除（stage 50）：置空该模组的全部绑定。Bedrock 侧的命令留着 ——
         *  注销不了 —— 之后被敲会回答「模组已禁用」而不是跳进已释放的 dylib。 */
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
