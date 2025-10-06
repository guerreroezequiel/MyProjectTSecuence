#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"

// GridWorld (mínimo): utilidades para mapear WorldSpace <-> Grid (celdas y tiles)
// Referencia: GRID_SYSTEM.md (Capas del Grid y Posicionamiento world-space)
// Header-only para avanzar de forma incremental.

namespace GridWorld
{
	// Origen del grid en mundo (uu). Por defecto (0,0,0). Puede configurarse desde un actor/editor.
	// NOTA: Mantener en runtime como una variable global/simple mientras avanzamos.
	static FVector OriginWS = FVector::ZeroVector;

	// --- Celdas ---
	FORCEINLINE int32 WorldToCellCoord(const float WorldCoordUU, const float OriginCoordUU)
	{
		return FMath::FloorToInt((WorldCoordUU - OriginCoordUU) / GridConfig::CellSizeUU);
	}

	FORCEINLINE FIntPoint WorldToCellXY(const FVector& WorldPosUU)
	{
		const FVector2D local = FVector2D(WorldPosUU.X - OriginWS.X, WorldPosUU.Y - OriginWS.Y);
		return FIntPoint(
			FMath::FloorToInt(local.X / GridConfig::CellSizeUU),
			FMath::FloorToInt(local.Y / GridConfig::CellSizeUU)
		);
	}

	FORCEINLINE FVector2D CellToWorldCenterXY(const FIntPoint& CellXY)
	{
		return FVector2D(
			OriginWS.X + (CellXY.X + 0.5f) * GridConfig::CellSizeUU,
			OriginWS.Y + (CellXY.Y + 0.5f) * GridConfig::CellSizeUU
		);
	}

	// --- Tiles ---
	FORCEINLINE FIntPoint CellToTileXY(const FIntPoint& CellXY)
	{
		return FIntPoint(CellXY.X / GridConfig::TileDim, CellXY.Y / GridConfig::TileDim);
	}

	FORCEINLINE FIntPoint WorldToTileXY(const FVector& WorldPosUU)
	{
		return CellToTileXY(WorldToCellXY(WorldPosUU));
	}

	FORCEINLINE FIntPoint LocalCellInTile(const FIntPoint& CellXY)
	{
		return FIntPoint(
			CellXY.X - (CellXY.X / GridConfig::TileDim) * GridConfig::TileDim,
			CellXY.Y - (CellXY.Y / GridConfig::TileDim) * GridConfig::TileDim
		);
	}

	FORCEINLINE FVector2D TileToWorldOriginXY(const FIntPoint& TileXY)
	{
		return FVector2D(
			OriginWS.X + TileXY.X * GridConfig::TileDim * GridConfig::CellSizeUU,
			OriginWS.Y + TileXY.Y * GridConfig::TileDim * GridConfig::CellSizeUU
		);
	}

	// --- Sub-slots 2x2 por celda ---
	// Devuelve el índice de sub-slot (0..3) a partir de una posición mundial.
	// Asume sub-slots 2x2: (0,0)=0, (1,0)=1, (0,1)=2, (1,1)=3
	FORCEINLINE int32 SubSlotIndexFromWorld(const FVector& WorldPosUU)
	{
		const FIntPoint cell = WorldToCellXY(WorldPosUU);
		const float cellMinX = OriginWS.X + cell.X * GridConfig::CellSizeUU;
		const float cellMinY = OriginWS.Y + cell.Y * GridConfig::CellSizeUU;
		const float localX = FMath::Clamp(WorldPosUU.X - cellMinX, 0.0f, GridConfig::CellSizeUU - KINDA_SMALL_NUMBER);
		const float localY = FMath::Clamp(WorldPosUU.Y - cellMinY, 0.0f, GridConfig::CellSizeUU - KINDA_SMALL_NUMBER);

		const int32 sx = FMath::Clamp(FMath::FloorToInt(localX / GridConfig::SubSlotSizeUU), 0, GridConfig::SubSlotsDim - 1);
		const int32 sy = FMath::Clamp(FMath::FloorToInt(localY / GridConfig::SubSlotSizeUU), 0, GridConfig::SubSlotsDim - 1);
		return sx + sy * GridConfig::SubSlotsDim;
	}

	// Offset local (XY) desde el centro de la celda al centro del sub-slot indicado
	FORCEINLINE FVector2D SubSlotLocalCenterOffsetUU(const int32 SubSlotIndex)
	{
		const int32 sx = FMath::Clamp(SubSlotIndex % GridConfig::SubSlotsDim, 0, GridConfig::SubSlotsDim - 1);
		const int32 sy = FMath::Clamp(SubSlotIndex / GridConfig::SubSlotsDim, 0, GridConfig::SubSlotsDim - 1);
		return FVector2D(
			(sx + 0.5f) * GridConfig::SubSlotSizeUU - GridConfig::CellSizeUU * 0.5f,
			(sy + 0.5f) * GridConfig::SubSlotSizeUU - GridConfig::CellSizeUU * 0.5f
		);
	}

	// Centro en mundo (XY) del sub-slot
	FORCEINLINE FVector2D SubSlotWorldCenterXY(const FIntPoint& CellXY, const int32 SubSlotIndex)
	{
		const FVector2D cellCenter = CellToWorldCenterXY(CellXY);
		return cellCenter + SubSlotLocalCenterOffsetUU(SubSlotIndex);
	}
}
