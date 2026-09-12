/**
 * SpecDimension.cpp: sky, height and generator choice, all read from the stored spec.
 *
 * What is read here is the payload already stored in dimension_config.json and not the
 * argument of the call, because generateNewData runs once when the dimension is first
 * created. A dimension built from a spec stays that spec across restarts; a pack edit on
 * the mod side does not reach it, by design: the stored hash pins the binary.
 *
 * The three structure symbols in the native branch are resolved lazily and not at load:
 * resolving them eagerly pulls in the structure registry before the engine has built it.
 */
#include "pier/dimensions/dim/complete_base_types.h"

#include "pier/dimensions/spec/spec_dimension.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "magic_enum.hpp"

#include "ll/api/memory/Memory.h"

#include "mc/common/Brightness.h"
#include "mc/deps/core/math/Color.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/DimensionConversionData.h"
#include "mc/world/level/Level.h"
#include "mc/world/level/LevelSeed64.h"
#include "mc/world/level/biome/registry/BiomeRegistry.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/dimension/DimensionArguments.h"
#include "mc/world/level/dimension/NetherBrightnessRamp.h"
#include "mc/world/level/dimension/OverworldBrightnessRamp.h"
#include "mc/world/level/dimension/VanillaDimensions.h"
#include "mc/world/level/levelgen/VoidGenerator.h"
#include "mc/world/level/levelgen/flat/FlatWorldGenerator.h"
#include "mc/world/level/levelgen/structure/EndCityFeature.h"
#include "mc/world/level/levelgen/structure/StructureFeatureRegistry.h"
#include "mc/world/level/levelgen/v1/NetherGenerator.h"
#include "mc/world/level/levelgen/v1/OverworldGeneratorMultinoise.h"
#include "mc/world/level/levelgen/v1/TheEndGenerator.h"
#include "mc/world/level/levelgen/v2/ChunkGeneratorStructureState.h"
#include "mc/world/level/storage/Experiments.h"
#include "mc/world/level/storage/LevelData.h"

#include "pier/dimensions/base/utils.h"
#include "pier/dimensions/dim/custom_dimension_manager.h"
#include "pier/dimensions/gen/supplied_generator.h"
#include "pier/dimensions/gen/template_generator.h"
#include "pier/dimensions/gen/volume_generator.h"
#include "pier/dimensions/pack/layers_pack.h"
#include "pier/dimensions/pack/pack_locate.h"
#include "pier/support/i18n.h"
#include "pier/support/log.h"

namespace pier::dimensions
{
    namespace
    {
        using ::pier::hostLogger;
        using namespace ll::memory_literals;

