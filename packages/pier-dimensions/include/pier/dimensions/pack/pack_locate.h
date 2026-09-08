/** pack_locate.h: from a pack config path to a verified binary.
 * A pack is a directory with a config file and a binary. The host reads four keys of
 * the config, pier_terrain, type, binary and sha256, and nothing else, so the same file
 * can carry whatever a mod wants to show in its own UI. Paths are relative to the server
 * root and must stay inside it; an absolute path or a ".." component is refused, and no
 * message ever echoes file content. The binary is read whole, its sha256 compared with
 * the config's, and its magic with the config's type, before anything is decoded. */
#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_reader.h"
#include "pier/dimensions/pack/template_pack.h"
#include "pier/dimensions/pack/volume_pack.h"

namespace pier::dimensions::pack
{
    /** Why a location or load was refused, for the slot's return code. Values match
     *  the PIER_PACK_* constants of abi.h. */
    enum class PackStatus : int
    {
        Ok = 0,
        BadPath = -1,
        ConfigUnreadable = -2,
        ConfigInvalid = -3,
        BinaryUnreadable = -4,
        Corrupt = -5,
        KindMismatch = -6,
        HashMismatch = -7,
        Unsupported = -8,
        Params = -9,
        Constraint = -10,
        Height = -11,
        StoredMismatch = -12,
        Spec = -13,
        Host = -14,
    };

    struct PackLocation
    {
        std::string configPath;
        std::string binaryPath;
        std::string kind;
        std::string sha256;
    };

    /** Resolves a relative path under the server root; nullopt for an absolute path, a
     *  ".." component, or a result outside the root. The returned string is the path
     *  to open. root is the current directory when empty. */
    std::optional<std::string> resolveUnderRoot(std::string const& relative, std::string const& root,
                                                std::vector<std::string>& problems);

    /** Reads the config and returns where the binary is and what it claims to be. */
    std::optional<PackLocation> locatePack(std::string const& configRelative, std::string const& root,
                                           PackStatus& status, std::vector<std::string>& problems);

    /** Loads the binary of a location and checks its hash against the config's and its
     *  magic against the config's type. */
    std::optional<PackFile> loadLocated(PackLocation const& loc, PackStatus& status, std::vector<std::string>& problems);

    /** Parsed template packs by config path and hash, shared between the slot that
     *  mounts and the dimension that generates. A pack is decoded once per hash; a
     *  file that changed on disk is a different hash and a different entry. */
    class PackCache
    {
        std::mutex mMutex;
        struct Entry
        {
            std::string path;
            std::string sha256;
            std::shared_ptr<TemplatePack const> pack;
        };
        std::vector<Entry> mTemplates;
        struct VolEntry
        {
            std::string path;
            std::string sha256;
            std::shared_ptr<VolumePack const> pack;
        };
        std::vector<VolEntry> mVolumes;

    public:
        static PackCache& instance();

        /** The decoded template pack at configRelative whose binary hashes to sha256.
         *  Loads and decodes on a miss; a hash that differs from expectedSha256 is
         *  reported as HashMismatch, never served. */
        std::shared_ptr<TemplatePack const> templateAt(std::string const& configRelative, std::string const& expectedSha256,
                                                       std::string const& root, PackStatus& status,
                                                       std::vector<std::string>& problems);

        /** The same for a volume pack. */
        std::shared_ptr<VolumePack const> volumeAt(std::string const& configRelative, std::string const& expectedSha256,
                                                   std::string const& root, PackStatus& status,
                                                   std::vector<std::string>& problems);
    };
} // namespace pier::dimensions::pack
