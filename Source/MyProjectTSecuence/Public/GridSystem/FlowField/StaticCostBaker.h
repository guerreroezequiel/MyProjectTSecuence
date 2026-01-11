#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/FlowField/TileStaticData.h"

// StaticCostBaker: almacenamiento y bake del buffer FinalCost_Static por tile.
// Header-only (MVP). Responsable de mantener coherencia de BakedStaticCostEpoch.

namespace Grid
{
	namespace StaticCost
	{
		inline TMap<FIntPoint, FTileStaticData> GStaticTiles;

		FORCEINLINE FTileStaticData& EnsureTile(const FIntPoint& TileXY)
		{
			FTileStaticData* Found = GStaticTiles.Find(TileXY);
			if (!Found)
			{
				FTileStaticData NewTile; NewTile.InitAll();
				Found = &GStaticTiles.Add(TileXY, MoveTemp(NewTile));
			}
			return *Found;
		}

		FORCEINLINE const FTileStaticData* FindTile(const FIntPoint& TileXY)
		{
			return GStaticTiles.Find(TileXY);
		}

		FORCEINLINE void ClearAll()
		{
			GStaticTiles.Reset();
		}

		// Bake MVP: bloqueada => INF_COST, libre => 1.0
		FORCEINLINE void BakeFinalCostStatic(const FIntPoint& TileXY, const FTileContext& Context)
		{
			FTileStaticData& StaticData = EnsureTile(TileXY);
			const int32 TileDim = GridConfig::TileDim;

			FIntPoint MinCell, MaxCell;
			GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

			for (int32 y = 0; y < TileDim; ++y)
			{
				for (int32 x = 0; x < TileDim; ++x)
				{
					const FIntPoint CellXY(MinCell.X + x, MinCell.Y + y);
					const bool bBlocked = Grid::IsBlocked(CellXY);
					const float Cost = bBlocked ? TNumericLimits<float>::Max() : 1.0f;
					StaticData.SetCost(x, y, Cost);
				}
			}

			StaticData.BakedStaticCostEpoch = Context.GetStaticCostEpoch();
		}
	}
}
