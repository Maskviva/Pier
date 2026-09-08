/** pack_inspect.h: the JSON a caller gets from md_pack_inspect, and the JSON of a
 * refusal. The shape is the one tools/pier-pack inspect prints, so a mod can read the
 * pack it ships with the same code it reads the host's answer. */
#pragma once

#include <string>
#include <vector>

#include "pier/dimensions/pack/pack_locate.h"
#include "pier/dimensions/pack/template_pack.h"

namespace pier::dimensions::pack
{
    /** A JSON string literal with the quotes and escapes. */
    std::string jsonString(std::string const& s);

    /** {"ok":false,"status":<code>,"problems":[...]}. */
    std::string refusalJson(PackStatus status, std::vector<std::string> const& problems);

    /** {"ok":true,"kind":"template","sha256":...,"height":{...},"params":[...],
     *  "roles":[...],"zones":[...],"constraints":[...],"shapes":n,"picks":n,
     *  "voxels":[...],"confine":bool}. */
    std::string templateJson(TemplatePack const& t, std::string const& configRelative);

    /** {"ok":true,"kind":"volume","settings":{...},"noises":[...],"functions":n,
     *  "splines":n,"router":{channel:bool},"surface_rules":n,"biomes":[...]}. */
    std::string volumeJson(VolumePack const& v, std::string const& configRelative);
} // namespace pier::dimensions::pack
