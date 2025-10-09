#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/GridWorld.h"

// Referencia: FLOWFIELD_CORE.md → Capas esenciales: Heat/Density
// Header-only con almacenamiento real SOA por tile.

namespace Grid
{
    	namespace Density
    	{
    		// Tunables por defecto para pruebas (pueden moverse a UDeveloperSettings)
    		inline float kDecayPerSecond = 0.15f;   // reducción lineal por segundo
    		inline float kMaxHeat        = 4.0f;    // clamp superior

    		// --- Almacenamiento SOA por tile ---
    		struct FTileHeat
    		{
    			TArray<float> Heat; // tamaño TileDim*TileDim
    			void InitAll()
    			{
    				const int32 N = GridConfig::TileDim * GridConfig::TileDim;
    				Heat.Init(0.0f, N);
    			}
    		};

    		inline TMap<FIntPoint, FTileHeat> GHeatTiles; // instancia única compartida

    		FORCEINLINE int32 LocalIndex(const FIntPoint& LocalXY)
    		{
    			return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
    		}

    		FORCEINLINE FTileHeat& EnsureTile(const FIntPoint& TileXY)
    		{
    			FTileHeat* Found = GHeatTiles.Find(TileXY);
    			if (!Found)
    			{
    				FTileHeat NewTile; NewTile.InitAll();
    				Found = &GHeatTiles.Add(TileXY, MoveTemp(NewTile));
    			}
    			return *Found;
    		}

    		FORCEINLINE void ClearAll()
    		{
    			GHeatTiles.Reset();
    		}

    		// Registra tráfico en una celda (valor aditivo y clamped)
    		FORCEINLINE void AccumulateTraffic(const FIntPoint& CellXY, const float Amount)
    		{
    			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
    			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
    			FTileHeat& T = EnsureTile(TileXY);
    			const int32 Idx = LocalIndex(LocalXY);
    			if (T.Heat.IsValidIndex(Idx))
    			{
    				const float NewV = FMath::Clamp(T.Heat[Idx] + Amount, 0.0f, kMaxHeat);
    				T.Heat[Idx] = NewV;
    			}
    		}

    		// Aplica decay global (dt en segundos) con clamp a [0, kMaxHeat]
    		FORCEINLINE void Decay(const float DeltaSeconds)
    		{
    			if (DeltaSeconds <= 0.0f || GHeatTiles.Num() == 0) { return; }
    			const float Dec = FMath::Max(0.0f, kDecayPerSecond * DeltaSeconds);
    			for (auto& It : GHeatTiles)
    			{
    				FTileHeat& T = It.Value;
    				for (float& v : T.Heat)
    				{
    					v = FMath::Clamp(v - Dec, 0.0f, kMaxHeat);
    				}
    			}
    		}

    		// Lee calor/heat de la celda (0 si no existe)
    		FORCEINLINE float GetHeat(const FIntPoint& CellXY)
    		{
    			const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
    			const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
    			if (FTileHeat* T = GHeatTiles.Find(TileXY))
    			{
    				const int32 Idx = LocalIndex(LocalXY);
    				if (T->Heat.IsValidIndex(Idx)) { return T->Heat[Idx]; }
    			}
    			return 0.0f;
    		}
    	}
}
