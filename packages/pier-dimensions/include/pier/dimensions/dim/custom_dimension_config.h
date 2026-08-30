#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

namespace pier::dimensions::CustomDimensionConfig
{
    struct DimensionInfo
    {
        int dimId{};
        std::string sNbt;
    };

    struct Config
    {
        int version = 4;
        std::unordered_map<std::string, DimensionInfo> dimensionList{};
    };

    Config& getConfig();

    /**
     * 定位配置文件：`worlds/<levelName>/dimension_config.json`。
     *
     * **跟着存档走，不在 `configs/` 下面。** 维度 id 是引擎按存档分配、写进
     * 存档 NameIdStore 的，把台账放到全局配置目录会让「换一个存档」变成
     * 「台账和引擎立刻漂移」。
     *
     * 必须在 Level 就绪之后调（要读 levelName），未就绪时抛。
     */
    void setDimensionConfigPath();
    bool loadConfigFile();
    bool saveConfigFile();
} // namespace pier::dimensions::CustomDimensionConfig
