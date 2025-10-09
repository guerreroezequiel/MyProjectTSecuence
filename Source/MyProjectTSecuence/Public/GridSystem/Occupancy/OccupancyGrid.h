#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridWorld.h"

//OccupancyGrid: estado por celda (Empty/Obstacle/Portal) con almacenamiento por tile.
// Header-only, estilo SOA por tile, para consultas O(1).

namespace Grid
{
    // Estado de celda (mínimo)
    enum class ECellState : uint8
    {
        Empty = 0,
        Obstacle,
        Portal
    };

    // --- Almacenamiento SOA por tile ---
    struct FTileOcc
    {
        TArray<uint8> States; // ECellState por celda
        void InitAll()
        {
            const int32 N = GridConfig::TileDim * GridConfig::TileDim;
            States.Init(static_cast<uint8>(ECellState::Empty), N);
        }
    };

    inline TMap<FIntPoint, FTileOcc> GOccTiles; // instancia única compartida

    FORCEINLINE int32 LocalIndex(const FIntPoint& LocalXY)
    {
        return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
    }

    FORCEINLINE FTileOcc& EnsureTile(const FIntPoint& TileXY)
    {
        FTileOcc* Found = GOccTiles.Find(TileXY);
        if (!Found)
        {
            FTileOcc NewTile; NewTile.InitAll();
            Found = &GOccTiles.Add(TileXY, MoveTemp(NewTile));
        }
        return *Found;
    }

    FORCEINLINE void ClearAll()
    {
        GOccTiles.Reset();
    }

    // API principal
    FORCEINLINE void SetCellState(const FIntPoint& CellXY, const ECellState NewState)
    {
        const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        FTileOcc& T = EnsureTile(TileXY);
        const int32 Idx = LocalIndex(LocalXY);
        if (T.States.IsValidIndex(Idx))
        {
            T.States[Idx] = static_cast<uint8>(NewState);
        }
    }

    FORCEINLINE ECellState GetCellState(const FIntPoint& CellXY)
    {
        const FIntPoint TileXY  = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        if (FTileOcc* T = GOccTiles.Find(TileXY))
        {
            const int32 Idx = LocalIndex(LocalXY);
            if (T->States.IsValidIndex(Idx))
            {
                return static_cast<ECellState>(T->States[Idx]);
            }
        }
        return ECellState::Empty;
    }

    FORCEINLINE bool IsBlocked(const FIntPoint& CellXY)
    {
        return GetCellState(CellXY) == ECellState::Obstacle;
    }

    FORCEINLINE bool IsPortal(const FIntPoint& CellXY)
    {
        return GetCellState(CellXY) == ECellState::Portal;
    }
}
