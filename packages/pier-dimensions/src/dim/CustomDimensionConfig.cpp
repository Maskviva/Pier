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
            throw std::runtime_error("Level is not open yet, the dimension config path cannot be resolved");
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
                            // Version upgrade. The older format stored the NBT as
                            // `base64Nbt`, snappy compressed and then base64 encoded,
                            // while the current one stores SNBT text directly. It is
                            // converted in place, and an entry that fails to convert is
                            // skipped rather than discarding the whole file, since one
                            // bad record must not make every dimension disappear.
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
            // Diagnostics go through the logger. Writing to stdout directly bypasses
            // the log system, carries no newline and no level, and in a console it
            // overwrites the start of the next log line.
            pier::hostLogger().debug("[dim] saving dimension config to {}", gConfigPath.string());
            return ll::config::saveConfig(getConfig(), gConfigPath);
        }
        catch (...)
        {
            return false;
        }
    }
} // namespace pier::dimensions::CustomDimensionConfig
