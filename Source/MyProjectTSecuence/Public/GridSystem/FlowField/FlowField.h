// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"

// Forward declaration
class FTileContext;

class MYPROJECTTSECUENCE_API FFlowField
{
public:
    FFlowField();
    ~FFlowField();

    // Validación
    bool IsValid(const FTileContext& Context, EFlowIntent Intent) const;
    
    // Reconstrucción
    void Rebuild(FTileContext& Context, EFlowIntent Intent);

    // Debug
    FString GetDebugInfo(const FIntPoint& TileXY, EFlowIntent Intent) const;

private:
    EFlowIntent CurrentIntent = EFlowIntent::Players;

    // Métodos auxiliares
    void Reset();
};
