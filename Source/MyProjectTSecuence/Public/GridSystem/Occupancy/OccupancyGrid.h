#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridWorld.h"

// Unified Grid System: Combina funcionalidad de estado (Occupancy) y capacidad (Capacity)
// en un solo sistema de almacenamiento por tile para mejor eficiencia.

namespace Grid
{
    // Estado de celda
    enum class ECellState : uint8
    {
        Empty = 0,  // Celda vacía
        Obstacle,   // Celda bloqueada
        Portal      // Celda especial de portal
    };

    // --- Almacenamiento unificado por tile ---
    struct FTileData
    {
        TArray<uint8> States;       // Estados de celda (ECellState)
        TArray<int16> BaseCapacity; // Capacidad base por celda
        TArray<int16> CurrentCount; // Conteo actual por celda
        
        void InitAll()
        {
            const int32 N = GridConfig::TileDim * GridConfig::TileDim;
            States.Init(static_cast<uint8>(ECellState::Empty), N);
            BaseCapacity.Init(0, N);
            CurrentCount.Init(0, N);
        }
    };

    // Almacenamiento global de tiles
    inline TMap<FIntPoint, FTileData> GGridTiles;

    // --- Gestión de Capacidad ---
    namespace Capacity
    {
        // Modo de capacidad
        enum class ECapacityMode : uint8
        {
            HardOnly = 0,     // No se permite exceso
            SoftAdaptive      // Se permite exceso suave que influye en costos
        };

        // Valores por defecto (pueden hacerse configurables)
        inline constexpr ECapacityMode Mode = ECapacityMode::SoftAdaptive;
        inline constexpr float SoftCapacityDelta = 0.5f; // Exceso relativo permitido (ej: +50%)

        // Calcula la capacidad efectiva según el modo
        FORCEINLINE int32 EffectiveCapacity(const int32 BaseCapacity)
        {
            if (Mode == ECapacityMode::SoftAdaptive)
            {
                const int32 Extra = FMath::FloorToInt(static_cast<float>(BaseCapacity) * SoftCapacityDelta);
                return BaseCapacity + FMath::Max(0, Extra);
            }
            return BaseCapacity;
        }

        // Limita un conteo a la capacidad efectiva
        FORCEINLINE int32 ClampToCapacity(const int32 CurrentCount, const int32 BaseCapacity)
        {
            return FMath::Min(CurrentCount, EffectiveCapacity(BaseCapacity));
        }

        // Calcula el factor de inflación de costo por exceso
        // overRatio = max(0, (CurrentCount - BaseCapacity) / max(1, BaseCapacity))
        // Retorna 1 si no hay exceso o si Mode == HardOnly
        FORCEINLINE float CostInflationFactor(const int32 CurrentCount, const int32 BaseCapacity)
        {
            if (Mode == ECapacityMode::HardOnly || BaseCapacity <= 0)
            {
                return 1.0f;
            }
            const float OverRatio = FMath::Max(0.0f, 
                (static_cast<float>(CurrentCount - BaseCapacity)) / 
                FMath::Max(1.0f, static_cast<float>(BaseCapacity)));
            return 1.0f + OverRatio;
        }
    }

    // --- Helpers Comunes ---
    FORCEINLINE int32 LocalIndex(const FIntPoint& LocalXY)
    {
        return LocalXY.Y * GridConfig::TileDim + LocalXY.X;
    }

    FORCEINLINE FTileData& EnsureTile(const FIntPoint& TileXY)
    {
        FTileData* Found = GGridTiles.Find(TileXY);
        if (!Found)
        {
            FTileData NewTile; 
            NewTile.InitAll();
            Found = &GGridTiles.Add(TileXY, MoveTemp(NewTile));
        }
        return *Found;
    }

    // --- API Pública ---
    
    // Limpia todos los datos de la grilla
    FORCEINLINE void ClearAll()
    {
        GGridTiles.Reset();
    }

    // --- Gestión de Estado de Celdas ---
    FORCEINLINE void SetCellState(const FIntPoint& CellXY, const ECellState NewState)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        FTileData& Tile = EnsureTile(TileXY);
        const int32 Idx = LocalIndex(LocalXY);
        if (Tile.States.IsValidIndex(Idx))
        {
            Tile.States[Idx] = static_cast<uint8>(NewState);
        }
    }

    FORCEINLINE ECellState GetCellState(const FIntPoint& CellXY)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        if (FTileData* Tile = GGridTiles.Find(TileXY))
        {
            const int32 Idx = LocalIndex(LocalXY);
            if (Tile->States.IsValidIndex(Idx))
            {
                return static_cast<ECellState>(Tile->States[Idx]);
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

    // --- Gestión de Capacidad ---
    FORCEINLINE void SetBaseCapacity(const FIntPoint& CellXY, const int32 Value)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        FTileData& Tile = EnsureTile(TileXY);
        const int32 Idx = LocalIndex(LocalXY);
        if (Tile.BaseCapacity.IsValidIndex(Idx))
        { 
            Tile.BaseCapacity[Idx] = static_cast<int16>(FMath::Clamp(Value, 0, 32767)); 
        }
    }

    FORCEINLINE int32 GetBaseCapacity(const FIntPoint& CellXY)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        if (FTileData* Tile = GGridTiles.Find(TileXY))
        {
            const int32 Idx = LocalIndex(LocalXY);
            if (Tile->BaseCapacity.IsValidIndex(Idx)) 
            { 
                return static_cast<int32>(Tile->BaseCapacity[Idx]); 
            }
        }
        return 0;
    }

    FORCEINLINE void SetCurrentCount(const FIntPoint& CellXY, const int32 Value)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        FTileData& Tile = EnsureTile(TileXY);
        const int32 Idx = LocalIndex(LocalXY);
        if (Tile.CurrentCount.IsValidIndex(Idx))
        { 
            Tile.CurrentCount[Idx] = static_cast<int16>(FMath::Clamp(Value, 0, 32767)); 
        }
    }

    FORCEINLINE int32 GetCurrentCount(const FIntPoint& CellXY)
    {
        const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
        const FIntPoint LocalXY = GridWorld::LocalCellInTile(CellXY);
        if (FTileData* Tile = GGridTiles.Find(TileXY))
        {
            const int32 Idx = LocalIndex(LocalXY);
            if (Tile->CurrentCount.IsValidIndex(Idx)) 
            { 
                return static_cast<int32>(Tile->CurrentCount[Idx]); 
            }
        }
        return 0;
    }

    // Helper para verificar si una celda está disponible (no bloqueada y dentro de capacidad)
    FORCEINLINE bool IsCellAvailable(const FIntPoint& CellXY)
    {
        if (IsBlocked(CellXY))
            return false;
            
        const int32 Current = GetCurrentCount(CellXY);
        const int32 Capacity = GetBaseCapacity(CellXY);
        return Current < Capacity::EffectiveCapacity(Capacity);
    }
}
