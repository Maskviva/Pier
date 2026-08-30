#pragma once
// 为 Dimension / WorldGenerator / FlatWorldGenerator 基类中以
// std::unique_ptr<前向声明类型> 持有的成员提供完整类型定义。
//
// ll::TypedStorage 的析构会真正析构内部 unique_ptr，派生类 .cpp 中实例化基类
// 析构时要求这些类型是完整的，否则报「使用了未定义类型 / can't delete an
// incomplete type」。每个派生维度的 .cpp 都要先包含这一份。
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
