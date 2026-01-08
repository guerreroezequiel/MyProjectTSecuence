// Copyright 2024 MyProjectTSec

#include "GridSystem/FlowField/FlowField.h"
#include "GridSystem/TileContext/TileContext.h"

FFlowField::FFlowField()
{
    Reset();
}

FFlowField::~FFlowField()
{
    // Cleanup si es necesario
}

bool FFlowField::IsValid(const FTileContext& Context, EFlowIntent Intent) const
{
    return (BuiltStaticCostEpoch == Context.GetStaticCostEpoch()) && 
           (BuiltGoalsEpoch == Context.GetGoalsEpoch(Intent));
}

void FFlowField::Rebuild(const FTileContext& Context, EFlowIntent Intent)
{
    CurrentIntent = Intent;
    BuiltStaticCostEpoch = Context.GetStaticCostEpoch();
    BuiltGoalsEpoch = Context.GetGoalsEpoch(Intent);
    
    // TODO: Implementar lógica de reconstrucción del flowfield
}

FString FFlowField::GetDebugInfo() const
{
    return FString::Printf(TEXT("FlowField - StaticEpoch: %d, GoalsEpoch: %d, Intent: %d"), 
        BuiltStaticCostEpoch, BuiltGoalsEpoch, (int32)CurrentIntent);
}

void FFlowField::Reset()
{
    BuiltStaticCostEpoch = -1;
    BuiltGoalsEpoch = -1;
    CurrentIntent = EFlowIntent::Players;
}
