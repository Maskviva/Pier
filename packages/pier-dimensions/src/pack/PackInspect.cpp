/** PackInspect.cpp: JSON for md_pack_inspect, written by hand so the pack layer needs
 * no JSON library and the field order is fixed. */
#include "pier/dimensions/pack/pack_inspect.h"

#include <cstring>

namespace pier::dimensions::pack
{
    std::string jsonString(std::string const& s)
    {
        std::string out;
        out.reserve(s.size() + 2);
        out += '"';
        for (unsigned char c : s)
        {
            switch (c)
            {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20)
                {
                    static constexpr char digits[] = "0123456789abcdef";
                    out += "\\u00";
                    out += digits[c >> 4];
                    out += digits[c & 15];
                }
                else out += static_cast<char>(c);
            }
        }
        out += '"';
        return out;
    }

    std::string refusalJson(PackStatus status, std::vector<std::string> const& problems)
    {
        std::string out = "{\"ok\":false,\"status\":" + std::to_string(static_cast<int>(status)) + ",\"problems\":[";
        for (std::size_t i = 0; i < problems.size(); ++i)
        {
            if (i) out += ',';
            out += jsonString(problems[i]);
        }
        out += "]}";
        return out;
    }

    std::string templateJson(TemplatePack const& t, std::string const& configRelative)
    {
        std::string out = "{\"ok\":true,\"kind\":\"template\",\"pack\":" + jsonString(configRelative)
            + ",\"sha256\":" + jsonString(hex(t.fileHash)) + ",\"name\":" + jsonString(t.sourceName)
            + ",\"biome\":" + jsonString(t.biome) + ",\"height\":{\"min\":" + std::to_string(t.heightMin)
            + ",\"max\":" + std::to_string(t.heightMax) + ",\"fixed\":" + (t.heightFixed ? "true" : "false") + "}";
        out += ",\"params\":[";
        for (std::size_t i = 0; i < t.params.size(); ++i)
        {
            auto const& p = t.params[i];
            if (i) out += ',';
            out += "{\"name\":" + jsonString(p.name);
            switch (p.kind)
            {
            case ParamKind::Free:
                out += ",\"kind\":\"free\",\"default\":" + std::to_string(p.def) + ",\"min\":" + std::to_string(p.min)
                    + ",\"max\":" + std::to_string(p.max) + ",\"step\":" + std::to_string(p.step);
                break;
            case ParamKind::Fixed: out += ",\"kind\":\"fixed\",\"value\":" + std::to_string(p.def); break;
            case ParamKind::Choice:
            {
                out += ",\"kind\":\"choice\",\"default\":" + std::to_string(p.def) + ",\"choices\":[";
                auto const& lst = t.choiceLists[p.aux];
                for (std::size_t k = 0; k < lst.size(); ++k)
                {
                    if (k) out += ',';
                    out += std::to_string(lst[k]);
                }
                out += ']';
                break;
            }
            case ParamKind::Derived: out += ",\"kind\":\"derived\""; break;
            }
            out += '}';
        }
        out += "],\"roles\":[";
        for (std::size_t i = 0; i < t.roles.size(); ++i)
        {
            if (i) out += ',';
            out += "{\"name\":" + jsonString(t.roles[i].name) + ",\"default\":" + jsonString(t.roles[i].defaultBlock) + "}";
        }
        out += "],\"zones\":[";
        for (std::size_t i = 0; i < t.zoneNames.size(); ++i)
        {
            if (i) out += ',';
            out += jsonString(t.zoneNames[i]);
        }
        out += "],\"constraints\":[";
        for (std::size_t i = 0; i < t.constraintMessages.size(); ++i)
        {
            if (i) out += ',';
            out += jsonString(t.constraintMessages[i]);
        }
        out += "],\"shapes\":" + std::to_string(t.shapes.size()) + ",\"picks\":" + std::to_string(t.picks.size()) + ",\"voxels\":[";
        for (std::size_t i = 0; i < t.voxels.size(); ++i)
        {
            auto const& v = t.voxels[i];
            if (i) out += ',';
            out += "{\"size\":[" + std::to_string(v.sx) + "," + std::to_string(v.sy) + "," + std::to_string(v.sz)
                + "],\"palette\":" + std::to_string(v.palette.size()) + "}";
        }
        out += "],\"confine\":";
        out += t.confine ? "true" : "false";
        out += '}';
        return out;
    }

    std::string volumeJson(VolumePack const& v, std::string const& configRelative)
    {
        auto const& s = v.settings;
        std::string out = "{\"ok\":true,\"kind\":\"volume\",\"pack\":" + jsonString(configRelative)
            + ",\"sha256\":" + jsonString(hex(v.fileHash)) + ",\"name\":" + jsonString(v.sourceName)
            + ",\"height\":{\"min\":" + std::to_string(s.minY) + ",\"max\":" + std::to_string(s.minY + s.height) + ",\"fixed\":true}"
            + ",\"settings\":{\"min_y\":" + std::to_string(s.minY) + ",\"height\":" + std::to_string(s.height)
            + ",\"sea_level\":" + std::to_string(s.seaLevel) + ",\"default_block\":" + jsonString(v.defaultBlock)
            + ",\"default_fluid\":" + jsonString(v.defaultFluid) + ",\"size_horizontal\":" + std::to_string(s.sizeHorizontal)
            + ",\"size_vertical\":" + std::to_string(s.sizeVertical) + ",\"legacy_random_source\":"
            + ((s.flags & kSettingsLegacyRandom) ? "true" : "false") + "}";
        out += ",\"noises\":[";
        for (std::size_t i = 0; i < v.noises.size(); ++i)
        {
            if (i) out += ',';
            out += jsonString(v.noises[i].name);
        }
        out += "],\"functions\":" + std::to_string(v.funcs.size()) + ",\"splines\":" + std::to_string(v.splines.size());
        static constexpr char const* channels[] = {"final_density", "temperature", "vegetation", "continents", "erosion", "depth",
                                                   "ridges", "initial_density_without_jaggedness", "barrier", "fluid_level_floodedness",
                                                   "fluid_level_spread", "lava", "vein_toggle", "vein_ridged", "vein_gap"};
        std::uint32_t r[16];
        std::memcpy(r, &v.router, sizeof r);
        out += ",\"router\":{";
        for (int k = 0; k < 15; ++k)
        {
            if (k) out += ',';
            out += std::string("\"") + channels[k] + "\":" + (r[k] != kNone ? "true" : "false");
        }
        out += "},\"surface_rules\":" + std::to_string(v.surf.size()) + ",\"biomes\":[";
        for (std::size_t i = 0; i < v.biomeNames.size(); ++i)
        {
            if (i) out += ',';
            out += jsonString(v.biomeNames[i]);
        }
        out += "]}";
        return out;
    }
} // namespace pier::dimensions::pack