        void* overworldAddress()
        {
            static void* p = "`anonymous namespace'::OverworldDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }
        void* netherAddress()
        {
            static void* p = "`anonymous namespace'::NetherDimensionAnon::addStructureFeatures"_sym.resolve(true);
            return p;
        }
        void* endcityAddress()
        {
            static void* p =
                "??$addStructureFeature@VEndCityFeature@@AEAVDimension@@AEAI@StructureFeatureRegistry@@QEAAAEAVEndCityFeature@@AEAVDimension@@AEAI@Z"_sym
                    .resolve(true);
            return p;
        }

        bool overworldAddStructureFeatures(StructureFeatureRegistry& registry, uint seed, bool isLegacy, BaseGameVersion const& v)
        {
            auto* addr = overworldAddress();
            if (!addr) { hostLogger().warn("[dim] {}", pier::trf("dim.structures.symbol_missing", "OverworldDimensionAnon::addStructureFeatures", "overworld")); return false; }
            ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, bool, BaseGameVersion const&>(addr, registry, seed, isLegacy, v);
            return true;
        }
        bool netherAddStructureFeatures(StructureFeatureRegistry& registry, uint seed, BaseGameVersion const& v, Experiments const& e)
        {
            auto* addr = netherAddress();
            if (!addr) { hostLogger().warn("[dim] {}", pier::trf("dim.structures.symbol_missing", "NetherDimensionAnon::addStructureFeatures", "nether")); return false; }
            ll::memory::addressCall<void*, StructureFeatureRegistry&, uint, BaseGameVersion const&, Experiments const&>(addr, registry, seed, v, e);
            return true;
        }
        bool createEndCityFeature(StructureFeatureRegistry* self, Dimension& dimension, uint& seed)
        {
            auto* addr = endcityAddress();
            if (!addr) { hostLogger().warn("[dim] StructureFeatureRegistry::addStructureFeature<EndCityFeature> not found; custom end dimensions generate no end cities"); return false; }
            ll::memory::addressCall<EndCityFeature&, StructureFeatureRegistry*, Dimension&, uint&>(addr, self, dimension, seed);
            return true;
        }

        /** The structure state a generator is given, and why it depends on whether the
         *  features went in.
         *
         *  createNormal fills mPossibleStructures from the level's structure sets, and
         *  placing one of them means looking the matching feature up in the generator's
         *  own StructureFeatureRegistry. That registry is filled by three engine
         *  functions this host reaches by symbol, and on 26.40 none of the three is in
         *  the binary, so createNormal would leave the generator holding every structure
         *  set in the game with an empty registry behind them. With no features, the
         *  honest state is no possible structures, which is what createFlat with an empty
         *  list says. Terrain is unaffected; what is missing is villages and the rest. */
        void setStructureState(
            Dimension& dim, WorldGenerator& gen, uint seed,
            br::worldgen::StructureSetRegistry const& sets, bool featuresAdded
        )
        {
            if (featuresAdded)
            {
                gen.mStructureFeatureRegistry->mGeneratorState =
                    br::worldgen::ChunkGeneratorStructureState::createNormal(seed, gen.getBiomeSource(), sets);
                return;
            }
            hostLogger().warn("[dim] {}", pier::trf("dim.structures.none", dim.mName.get()));
            gen.mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen.getBiomeSource(), {});
        }

        /** The engine's void generator with a fixed biome, plains when the named one is
         *  missing so that a void never fails to build. */
        std::unique_ptr<WorldGenerator> voidWith(Dimension& dim, std::string const& biome)
        {
            auto v = std::make_unique<VoidGenerator>(dim);
            v->mBiome = dim.mLevel.getBiomeRegistry().lookupByName(biome);
            if (!v->mBiome) v->mBiome = dim.mLevel.getBiomeRegistry().lookupByName("minecraft:plains");
            if (v->mBiome) v->mBiomeSource = std::make_unique<FixedBiomeSource>(*v->mBiome);
            return v;
        }

        /** The biome an engine generator is handed.
         *
         *  The level's biome override is what vanilla passes, and on a normal level it is
         *  empty, so the lookup answers with nothing. OverworldGeneratorMultinoise builds
         *  its own biome source and does not care; TheEndGenerator and NetherGenerator are
         *  each written for one biome and keep what they are given. A live server generating
         *  end terrain read a pointer of all ones inside the column loop, which is what a
         *  biome that was never there looks like once something walks it, so the generator's
         *  own biome is named here when the level does not name one.
         *
         *  Empty when neither is in the registry, which the caller must treat as a refusal:
         *  building the generator anyway is the crash this avoids. */
        Biome* generatorBiome(Dimension& dim, GeneratorType gen, std::string const& overrideName)
        {
            auto& registry = dim.mLevel.getBiomeRegistry();
            if (!overrideName.empty())
            {
                if (auto* b = registry.lookupByName(overrideName)) return b;
            }
            char const* fallback = nullptr;
            switch (gen)
            {
            case GeneratorType::TheEnd:
                fallback = "minecraft:the_end";
                break;
            case GeneratorType::Nether:
                fallback = "minecraft:hell";
                break;
            default:
                return nullptr; // the overworld generators bring their own biome source
            }
            if (auto* b = registry.lookupByName(fallback)) return b;
            // Named without a namespace on some builds.
            std::string const qualified{fallback};
            std::string const bare = qualified.substr(qualified.find(':') + 1);
            return registry.lookupByName(bare);
        }

