// Copyright 2024 MyProjectTSec

#include "GridSystem/TileContext/TileContext.h"

FTileContext::FTileContext()
{
    Initialize();
}

void FTileContext::Initialize()
{
    // Inicializar epochs a 0
    for (uint8 i = 0; i < static_cast<uint8>(EFlowIntent::MAX); ++i)
    {
        EFlowIntent Intent = static_cast<EFlowIntent>(i);
        GoalsEpochs.Add(Intent, 0);
        FlowEpochs.Add(Intent, 0);
        GoalsDirtyFlags.Add(Intent, false);
    }
}

int32 FTileContext::GetGoalsEpoch(EFlowIntent Intent) const
{
    return GoalsEpochs.FindRef(Intent);
}

int32 FTileContext::GetFlowEpoch(EFlowIntent Intent) const
{
    return FlowEpochs.FindRef(Intent);
}

void FTileContext::MarkGoalsDirty(EFlowIntent Intent)
{
    GoalsDirtyFlags[Intent] = true;
}

bool FTileContext::IsDirty() const
{
    if (bStaticCostDirty) return true;
    
    for (const auto& Pair : GoalsDirtyFlags)
    {
        if (Pair.Value) return true;
    }
    
    return false;
}

void FTileContext::UpdateEpochs()
{
    if (bStaticCostDirty)
    {
        StaticCostEpoch++;
        bStaticCostDirty = false;
    }

    for (auto& Pair : GoalsDirtyFlags)
    {
        if (Pair.Value)
        {
            GoalsEpochs[Pair.Key]++;
            FlowEpochs[Pair.Key]++;  // Invalida el FlowField correspondiente
            Pair.Value = false;
        }
    }
}
