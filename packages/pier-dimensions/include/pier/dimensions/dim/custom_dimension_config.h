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
     * Locates the config file at `worlds/<levelName>/dimension_config.json`.
     *
     * It follows the save and does not live under `configs/`. A dimension id is
     * allocated by the engine per save and written into the save NameIdStore, so
     * putting the ledger in a global config directory would make switching saves mean
     * the ledger and the engine drift apart immediately.
     *
     * Must be called once Level is ready, since it reads levelName, and throws
     * otherwise.
     */
    void setDimensionConfigPath();
    bool loadConfigFile();
    bool saveConfigFile();
} // namespace pier::dimensions::CustomDimensionConfig
