#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"

// TileStaticData: cache de costos estáticos por tile (tile-space)
// Responsable de almacenar el buffer FinalCost_Static y el epoch bakeado

struct FTileStaticData
{
	TArray<float> FinalCost_Static; // tamaño TileDim*TileDim (tile-space)
	int32 BakedStaticCostEpoch = -1;

	FORCEINLINE void InitAll()
	{
		const int32 N = GridConfig::TileDim * GridConfig::TileDim;
		FinalCost_Static.Init(TNumericLimits<float>::Max(), N);
		BakedStaticCostEpoch = -1;
	}

	FORCEINLINE static int32 LocalIndex(const int32 LocalX, const int32 LocalY)
	{
		return LocalY * GridConfig::TileDim + LocalX;
	}

	FORCEINLINE static int32 LocalIndex(const FIntPoint& LocalXY)
	{
		return LocalIndex(LocalXY.X, LocalXY.Y);
	}

	FORCEINLINE bool IsValidLocal(const int32 LocalX, const int32 LocalY) const
	{
		return LocalX >= 0 && LocalY >= 0 && LocalX < GridConfig::TileDim && LocalY < GridConfig::TileDim;
	}

	FORCEINLINE float GetCost(const int32 LocalX, const int32 LocalY) const
	{
		const int32 Idx = LocalIndex(LocalX, LocalY);
		return FinalCost_Static.IsValidIndex(Idx) ? FinalCost_Static[Idx] : TNumericLimits<float>::Max();
	}

	FORCEINLINE void SetCost(const int32 LocalX, const int32 LocalY, const float Cost)
	{
		const int32 Idx = LocalIndex(LocalX, LocalY);
		if (FinalCost_Static.IsValidIndex(Idx)) { FinalCost_Static[Idx] = Cost; }
	}
};
