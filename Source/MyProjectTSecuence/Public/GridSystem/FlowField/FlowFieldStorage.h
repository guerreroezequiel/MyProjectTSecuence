#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/GridWorld.h"

// FlowFieldStorage: interfaz mínima de acceso a dist[] y dir[] por celda.
// Referencia: FLOWFIELD_CORE.md → API mínima
// Header-only (stub sin almacenamiento real).

namespace Grid
{
	namespace Flow
	{
		// --- Almacenamiento SOA por tile ---
		struct FTileData
		{
			TArray<float>     Dist;       // tamaño TileDim*TileDim
			TArray<FVector2D> Dir;        // tamaño TileDim*TileDim (normalizado)
			uint32            TileVersion = 0; // versión de coherencia por tile

			void InitAll()
			{
				const int32 N = GridConfig::TileDim * GridConfig::TileDim;
				Dist.Init(TNumericLimits<float>::Infinity(), N);
				Dir.Init(FVector2D::ZeroVector, N);
			}
		};

		// Mapa global de tiles -> datos (inline: instancia única compartida)
		inline TMap<FIntPoint, FTileData> GStorage;

		// Helpers de indexación
		FORCEINLINE int32 LocalIndex(const FIntPoint& LocalXY)
		{
			return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
		}

		FORCEINLINE FTileData& EnsureTile(const FIntPoint& TileXY)
		{
			FTileData* Found = GStorage.Find(TileXY);
			if (!Found)
			{
				FTileData NewTile;
				NewTile.InitAll();
				Found = &GStorage.Add(TileXY, MoveTemp(NewTile));
			}
			return *Found;
		}

		FORCEINLINE void ResetTile(const FIntPoint& TileXY)
		{
			FTileData& T = EnsureTile(TileXY);
			T.InitAll();
			T.TileVersion++;
		}

		FORCEINLINE void ClearAll()
		{
			GStorage.Reset();
		}

		FORCEINLINE void BumpTileVersion(const FIntPoint& TileXY)
		{
			FTileData& T = EnsureTile(TileXY);
			T.TileVersion++;
		}

		// --- Acceso por celda (CellXY en coords globales de celdas) ---
		FORCEINLINE void WriteDist(const FIntPoint& CellXY, const float Value)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FTileData& T = EnsureTile(TileXY);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dist.IsValidIndex(Idx)) { T.Dist[Idx] = Value; }
		}

		FORCEINLINE void WriteDir(const FIntPoint& CellXY, const FVector2D& DirValue)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			FTileData& T = EnsureTile(TileXY);
			const int32 Idx = LocalIndex(LocalXY);
			if (T.Dir.IsValidIndex(Idx)) { T.Dir[Idx] = DirValue; }
		}

		FORCEINLINE void WriteDistDir(const FIntPoint& CellXY, const float DistValue, const FVector2D& DirValue)
		{
			WriteDist(CellXY, DistValue);
			WriteDir(CellXY, DirValue);
		}

		// Lectura de distancia (cost-to-go) por celda. Default: +inf (no resuelto)
		FORCEINLINE float ReadDist(const FIntPoint& CellXY)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			if (FTileData* T = GStorage.Find(TileXY))
			{
				const int32 Idx = LocalIndex(LocalXY);
				if (T->Dist.IsValidIndex(Idx)) { return T->Dist[Idx]; }
			}
			return TNumericLimits<float>::Infinity();
		}

		// Lectura de dirección continua del flow (normalizada). Default: (0,0)
		FORCEINLINE FVector2D ReadDir(const FIntPoint& CellXY)
		{
			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
			if (FTileData* T = GStorage.Find(TileXY))
			{
				const int32 Idx = LocalIndex(LocalXY);
				if (T->Dir.IsValidIndex(Idx)) { return T->Dir[Idx]; }
			}
			return FVector2D::ZeroVector;
		}

		// Versionado por tile (para coherencia lecturas). Default: 0
		FORCEINLINE uint32 ReadTileVersion(const FIntPoint& TileXY)
		{
			if (const FTileData* T = GStorage.Find(TileXY))
			{
				return T->TileVersion;
			}
			return 0u;
		}

		// Escribe versión de tile (útil tras rebuild del tile)
		FORCEINLINE void SetTileVersion(const FIntPoint& TileXY, const uint32 NewVersion)
		{
			FTileData& T = EnsureTile(TileXY);
			T.TileVersion = NewVersion;
		}
	}
}