        spec::DimensionSpec specOf(std::string const& name, CompoundTag const& stored)
        {
            auto [s, problems] = spec::DimensionSpec::fromNbt(stored);
            for (auto const& p : problems) hostLogger().warn("[dim] '{}': {}", name, p);
            if (!s)
            {
                // Refusing here would fastfail on a chunk thread. The dimension is already
                // registered by id; the least harmful shape is a void with the stored seed.
                hostLogger().error("[dim] '{}': the stored spec could not be read, generating a void. The terrain differs from what was chosen at creation; fix dimension_config.json and restart", name);
                spec::DimensionSpec v;
                v.terrain = spec::Native{GeneratorType::Void};
                return v;
            }
            return *s;
        }
    } // namespace

    SpecDimension::SpecDimension(std::string const& name, DimensionFactoryInfo const& info)
        : Dimension(DimensionArguments(std::move(info.arguments), info.dimId, spec::dimensionHeightOf(info.data), name, name))
        , mSpec(specOf(name, info.data))
        , mHeight(spec::dimensionHeightOf(info.data))
    {
        mDefaultBrightness->sky = Brightness::MAX();
        mHasWeather = mSpec.sky.weather;
        if (mSpec.sky.client == GeneratorType::Nether)
        {
            mSeaLevel = 32;
            mDimensionBrightnessRamp = std::make_unique<NetherBrightnessRamp>();
        }
        else
        {
            mSeaLevel = 63;
            mDimensionBrightnessRamp = std::make_unique<OverworldBrightnessRamp>();
        }
        mDimensionBrightnessRamp->buildBrightnessRamp();
    }

    CompoundTag SpecDimension::generateNewData(std::string const& verbatimSnbt)
    {
        // The parsed tag, not the caller's bytes. CustomDimensionManager writes it back
        // out with SnbtFormat::Minimize, so dimension_config.json holds the normalized
        // form and the equality it compares is structural, not byte for byte. A mod-side
        // record that keeps the caller's own spacing will not match it as bytes.
        // Slots.cpp parsed it already; a parse failure here cannot happen and is not
        // papered over with a default.
        auto tag = CompoundTag::fromSnbt(verbatimSnbt);
        if (!tag) throw std::runtime_error("the dimension spec that parsed a moment ago does not parse now");
        return *tag;
    }

    /*
     * The sky angle this dimension shows, from its own clock rather than the level's.
     *
     * sky.time belongs beside skylight and weather: all three describe what the sky does
     * and none of them is a property of the terrain. Holding the tick here rather than in
     * a mod keeps a lock on one dimension from moving anyone else's, which is what
     * set_time would do, and needs no packet hook to undo per player.
     *
     * The base implementation is what an unlocked dimension does, so a spec without the
     * field behaves exactly as before.
     */
    float SpecDimension::getTimeOfDay(int time, float a) const
    {
        return Dimension::getTimeOfDay(mSpec.sky.time ? *mSpec.sky.time : time, a);
    }

    void SpecDimension::init(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        // Skylight follows the sky, not the generator: layered terrain under an end sky is a
        // legitimate combination and it must be dark the way the end is.
        mHasSkylight = mSpec.sky.skylight;
        Dimension::init(structureSetRegistry);
        // Against the range the Dimension was constructed with, which for an engine
        // generator is the generator's own and not the spec's.
        verifyHeightRange(*this, mHeight.mMin, mHeight.mMax, "SpecDimension");
    }

