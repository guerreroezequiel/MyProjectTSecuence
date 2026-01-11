#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/GridEpoch.h"
#include "GridSystem/FlowField/FlowFieldSolver.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/TileRegistry.h"

// FlowFieldRebuilder: gestiona dirty tiles y presupuesto por frame.
// Referencia: FLOWFIELD_CORE.md → Rebuild parcial
// Header-only stub (sin cola real aún).

namespace Grid
{
	namespace Flow
	{
		struct FRebuildBudget
		{
			int32 MaxTilesPerFrame = 4;
			int32 MaxCellsPerFrame = 4096;
		};

		// --- Estructuras de dirty ---
		inline TSet<FIntPoint>  GDirtyTiles;  // set para evitar duplicados
		inline TQueue<FIntPoint> GDirtyQueue; // cola FIFO de tiles sucios

		// Marca un tile como sucio
		FORCEINLINE void MarkTileDirty(const FIntPoint& TileXY)
		{
			// Asegurar que exista el TileContext con su TileXY seteado
			Grid::Tiles::EnsureTileContext(TileXY);

			if (!GDirtyTiles.Contains(TileXY))
			{
				GDirtyTiles.Add(TileXY);
				GDirtyQueue.Enqueue(TileXY);
			}
		}

		// Marca un área rectangular de tiles como sucia (incluyente)
		FORCEINLINE void MarkTilesDirty(const FIntPoint& MinTileXY, const FIntPoint& MaxTileXY)
		{
			for (int32 ty = MinTileXY.Y; ty <= MaxTileXY.Y; ++ty)
			{
				for (int32 tx = MinTileXY.X; tx <= MaxTileXY.X; ++tx)
				{
					MarkTileDirty(FIntPoint(tx, ty));
				}
			}
		}

		// Ejecuta un paso de rebuild con presupuesto; resuelve cada tile y actualiza versión
		FORCEINLINE void RebuildStep(const FRebuildBudget& Budget, const FGoalSet& Goals, const FSolverParams& Params)
		{
			int32 ProcessedTiles = 0;
			FIntPoint Tile;
			while (ProcessedTiles < Budget.MaxTilesPerFrame && GDirtyQueue.Dequeue(Tile))
			{
				// Evitar reprocesar si ya fue limpiado externamente
				if (!GDirtyTiles.Contains(Tile)) { continue; }

				// Bounds de celdas para este tile
				FIntPoint MinCell, MaxCell;
				GridWorld::TileBoundsInCells(Tile, MinCell, MaxCell);

				// Resolver dist/dir en este tile con Dijkstra multi-fuente (8-dir)
				SolveTileDijkstra(MinCell, MaxCell, Goals, Params);

				// Versionar tile tras escribir
				BumpTileVersion(Tile);

				// Marcar como limpio
				GDirtyTiles.Remove(Tile);
				ProcessedTiles++;
			}
		}
	}
}
