#include "GridSystem/FlowField/FlowFieldStorage.h"

namespace Grid
{
	namespace Flow
	{
		// Definición del mapa global
		TMap<FIntPoint, FTileBucket> GStorage;

		int32 LocalIndex(const FIntPoint& LocalXY)
		{
			return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
		}

		FIntentEntry& EnsureEntry(const FIntPoint& TileXY, EFlowIntent Intent)
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

		void ResetTile(const FIntPoint& TileXY)
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

		void ClearAll()
		{
			GStorage.Reset();
		}

		void WriteDist(const FIntPoint& CellXY, EFlowIntent Intent, const float Value)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FIntentEntry& T = EnsureEntry(TileXY, Intent);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dist.IsValidIndex(Idx)) { T.Dist[Idx] = Value; }
		}

		void WriteDir(const FIntPoint& CellXY, EFlowIntent Intent, const FVector2D& DirValue)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FIntentEntry& T = EnsureEntry(TileXY, Intent);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dir.IsValidIndex(Idx)) { T.Dir[Idx] = DirValue; }
		}

		void WriteDistDir(const FIntPoint& CellXY, EFlowIntent Intent, const float DistValue, const FVector2D& DirValue)
		{
			WriteDist(CellXY, Intent, DistValue);
			WriteDir(CellXY, Intent, DirValue);
		}

		float ReadDist(const FIntPoint& CellXY, EFlowIntent Intent)
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

		FVector2D ReadDir(const FIntPoint& CellXY, EFlowIntent Intent)
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

		void SetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch)
		{
			FIntentEntry& E = EnsureEntry(TileXY, Intent);
			E.BuiltStaticCostEpoch = StaticCostEpoch;
			E.BuiltGoalsEpoch = GoalsEpoch;
		}

		bool GetBuiltMeta(const FIntPoint& TileXY, EFlowIntent Intent, int32& OutStaticCostEpoch, int32& OutGoalsEpoch)
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

		bool IsValid(const FIntPoint& TileXY, EFlowIntent Intent, int32 StaticCostEpoch, int32 GoalsEpoch)
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

		FFieldView TryGetFieldView(const FIntPoint& TileXY, EFlowIntent Intent)
		{
			FFieldView View;
			if (FTileBucket* B = GStorage.Find(TileXY))
			{
				if (const FIntentEntry* E = B->ByIntent.Find(Intent))
				{
					View.DirPtr = &E->Dir;
					View.StaticCostEpoch = E->BuiltStaticCostEpoch;
					View.GoalsEpoch = E->BuiltGoalsEpoch;
					View.Epoch = FMath::Max(E->BuiltStaticCostEpoch, E->BuiltGoalsEpoch);
					const bool bEpochsReady = (View.StaticCostEpoch >= 0) && (View.GoalsEpoch >= 0);
					bool bSizeOK = true;
					if (View.DirPtr)
					{
						const int32 ExpectedCells = GridConfig::TileDim * GridConfig::TileDim;
						bSizeOK = (View.DirPtr->Num() == ExpectedCells);
					}
					View.bValid = bEpochsReady && bSizeOK;
				}
			}
			return View;
		}

		// Compat API (Intent=Players)
		void WriteDist(const FIntPoint& CellXY, const float Value) { WriteDist(CellXY, EFlowIntent::Players, Value); }
		void WriteDir(const FIntPoint& CellXY, const FVector2D& DirValue) { WriteDir(CellXY, EFlowIntent::Players, DirValue); }
		void WriteDistDir(const FIntPoint& CellXY, const float DistValue, const FVector2D& DirValue) { WriteDistDir(CellXY, EFlowIntent::Players, DistValue, DirValue); }
		float ReadDist(const FIntPoint& CellXY) { return ReadDist(CellXY, EFlowIntent::Players); }
		FVector2D ReadDir(const FIntPoint& CellXY) { return ReadDir(CellXY, EFlowIntent::Players); }
	}
}
