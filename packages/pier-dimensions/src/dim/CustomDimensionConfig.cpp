#include "pier/dimensions/dim/custom_dimension_config.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#include "ll/api/Config.h"
#include "ll/api/service/Bedrock.h"
#include "ll/api/utils/Base64Utils.h"
#include "ll/api/utils/ErrorUtils.h"
#include "ll/api/utils/StringUtils.h"

#include "mc/deps/nbt/CompoundTag.h"
#include "mc/server/PropertiesSettings.h"

#include "pier/dimensions/base/utils.h"
#include "pier/support/log.h"

namespace pier::dimensions::CustomDimensionConfig
{
    namespace
    {
        std::filesystem::path gConfigPath{u8"./worlds"};
    } // namespace

    Config& getConfig()
    {
        static Config instance;
        return instance;
    }

    void setDimensionConfigPath()
    {
        if (!ll::service::getLevel())
        {
            throw std::runtime_error("Level 还没开，定位不了维度配置路径");
        }
        gConfigPath /= ll::string_utils::str2u8str(ll::service::getPropertiesSettings()->mLevelName);
        gConfigPath /= u8"dimension_config.json";
    }

    bool loadConfigFile()
    {
        if (std::ifstream(gConfigPath).good())
        {
            try
            {
                if (ll::config::loadConfig(
                        getConfig(),
                        gConfigPath,
                        [](Config& config, nlohmann::ordered_json& data)
                        {
                            // 版本升级：老格式把 NBT 存成 snappy 压缩后再
                            // base64 的 `base64Nbt`，新格式直接存 SNBT 文本。
                            // 就地转换，转不出来的条目跳过而不是丢弃整个文件
                            // —— 一条坏数据不该让所有维度都消失。
                            if (data["version"] < config.version)
                            {
                                for (auto& item : data["dimensionList"])
                                {
                                    auto decompressed =
                                        utils::decompress(ll::base64_utils::decode(item["base64Nbt"]));
                                    auto nbtTag = CompoundTag::fromBinaryNbt(decompressed);
                                    if (!nbtTag)
                                    {
                                        continue;
                                    }
                                    item["sNbt"] = nbtTag->toSnbt(SnbtFormat::Minimize);
                                    item.erase("base64Nbt");
                                }
                            }
                            data.erase("version");
                            auto patch = ll::reflection::serialize<nlohmann::ordered_json>(config);
                            patch.value().merge_patch(data);
                            data = *std::move(patch);
                            return true;
                        }
                    ))
                {
                    return true;
                }
            }
            catch (...)
            {
                ll::error_utils::printCurrentException(pier::hostLogger());
            }
        }
        try
        {
            return ll::config::saveConfig(getConfig(), gConfigPath);
        }
        catch (...)
        {
            return false;
        }
    }

    bool saveConfigFile()
    {
        try
        {
            // 早先这里有一句 printf("saveConfigFile::: %ls", ...)：它绕开日志
            // 系统直接写 stdout、没有换行、也没有等级，在控制台里表现为把下一
            // 条日志的开头顶掉。诊断价值由下面这条 debug 覆盖。
            pier::hostLogger().debug("保存维度配置：{}", gConfigPath.string());
            return ll::config::saveConfig(getConfig(), gConfigPath);
        }
        catch (...)
        {
            return false;
        }
    }
} // namespace pier::dimensions::CustomDimensionConfig
