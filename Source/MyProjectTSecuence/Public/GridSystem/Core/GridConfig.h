#pragma once

#include "CoreMinimal.h"

// Config mínimos del Grid System (ver GRID_SYSTEM.md)
// Unidades: 1 m = 100 uu (Unreal Units)

namespace GridConfig
{
	// Resolución base y tiling (GRID_SYSTEM.md: Capas del Grid → Celda Base)
	inline constexpr float CellSizeUU = 100.0f;      // 1 m por celda
	inline constexpr int32  TileDim     = 64;         // 64x64 celdas por tile

	// Epoch global (GRID_SYSTEM.md: Temporización y Epoch global)
	inline constexpr float EpochMs = 200.0f;          // 5 Hz

	// Seguidores por LOD (GRID_SYSTEM.md: LOD y visibilidad)
	inline constexpr int32 FollowersLOD0 = 16;
	inline constexpr int32 FollowersLOD1 = 8;
	inline constexpr int32 FollowersLOD2 = 4;
	inline constexpr int32 FollowersOffscreen = 0;

	// Sub-slots por celda (GRID_SYSTEM.md: Capa 2 – Sub‑slots 2x2 de 50 cm)
	inline constexpr int32  SubSlotsDim    = 2;                     // 2 x 2
	inline constexpr int32  SubSlotCount   = SubSlotsDim * SubSlotsDim; // 4
	inline constexpr float  SubSlotSizeUU  = CellSizeUU / (float)SubSlotsDim; // 50 cm = 50 uu
}