    std::unique_ptr<WorldGenerator> SpecDimension::createGenerator(br::worldgen::StructureSetRegistry const& structureSetRegistry)
    {
        auto& level = mLevel;
        auto& levelData = level.getLevelData();
        auto const seed = mSpec.seed;
        std::unique_ptr<WorldGenerator> gen;

        if (auto const* n = std::get_if<spec::Native>(&mSpec.terrain))
        {
            auto biome = generatorBiome(*this, n->generator, levelData.mBiomeOverride);
            if (!biome && (n->generator == GeneratorType::TheEnd || n->generator == GeneratorType::Nether))
            {
                hostLogger().error(
                    "[dim] '{}': the {} generator needs its own biome and neither the level's "
                    "override nor the vanilla name is in the biome registry; generating a void "
                    "instead, because that generator keeps the biome it is handed and has no "
                    "other source for one",
                    mName.get(), magic_enum::enum_name(n->generator)
                );
                gen = voidWith(*this, n->biome);
                setStructureState(*this, *gen, seed, structureSetRegistry, false);
                return gen;
            }
            bool const engineTerrain = n->generator == GeneratorType::TheEnd
                                    || n->generator == GeneratorType::Nether
                                    || n->generator == GeneratorType::Overworld;
            if (engineTerrain && !n->engineTerrain)
            {
                // The sky is a separate setting and is already applied, so what this costs
                // is the blocks and nothing else. See Native::engineTerrain.
                hostLogger().warn(
                    "[dim] '{}': engine_terrain is 0b in its terrain, so the engine's {} "
                    "generator is not used and this dimension is a void under the sky it asked "
                    "for. Set that field to 1b in dimension_config.json, or drop it, for the "
                    "terrain the template chose",
                    mName.get(), magic_enum::enum_name(n->generator)
                );
                gen = voidWith(*this, n->biome);
                setStructureState(*this, *gen, seed, structureSetRegistry, false);
                return gen;
            }
            switch (n->generator)
            {
            case GeneratorType::Overworld:
                gen = std::make_unique<OverworldGeneratorMultinoise>(*this, LevelSeed64{seed}, biome);
                hostLogger().debug("[dim] '{}': overworld generator", mName.get());
                setStructureState(
                    *this, *gen, seed, structureSetRegistry,
                    overworldAddStructureFeatures(*gen->mStructureFeatureRegistry, seed, false, levelData.getBaseGameVersion())
                );
                break;
            case GeneratorType::Nether:
                gen = std::make_unique<NetherGenerator>(*this, seed, biome);
                hostLogger().debug("[dim] '{}': nether generator", mName.get());
                setStructureState(
                    *this, *gen, seed, structureSetRegistry,
                    netherAddStructureFeatures(
                        *gen->mStructureFeatureRegistry, seed, levelData.getBaseGameVersion(),
                        static_cast<Experiments&>(levelData.mExperiments.get())
                    )
                );
                break;
            case GeneratorType::TheEnd:
            {
                uint s = seed;
                gen = std::make_unique<TheEndGenerator>(*this, seed, biome);
                hostLogger().debug("[dim] {}", pier::trf("dim.terrain.end", mName.get()));
                setStructureState(
                    *this, *gen, seed, structureSetRegistry,
                    createEndCityFeature(gen->mStructureFeatureRegistry.get(), *this, s)
                );
                break;
            }
            case GeneratorType::Flat:
                gen = std::make_unique<FlatWorldGenerator>(*this, seed, levelData.mFlatWorldOptions);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                break;
            case GeneratorType::Void:
            default:
            {
                if (n->generator != GeneratorType::Void)
                    hostLogger().error("[dim] '{}' has a native generator {} with no branch; this is a bug", mName.get(), magic_enum::enum_name(n->generator));
                gen = voidWith(*this, n->biome);
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
            }
            }
            return gen;
        }

        if (mSpec.isSupplied())
        {
            // The terrain belongs to a mod. It is there when that mod registered this
            // dimension this session, and absent when the save has a dimension whose
            // mod is gone -- the chunks stay, and the spec is what the mod will
            // register against next time, so a void here loses nothing.
            if (auto terrain = suppliedTerrainOf(mName.get()))
            {
                hostLogger().debug("[dim] {}", pier::trf("dim.supplied.by", mName.get(), terrain->owner));
                gen = std::make_unique<SuppliedGenerator>(*this, seed, levelData.mFlatWorldOptions,
                                                          std::move(terrain), mHeight.mMin,
                                                          mHeight.mMax - mHeight.mMin);
                gen->mStructureFeatureRegistry->mGeneratorState =
                    br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                return gen;
            }
            hostLogger().error(
                "[dim] '{}' has no terrain this session: its spec says the terrain comes from a mod and no mod "
                "registered it. The chunks already generated stay where they are; the world is void until that "
                "mod is loaded again",
                mName.get()
            );
            // Returned here and not left to fall through. Below this point the terrain
            // is read as a Pack, and a supplied spec reaching that line throws
            // bad_variant_access on a chunk thread -- the shape of failure this file
            // has already been fixed for once.
            gen = voidWith(*this, "minecraft:plains");
            gen->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
            return gen;
        }

        if (auto const* l = std::get_if<spec::Layers>(&mSpec.terrain))
        {
            std::vector<std::string> problems;
            // Held for as long as the generator: MountedTemplate points into the pack.
            mLayersPack = pack::buildFromLayers(*l, mHeight.mMin, mHeight.mMax, problems);
            std::optional<pack::MountedTemplate> mounted;
            if (mLayersPack) mounted = pack::mountTemplate(*mLayersPack, {}, {}, mHeight.mMin, mHeight.mMax, problems);
            if (mLayersPack && mounted)
            {
                // The grid tail is its own key rather than a hardcoded ", on a grid":
                // a translation that cannot move that clause has to reorder the whole
                // sentence around it, and some languages put it first.
                hostLogger().debug(
                    "[dim] {}",
                    pier::trf("dim.terrain.layers", mName.get(), l->layers.size(),
                              l->grid ? pier::tr("dim.terrain.layers.grid") : std::string_view{})
                );
                gen = std::make_unique<TemplateGenerator>(*this, seed, levelData.mFlatWorldOptions, mLayersPack,
                                                          std::move(*mounted));
                gen->mStructureFeatureRegistry->mGeneratorState =
                    br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                return gen;
            }
            // Refusing here would fastfail on a chunk thread; the dimension is already
            // registered by id. A void with the reasons is the least harmful shape, and
            // the stored spec stays, so the terrain returns once the recipe is fixed.
            for (auto const& msg : problems) hostLogger().error("[dim] '{}': {}", mName.get(), msg);
            hostLogger().error("[dim] '{}': the layered terrain could not be built; generating a void for this session", mName.get());
            gen = voidWith(*this, l->biome);
            gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
            return gen;
        }

        auto const* pp = std::get_if<spec::Pack>(&mSpec.terrain);
        if (!pp)
        {
            // Every alternative above returns, so this is unreachable today. It is a
            // get_if and not a get because the next alternative added to the variant
            // would otherwise land here as an exception thrown on a chunk thread.
            hostLogger().error("[dim] '{}': the stored terrain is of a kind this generator does not build; generating a void", mName.get());
            gen = voidWith(*this, "minecraft:plains");
            gen->mStructureFeatureRegistry->mGeneratorState =
                br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
            return gen;
        }
        auto const& p = *pp;
        std::vector<std::string> problems;
        pack::PackStatus status = pack::PackStatus::Ok;
        if (p.isTemplate())
        {
            auto tpl = pack::PackCache::instance().templateAt(p.path, p.sha256, "", status, problems);
            std::optional<pack::MountedTemplate> mounted;
            if (tpl) mounted = pack::mountTemplate(*tpl, p.params, p.roles, mSpec.minY, mSpec.maxY, problems);
            if (tpl && mounted)
            {
                gen = std::make_unique<TemplateGenerator>(*this, seed, levelData.mFlatWorldOptions, tpl, std::move(*mounted));
                gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                return gen;
            }
        }
        else
        {
            auto vol = pack::PackCache::instance().volumeAt(p.path, p.sha256, "", status, problems);
            if (vol)
            {
                if (mSpec.minY != vol->minY() || mSpec.maxY != vol->maxY())
                    problems.push_back("the stored height does not match the pack's fixed range");
                else
                {
                    auto seeded = std::make_shared<pack::SeededVolume const>(vol, seed);
                    gen = std::make_unique<VolumeGenerator>(*this, seed, levelData.mFlatWorldOptions, std::move(seeded));
                    gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
                    return gen;
                }
            }
        }
        // Refusing here would fastfail on a chunk thread. The dimension is already
        // registered by id; a void with an error line is the least harmful shape, and
        // the stored spec stays so the terrain returns once the pack is back in place.
        for (auto const& msg : problems) hostLogger().error("[dim] '{}': pack '{}': {}", mName.get(), p.path, msg);
        hostLogger().error("[dim] '{}': the terrain pack cannot be mounted (status {}); generating a void for this session", mName.get(), static_cast<int>(status));
        gen = voidWith(*this, "minecraft:plains");
        gen->mStructureFeatureRegistry->mGeneratorState = br::worldgen::ChunkGeneratorStructureState::createFlat(seed, gen->getBiomeSource(), {});
        return gen;
    }

