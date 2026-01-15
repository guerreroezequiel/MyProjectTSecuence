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
		inline TMap<FIntPoint, FTileBucket> GStorage;

		// Helpers de indexación
		FORCEINLINE int32 LocalIndex(const FIntPoint& LocalXY)
		{
			return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
		}

		FORCEINLINE FIntentEntry& EnsureEntry(const FIntPoint& TileXY, EFlowIntent Intent)
		{
			FTileBucket& Bucket = GStorage.FindOrAdd(TileXY);
			FIntentEntry* Found = Bucket.ByIntent.Find(Intent);
			if (!Found)
			{
				FIntentEntry NewEntry; NewEntry.InitAll();
				Found = &Bucket.ByIntent.Add(Intent, MoveTemp(NewEntry));
			}
			return *Found;
		}

		FORCEINLINE void ResetTile(const FIntPoint& TileXY)
		{
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				for (auto& Pair : B->ByIntent)
				{
					Pair.Value.InitAll();
					Pair.Value.BuiltStaticCostEpoch = -1;
					Pair.Value.BuiltGoalsEpoch = -1;
				}
			}
		}

		FORCEINLINE void ClearAll()
		{
			GStorage.Reset();
		}

		// --- Acceso por celda (CellXY en coords globales de celdas) ---
		FORCEINLINE void WriteDist(const FIntPoint& CellXY, EFlowIntent Intent, const float Value)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FIntentEntry& T = EnsureEntry(TileXY, Intent);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dist.IsValidIndex(Idx)) { T.Dist[Idx] = Value; }
		}

		FORCEINLINE void WriteDir(const FIntPoint& CellXY, EFlowIntent Intent, const FVector2D& DirValue)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FIntentEntry& T = EnsureEntry(TileXY, Intent);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dir.IsValidIndex(Idx)) { T.Dir[Idx] = DirValue; }
		}

		FORCEINLINE void WriteDistDir(const FIntPoint& CellXY, EFlowIntent Intent, const float DistValue, const FVector2D& DirValue)
		{
			WriteDist(CellXY, Intent, DistValue);
			WriteDir(CellXY, Intent, DirValue);
		}

		// Lectura de distancia (cost-to-go) por celda. Default: +max (no resuelto)
		FORCEINLINE float ReadDist(const FIntPoint& CellXY, EFlowIntent Intent)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				if (FIntentEntry* T = B->ByIntent.Find(Intent))
				{
					const int32 Idx = LocalIndex(LocalXY);
					if (T->Dist.IsValidIndex(Idx)) { return T->Dist[Idx]; }
				}
			}
			return TNumericLimits<float>::Max();
		}

		// Lectura de dirección continua del flow (normalizada). Default: (0,0)
		FORCEINLINE FVector2D ReadDir(const FIntPoint& CellXY, EFlowIntent Intent)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				if (FIntentEntry* T = B->ByIntent.Find(Intent))
				{
					const int32 Idx = LocalIndex(LocalXY);
					if (T->Dir.IsValidIndex(Idx)) { return T->Dir[Idx]; }
				}
			}
			return FVector2D::ZeroVector;
		}

		// --- Meta por entry ---
		FORCEINLINE void SetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch)
		{
			FIntentEntry& E = EnsureEntry(TileXY, Intent);
			E.BuiltStaticCostEpoch = StaticCostEpoch;
			E.BuiltGoalsEpoch = GoalsEpoch;
		}

		FORCEINLINE bool GetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32& OutStaticCostEpoch, int32& OutGoalsEpoch)
		{
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				if (const FIntentEntry* E = B->ByIntent.Find(Intent))
				{
					OutStaticCostEpoch = E->BuiltStaticCostEpoch;
					OutGoalsEpoch = E->BuiltGoalsEpoch;
					return true;
				}
			}
			return false;
		}

		FORCEINLINE bool IsValid(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch)
		{
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				if (const FIntentEntry* E = B->ByIntent.Find(Intent))
				{
					return (E->BuiltStaticCostEpoch == StaticCostEpoch) && (E->BuiltGoalsEpoch == GoalsEpoch);
				}
			}
			return false;
		}

		// --- Compat con API vieja: asume Intent=Players ---
		FORCEINLINE void WriteDist(const FIntPoint& CellXY, const float Value) { WriteDist(CellXY, EFlowIntent::Players, Value); }
		FORCEINLINE void WriteDir(const FIntPoint& CellXY, const FVector2D& DirValue) { WriteDir(CellXY, EFlowIntent::Players, DirValue); }
		FORCEINLINE void WriteDistDir(const FIntPoint& CellXY, const float DistValue, const FVector2D& DirValue) { WriteDistDir(CellXY, EFlowIntent::Players, DistValue, DirValue); }
		FORCEINLINE float ReadDist(const FIntPoint& CellXY) { return ReadDist(CellXY, EFlowIntent::Players); }
		FORCEINLINE FVector2D ReadDir(const FIntPoint& CellXY) { return ReadDir(CellXY, EFlowIntent::Players); }
	}
}
