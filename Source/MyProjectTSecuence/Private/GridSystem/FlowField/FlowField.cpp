// Copyright 2024 MyProjectTSec

#include "GridSystem/FlowField/FlowField.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "GridSystem/FlowField/FlowFieldSolver.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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

void FFlowField::Rebuild(FTileContext& Context, EFlowIntent Intent)
{
    CurrentIntent = Intent;

    // Obtener metas desde el Subsystem del mundo (MVP)
    if (!GWorld)
    {
        return;
    }

    UGridEpochSubsystem* Subsystem = GWorld->GetSubsystem<UGridEpochSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    const Grid::Flow::FGoalSet& Goals = Subsystem->Goals;
    if (Goals.GoalCells.Num() == 0)
    {
        // Sin metas, no hay nada que resolver
        return;
    }

    // Resolver en el rectángulo de celdas del tile
    const FIntPoint TileXY = Context.GetTileXY();
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    Grid::Flow::FSolverParams Params; // defaults
    Grid::Flow::SolveTileDijkstra(MinCell, MaxCell, Goals, Params);

    // Actualizar epochs construidos tras el solve
    BuiltStaticCostEpoch = Context.GetStaticCostEpoch();
    BuiltGoalsEpoch = Context.GetGoalsEpoch(Intent);

    // Incrementar FlowEpoch solo en rebuild real (telemetría/trazabilidad)
    Context.IncrementFlowEpoch(Intent);
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
