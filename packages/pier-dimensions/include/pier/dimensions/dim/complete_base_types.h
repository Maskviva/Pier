#pragma once
// Supplies complete type definitions for the members that Dimension, WorldGenerator
// and FlatWorldGenerator hold as std::unique_ptr<forward-declared type>.
//
// The destructor of ll::TypedStorage really destroys the inner unique_ptr, so
// instantiating the base destructor in a derived .cpp requires those types to be
// complete, otherwise the compiler reports use of an undefined type or that it cannot
// delete an incomplete type. Every derived dimension .cpp includes this first.
#include "mc/deps/core/threading/TaskGroup.h"
#include "mc/server/commands/DelayActionList.h"
#include "mc/world/actor/ai/village/VillageManager.h"
#include "mc/world/events/BlockEventDispatcher.h"
#include "mc/world/events/gameevents/GameEventDispatcher.h"
#include "mc/world/level/RuntimeLightingManager.h"
#include "mc/world/level/Seasons.h"
#include "mc/world/level/Weather.h"
#include "mc/world/level/biome/source/FixedBiomeSource.h"
#include "mc/world/level/chunk/ChunkLoadActionList.h"
#include "mc/world/level/chunk/ChunkSource.h"
#include "mc/world/level/chunk/LevelChunkBuilderData.h"
#include "mc/world/level/chunk/PostprocessingManager.h"
#include "mc/world/level/chunk/SubChunkInterlocker.h"
#include "mc/world/level/dimension/ChunkBuildOrderPolicyBase.h"
#include "mc/world/level/dimension/DimensionBrightnessRamp.h"
#include "mc/world/level/dimension/IClientDimensionExtensions.h"
#include "mc/world/level/levelgen/structure/StructureFeatureRegistry.h"
#include "mc/world/level/poi/Manager.h"
#include "mc/world/redstone/circuit/CircuitSystem.h"
