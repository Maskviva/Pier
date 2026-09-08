/** PackLocate.cpp: path policy, the four config keys, binary verification, cache.
 * The config is scanned with a small strict JSON reader of its own so the pack layer
 * stays engine-free: only top-level string values are kept, nested values are skipped
 * structurally, and any syntax error refuses the file. */
#include "pier/dimensions/pack/pack_locate.h"

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <system_error>

namespace pier::dimensions::pack
{
    namespace
    {
        namespace fs = std::filesystem;

        struct Json
        {
            std::string const& s;
            std::size_t i = 0;
            bool ok = true;

            void ws() { while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\n' || s[i] == '\r')) ++i; }
            bool take(char c)
            {
                ws();
                if (i < s.size() && s[i] == c) { ++i; return true; }
                return false;
            }
            bool string(std::string& out)
            {
                ws();
                if (i >= s.size() || s[i] != '"') return false;
                ++i;
                out.clear();
                while (i < s.size())
                {
                    char c = s[i++];
                    if (c == '"') return true;
                    if (c == '\\')
                    {
                        if (i >= s.size()) return false;
                        char e = s[i++];
                        switch (e)
                        {
                        case '"': out += '"'; break;
                        case '\\': out += '\\'; break;
                        case '/': out += '/'; break;
                        case 'b': out += '\b'; break;
                        case 'f': out += '\f'; break;
                        case 'n': out += '\n'; break;
                        case 'r': out += '\r'; break;
                        case 't': out += '\t'; break;
                        case 'u':
                        {
                            if (i + 4 > s.size()) return false;
                            unsigned v = 0;
                            for (int k = 0; k < 4; ++k)
                            {
                                char h = s[i++];
                                v <<= 4;
                                if (h >= '0' && h <= '9') v |= unsigned(h - '0');
                                else if (h >= 'a' && h <= 'f') v |= unsigned(h - 'a' + 10);
                                else if (h >= 'A' && h <= 'F') v |= unsigned(h - 'A' + 10);
                                else return false;
                            }
                            if (v < 0x80) out += char(v);
                            else if (v < 0x800) { out += char(0xC0 | (v >> 6)); out += char(0x80 | (v & 0x3F)); }
                            else { out += char(0xE0 | (v >> 12)); out += char(0x80 | ((v >> 6) & 0x3F)); out += char(0x80 | (v & 0x3F)); }
                            break;
                        }
                        default: return false;
                        }
                    }
                    else if (static_cast<unsigned char>(c) < 0x20) return false;
                    else out += c;
                }
                return false;
            }
            bool skipValue(int depth = 0)
            {
                ws();
                if (depth > 64 || i >= s.size()) return false;
                char c = s[i];
                if (c == '"') { std::string tmp; return string(tmp); }
                if (c == '{')
                {
                    ++i;
                    if (take('}')) return true;
                    do
                    {
                        std::string key;
                        if (!string(key) || !take(':') || !skipValue(depth + 1)) return false;
                    } while (take(','));
                    return take('}');
                }
                if (c == '[')
                {
                    ++i;
                    if (take(']')) return true;
                    do { if (!skipValue(depth + 1)) return false; } while (take(','));
                    return take(']');
                }
                if (s.compare(i, 4, "true") == 0) { i += 4; return true; }
                if (s.compare(i, 5, "false") == 0) { i += 5; return true; }
                if (s.compare(i, 4, "null") == 0) { i += 4; return true; }
                if (c == '-' || (c >= '0' && c <= '9'))
                {
                    ++i;
                    while (i < s.size() && (std::isdigit(static_cast<unsigned char>(s[i])) || s[i] == '.' || s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-')) ++i;
                    return true;
                }
                return false;
            }
        };

        /** Top-level members that are strings or integers, as text. */
        bool readTopLevel(std::string const& text, std::map<std::string, std::string>& out)
        {
            Json j{text};
            if (!j.take('{')) return false;
            if (j.take('}')) { j.ws(); return j.i == text.size(); }
            do
            {
                std::string key;
                if (!j.string(key) || !j.take(':')) return false;
                j.ws();
                if (j.i < text.size() && text[j.i] == '"')
                {
                    std::string v;
                    if (!j.string(v)) return false;
                    out[key] = v;
                }
                else if (j.i < text.size() && (text[j.i] == '-' || std::isdigit(static_cast<unsigned char>(text[j.i]))))
                {
                    std::size_t start = j.i;
                    if (!j.skipValue()) return false;
                    out[key] = text.substr(start, j.i - start);
                }
                else if (!j.skipValue()) return false;
            } while (j.take(','));
            if (!j.take('}')) return false;
            j.ws();
            return j.i == text.size();
        }

        /** UTF-8 text to a path without the deprecated u8path. */
        fs::path toPath(std::string const& s) { return fs::path(std::u8string(s.begin(), s.end())); }

        bool hasDotDot(fs::path const& p)
        {
            for (auto const& part : p)
                if (part == "..") return true;
            return false;
        }
    } // namespace

    std::optional<std::string> resolveUnderRoot(std::string const& relative, std::string const& root,
                                                std::vector<std::string>& problems)
    {
        if (relative.empty())
        {
            problems.push_back("the pack path is empty");
            return std::nullopt;
        }
        fs::path rel = toPath(relative);
        if (rel.is_absolute() || hasDotDot(rel) || relative.find(':') != std::string::npos)
        {
            problems.push_back("the pack path must be relative to the server root and contain no '..'");
            return std::nullopt;
        }
        std::error_code ec;
        fs::path base = root.empty() ? fs::current_path(ec) : toPath(root);
        if (ec)
        {
            problems.push_back("the server root cannot be determined");
            return std::nullopt;
        }
        fs::path full = fs::weakly_canonical(base / rel, ec);
        fs::path canonBase = fs::weakly_canonical(base, ec);
        if (ec)
        {
            problems.push_back("the pack path cannot be resolved");
            return std::nullopt;
        }
        auto fullStr = full.generic_u8string();
        auto baseStr = canonBase.generic_u8string();
        if (fullStr.size() < baseStr.size() || fullStr.compare(0, baseStr.size(), baseStr) != 0)
        {
            problems.push_back("the pack path leaves the server root");
            return std::nullopt;
        }
        // Paths travel as UTF-8 bytes in a std::string and are turned back into a
        // filesystem path with toPath wherever a file is opened.
        return std::string(fullStr.begin(), fullStr.end());
    }

    std::optional<PackLocation> locatePack(std::string const& configRelative, std::string const& root,
                                           PackStatus& status, std::vector<std::string>& problems)
    {
        auto configPath = resolveUnderRoot(configRelative, root, problems);
        if (!configPath)
        {
            status = PackStatus::BadPath;
            return std::nullopt;
        }
        std::ifstream in(toPath(*configPath), std::ios::binary);
        if (!in)
        {
            status = PackStatus::ConfigUnreadable;
            problems.push_back("the pack config cannot be opened");
            return std::nullopt;
        }
        std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        if (text.size() > (1u << 20))
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config is larger than 1 MiB");
            return std::nullopt;
        }
        std::map<std::string, std::string> kv;
        if (!readTopLevel(text, kv))
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config is not a JSON object");
            return std::nullopt;
        }
        auto get = [&](char const* key) -> std::optional<std::string>
        {
            auto it = kv.find(key);
            if (it == kv.end()) return std::nullopt;
            return it->second;
        };
        auto version = get("pier_terrain");
        if (!version || *version != "1")
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config has no pier_terrain: 1 key");
            return std::nullopt;
        }
        auto type = get("type");
        if (!type || (*type != "template" && *type != "volume"))
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config type must be template or volume");
            return std::nullopt;
        }
        auto binary = get("binary");
        if (!binary || binary->empty())
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config names no binary");
            return std::nullopt;
        }
        auto sha = get("sha256");
        if (!sha || sha->size() != 64)
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config sha256 is not 64 hex digits");
            return std::nullopt;
        }
        Sha256 parsed{};
        if (!parseHex(*sha, parsed))
        {
            status = PackStatus::ConfigInvalid;
            problems.push_back("the pack config sha256 is not hex");
            return std::nullopt;
        }
        // The binary is named relative to the config's directory, and it is subject to
        // the same root policy: the config's own directory is inside the root, so a
        // binary path with ".." is the only way out and is refused.
        fs::path bin = toPath(*binary);
        if (bin.is_absolute() || hasDotDot(bin) || binary->find(':') != std::string::npos)
        {
            status = PackStatus::BadPath;
            problems.push_back("the binary path in the pack config must be relative and contain no '..'");
            return std::nullopt;
        }
        PackLocation loc;
        loc.configPath = *configPath;
        auto binPath = (toPath(*configPath).parent_path() / bin).generic_u8string();
        loc.binaryPath = std::string(binPath.begin(), binPath.end());
        loc.kind = *type;
        loc.sha256 = *sha;
        for (auto& c : loc.sha256) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        status = PackStatus::Ok;
        return loc;
    }

    std::optional<PackFile> loadLocated(PackLocation const& loc, PackStatus& status, std::vector<std::string>& problems)
    {
        std::vector<std::string> local;
        auto file = PackFile::load(loc.binaryPath, local);
        if (!file)
        {
            bool unreadable = !local.empty() && local.front().find("cannot be") != std::string::npos;
            status = unreadable ? PackStatus::BinaryUnreadable : PackStatus::Corrupt;
            for (auto& p : local) problems.push_back(std::move(p));
            return std::nullopt;
        }
        if (hex(file->fileHash()) != loc.sha256)
        {
            status = PackStatus::HashMismatch;
            problems.push_back("the binary does not hash to the sha256 in the pack config");
            return std::nullopt;
        }
        bool isTemplate = file->kind() == PackKind::Template;
        if ((loc.kind == "template") != isTemplate)
        {
            status = PackStatus::KindMismatch;
            problems.push_back("the pack config says " + loc.kind + " but the binary is a " + (isTemplate ? "template" : "volume") + " pack");
            return std::nullopt;
        }
        status = PackStatus::Ok;
        return file;
    }

    PackCache& PackCache::instance()
    {
        static PackCache cache;
        return cache;
    }

    std::shared_ptr<TemplatePack const> PackCache::templateAt(std::string const& configRelative, std::string const& expectedSha256,
                                                              std::string const& root, PackStatus& status,
                                                              std::vector<std::string>& problems)
    {
        std::lock_guard lock(mMutex);
        for (auto const& e : mTemplates)
            if (e.path == configRelative && e.sha256 == expectedSha256) { status = PackStatus::Ok; return e.pack; }
        auto loc = locatePack(configRelative, root, status, problems);
        if (!loc) return nullptr;
        if (loc->sha256 != expectedSha256)
        {
            status = PackStatus::StoredMismatch;
            problems.push_back("the pack config now names a different binary hash than the one this dimension was created with");
            return nullptr;
        }
        if (loc->kind != "template")
        {
            status = PackStatus::KindMismatch;
            problems.push_back("the pack config is not a template pack");
            return nullptr;
        }
        auto file = loadLocated(*loc, status, problems);
        if (!file) return nullptr;
        auto decoded = decodeTemplate(*file, problems);
        if (!decoded)
        {
            status = PackStatus::Corrupt;
            return nullptr;
        }
        auto pack = std::make_shared<TemplatePack const>(std::move(*decoded));
        mTemplates.push_back({configRelative, expectedSha256, pack});
        status = PackStatus::Ok;
        return pack;
    }

    std::shared_ptr<VolumePack const> PackCache::volumeAt(std::string const& configRelative, std::string const& expectedSha256,
                                                          std::string const& root, PackStatus& status,
                                                          std::vector<std::string>& problems)
    {
        std::lock_guard lock(mMutex);
        for (auto const& e : mVolumes)
            if (e.path == configRelative && e.sha256 == expectedSha256) { status = PackStatus::Ok; return e.pack; }
        auto loc = locatePack(configRelative, root, status, problems);
        if (!loc) return nullptr;
        if (loc->sha256 != expectedSha256)
        {
            status = PackStatus::StoredMismatch;
            problems.push_back("the pack config now names a different binary hash than the one this dimension was created with");
            return nullptr;
        }
        if (loc->kind != "volume")
        {
            status = PackStatus::KindMismatch;
            problems.push_back("the pack config is not a volume pack");
            return nullptr;
        }
        auto file = loadLocated(*loc, status, problems);
        if (!file) return nullptr;
        auto decoded = decodeVolume(*file, problems);
        if (!decoded)
        {
            status = PackStatus::Corrupt;
            return nullptr;
        }
        auto pack = std::make_shared<VolumePack const>(std::move(*decoded));
        mVolumes.push_back({configRelative, expectedSha256, pack});
        status = PackStatus::Ok;
        return pack;
    }
} // namespace pier::dimensions::pack
