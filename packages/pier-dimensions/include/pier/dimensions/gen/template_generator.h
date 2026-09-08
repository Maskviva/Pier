/** template_generator.h: the chunk generator of a template-pack dimension.
 * Derives from FlatWorldGenerator for the reason the layers generator did: the flat
 * generator wires the BlockVolume prototype, the biome source and structure queries,
 * leaving loadChunk to fill the buffer. The mounted pack decides every cell through
 * generateTemplateChunk; this class only turns materials into Block pointers, once in
 * the constructor, and keeps a per-thread buffer whose static rows are filled once and
 * whose changing rows are rewritten per chunk. */
#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"

#include "pier/dimensions/pack/template_pack.h"

class Block;
class Dimension;
class LevelChunk;
namespace Json { class Value; }

namespace pier::dimensions
{
    class TemplateGenerator final : public FlatWorldGenerator
    {
        std::shared_ptr<pack::TemplatePack const> mPack;
        pack::MountedTemplate mMounted;
        std::vector<Block const*> mBlocks;
        int mDirtyFrom{0};
        int mDirtyTo{0};

        struct ThreadBuffer
        {
            std::vector<Block const*> blocks;
            std::vector<std::uint16_t> materials;
            std::optional<BlockVolume> volume;
            void const* owner{nullptr};
        };
        ThreadBuffer& acquireBuffer();
        void refillStatic(ThreadBuffer& buf);

    public:
        /** The mounted pack is moved in; mounting happened in SpecDimension where the
         *  parameters and the height come from the stored spec. The pack pointer keeps
         *  the tables the mount refers to alive. */
        TemplateGenerator(Dimension& dimension, uint seed, Json::Value const& options,
                          std::shared_ptr<pack::TemplatePack const> pack, pack::MountedTemplate mounted);
        void loadChunk(LevelChunk& lc, bool forceImmediateReplacementDataLoad) override;

        [[nodiscard]] pack::MountedTemplate const& mounted() const { return mMounted; }
    };
} // namespace pier::dimensions
