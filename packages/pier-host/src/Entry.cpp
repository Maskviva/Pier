#include <memory>

#include "ll/api/mod/ModManagerRegistry.h"
#include "ll/api/mod/NativeMod.h"
#include "ll/api/mod/RegisterHelper.h"

#include "sdk/abi.h"

#include "pier/host/api_table.h"
#include "pier/host/hosted_mod.h"
#include "pier/host/mod_host.h"
#include "pier/host/spi.h"

#ifndef PIER_BUILD_CLIENT
#include "pier/host/mod_control.h"
#endif

namespace pier
{
    class LoaderMod
    {
    public:
        static LoaderMod& getInstance()
        {
            static LoaderMod instance;
            return instance;
        }

        [[nodiscard]] ll::mod::NativeMod& getSelf() const { return *ll::mod::NativeMod::current(); }

        bool load()
        {
            auto& logger = getSelf().getLogger();

            // 顺序即正确性：
            //   1. 填表 —— 各能力包把函数指针写进 PierApi（缺席的包留 NULL，
            //      这就是「可选」的全部机制，契约 §2.1）；
            //   2. 引导 —— dimensions 布 hook、读配置等「宿主起来就干活」的步骤；
            //   3. 注册管理器 —— 从这一刻起 LeviLamina 才可能派发 pier 模组
            //      过来，而它们拿到的表必须已经建完。
            // v1 的教训是把这三件事散在静态初始化和 enable() 里，谁先谁后
            // 靠运气；现在它们排在同一个函数里，顺序看得见。
            spi::buildApi(mutableApi(), logger);
            spi::runBootstrap(logger);

            if (!ll::mod::ModManagerRegistry::getInstance().addManager(std::make_shared<ModHost>()))
            {
                logger.error("注册 '{}' 模组管理器失败", ModHostName);
                return false;
            }
            logger.info("Pier 就绪（ABI v{}，表 {} 字节）", PIER_ABI_VERSION, sizeof(PierApi));
            return true;
        }

        bool enable()
        {
#ifndef PIER_BUILD_CLIENT
            // /pier —— 运行期装卸与自检。放 enable() 而不是 load()：
            // CommandRegistrar 需要服务器命令系统已经起来。
            mod_control::registerCommand();
#endif
            return true;
        }

        bool disable() { return true; }
    };
} // namespace pier

LL_REGISTER_MOD(pier::LoaderMod, pier::LoaderMod::getInstance());
