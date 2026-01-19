#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"

// FlowFieldStorage (MVP):
// - Clave: (TileXY, Intent)
// - Almacena IntegrationField (Dist) y DirectionField (Dir)
// - Meta por entry: BuiltStaticCostEpoch, BuiltGoalsEpoch
// - Sin TileVersion (eliminado)

namespace Grid
{
	namespace Flow
	{
		// Entry por (TileXY, Intent)
		struct FIntentEntry
		{
			TArray<float>     Dist; // tamaño TileDim*TileDim
			TArray<FVector2D> Dir;  // tamaño TileDim*TileDim
			int32 BuiltStaticCostEpoch = -1;
			int32 BuiltGoalsEpoch = -1;

			void InitAll()
			{
				const int32 N = GridConfig::TileDim * GridConfig::TileDim;
				Dist.Init(TNumericLimits<float>::Max(), N);
				Dir.Init(FVector2D::ZeroVector, N);
			}
		};
		
		// Bucket por Tile con entradas por Intent
		struct FTileBucket
		{
			TMap<EFlowIntent, FIntentEntry> ByIntent;
		};

		// Mapa global (MVP): por TileXY
		extern TMap<FIntPoint, FTileBucket> GStorage;

		// Helpers de indexación
		int32 LocalIndex(const FIntPoint& LocalXY);

		FIntentEntry& EnsureEntry(const FIntPoint& TileXY, EFlowIntent Intent);

		void ResetTile(const FIntPoint& TileXY);

		void ClearAll();

		// --- Acceso por celda (CellXY en coords globales de celdas) ---
		void WriteDist(const FIntPoint& CellXY, EFlowIntent Intent, const float Value);

		void WriteDir(const FIntPoint& CellXY, EFlowIntent Intent, const FVector2D& DirValue);

		void WriteDistDir(const FIntPoint& CellXY, EFlowIntent Intent, const float DistValue, const FVector2D& DirValue);

		// Lectura de distancia (cost-to-go) por celda. Default: +max (no resuelto)
		float ReadDist(const FIntPoint& CellXY, EFlowIntent Intent);

		// Lectura de dirección continua del flow (normalizada). Default: (0,0)
		FVector2D ReadDir(const FIntPoint& CellXY, EFlowIntent Intent);

		// --- Meta por entry ---
		void SetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch);

		bool GetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32& OutStaticCostEpoch, int32& OutGoalsEpoch);

		bool IsValid(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch);

		// Lightweight view for Direction field and epochs per tile/intent
		struct FFieldView
		{
			const TArray<FVector2D>* DirPtr = nullptr;
			int32 StaticCostEpoch = -1;
			int32 GoalsEpoch = -1;
			int32 Epoch = -1; // Epoch combinado (criterio MVP)
			bool bValid = false;
		};

		FFieldView TryGetFieldView(const FIntPoint& TileXY, EFlowIntent Intent);

		// --- Compat con API vieja: asume Intent=Players ---
		void WriteDist(const FIntPoint& CellXY, const float Value);
		void WriteDir(const FIntPoint& CellXY, const FVector2D& DirValue);
		void WriteDistDir(const FIntPoint& CellXY, const float DistValue, const FVector2D& DirValue);
		float ReadDist(const FIntPoint& CellXY);
		FVector2D ReadDir(const FIntPoint& CellXY);
	}
}