    /*
     * The four chunk-upgrade overrides do nothing, and that is the whole of them.
     *
     * They exist to bring a chunk written by an older BDS up to the current storage
     * version, and a dimension of this kind has no such chunk: it is created by this host
     * on the version that is running, so everything in it was written by that version.
     *
     * They used to reinterpret_cast this object to OverworldDimension and call the
     * overworld's exported thunks. A live server fastfailed on a chunk worker inside those
     * bodies a third of a second after a player entered the dimension, on a control-flow
     * guard failure: an indirect call through a value that is not a function. Whatever
     * those bodies read, it was not what this object holds there. Doing nothing is not a
     * workaround for that; it is what the correct answer was all along here.
     */
    void SpecDimension::upgradeLevelChunk(ChunkSource&, LevelChunk&, LevelChunk&) {}

    void SpecDimension::fixWallChunk(ChunkSource&, LevelChunk&) {}

    bool SpecDimension::levelChunkNeedsUpgrade(LevelChunk const&) const { return false; }

    void SpecDimension::_upgradeOldLimboEntity(CompoundTag&, ::LimboEntitiesVersion) {}

    Vec3 SpecDimension::translatePosAcrossDimension(Vec3 const& fromPos, DimensionType fromId) const
    {
        Vec3 topos;
        VanillaDimensions::convertPointBetweenDimensions(fromPos, topos, fromId, mId, mLevel.getDimensionConversionData());
        constexpr auto clampVal = 32000000.0f - 128.0f;
        topos.x = std::clamp(topos.x, -clampVal, clampVal);
        topos.z = std::clamp(topos.z, -clampVal, clampVal);
        return topos;
    }

    short SpecDimension::getCloudHeight() const { return static_cast<short>(std::min(192, mSpec.maxY - 64)); }

    std::unique_ptr<ChunkSource> SpecDimension::_wrapStorageForVersionCompatibility(std::unique_ptr<ChunkSource> cs, ::StorageVersion) { return cs; }

    mce::Color SpecDimension::getBrightnessDependentFogColor(mce::Color const& color, float brightness) const
    {
        float temp = (brightness * 0.94f) + 0.06f;
        float temp2 = (brightness * 0.91f) + 0.09f;
        auto result = color;
        result.r = color.r * temp;
        result.g = color.g * temp;
        result.b = color.b * temp2;
        return result;
    }
} // namespace pier::dimensions
