// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"

class MYPROJECTTSECUENCE_API FTileContext
{
public:
    FTileContext();

    // Epochs
    int32 GetStaticCostEpoch() const { return StaticCostEpoch; }
    int32 GetGoalsEpoch(EFlowIntent Intent) const;
    int32 GetFlowEpoch(EFlowIntent Intent) const;

    // Dirty Flags
    void MarkStaticCostDirty() { bStaticCostDirty = true; }
    void MarkGoalsDirty(EFlowIntent Intent);
    bool IsDirty() const;

    // Pipeline
    void UpdateEpochs();

private:
private:
    // Epochs
    int32 StaticCostEpoch = 0;
    TMap<EFlowIntent, int32> GoalsEpochs;
    TMap<EFlowIntent, int32> FlowEpochs;

    // Dirty Flags
    bool bStaticCostDirty = true;  // Inicialmente sucio
    TMap<EFlowIntent, bool> GoalsDirtyFlags;

    // Inicialización
    void Initialize();
};
