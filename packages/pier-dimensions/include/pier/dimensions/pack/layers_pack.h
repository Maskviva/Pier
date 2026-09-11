/** pack/layers_pack.h: a layers spec assembled into a template pack, in memory.
 *
 * The old inline terrain kind and the pack path describe the same thing: a stack of
 * blocks, optionally cut into a grid of cells and gaps. Rather than serve the inline
 * kind with a second generator, it is turned into the pack the tool would have built
 * and handed to the same code, so the chunk fill, the layout expansion and the
 * confinement hook underneath are the ones the pack path uses and the ones the tests
 * cover. `tools/pier-pack from-layers` performs the identical mapping offline, and
 * `equivalent_to_the_tool` holds the two together.
 *
 * The inline spec names concrete numbers, so every expression the pack carries is a
 * constant and the parameter table is empty. That is the whole reason this can be built
 * at registration time without the tool: nothing has to be bound, and nothing can fail
 * to bind.
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "pier/dimensions/pack/template_pack.h"
#include "pier/dimensions/spec/dimension_spec.h"

namespace pier::dimensions::pack
{
    /** The template pack a layers spec describes, or null with the reasons.
     *
     * Refuses rather than corrects: a grid whose border does not fit inside its cell,
     * or a layer stack that runs past the top of the world, is a recipe whose author
     * meant something this cannot deliver. Silently shrinking it would give them a
     * world that is not the one they wrote down.
     */
    std::shared_ptr<TemplatePack const> buildFromLayers(spec::Layers const& spec, int minY, int maxY,
                                                        std::vector<std::string>& problems);
} // namespace pier::dimensions::pack
