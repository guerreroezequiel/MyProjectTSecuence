// Copyright 2024 MyProjectTSec

#include "GridSystem/FlowField/FlowField.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "GridSystem/FlowField/FlowFieldSolver.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/FlowField/StaticCostBaker.h"
#include "GridSystem/FlowField/TileStaticData.h"
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
    const FIntPoint TileXY = Context.GetTileXY();
    return Grid::Flow::IsValid(
        TileXY,
        Intent,
        Context.GetStaticCostEpoch(),
        Context.GetGoalsEpoch(Intent)
    );
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
        // Sin metas: marcar como construido con los epochs actuales para evitar loops de rebuild
        Grid::Flow::SetBuiltMeta(Context.GetTileXY(), Intent, Context.GetStaticCostEpoch(), Context.GetGoalsEpoch(Intent));
        Context.IncrementFlowEpoch(Intent);
        return;
    }

    // Resolver en el rectángulo de celdas del tile
    const FIntPoint TileXY = Context.GetTileXY();

    // Coherencia bake -> solve: el epoch bakeado debe coincidir con el del TileContext
    if (const FTileStaticData* Static = Grid::StaticCost::FindTile(TileXY))
    {
        ensureMsgf(
            Static->BakedStaticCostEpoch == Context.GetStaticCostEpoch(),
            TEXT("StaticCost bake epoch mismatch before solve: Baked=%d, Context=%d for Tile(%d,%d)"),
            Static->BakedStaticCostEpoch,
            Context.GetStaticCostEpoch(),
            TileXY.X,
            TileXY.Y
        );
    }
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    Grid::Flow::FSolverParams Params; // defaults
    Grid::Flow::SolveTileDijkstraWithIntent(MinCell, MaxCell, Goals, Params, Intent);

    // Actualizar meta en storage tras el solve
    Grid::Flow::SetBuiltMeta(Context.GetTileXY(), Intent, Context.GetStaticCostEpoch(), Context.GetGoalsEpoch(Intent));

    // Incrementar FlowEpoch solo en rebuild real (telemetría/trazabilidad)
    Context.IncrementFlowEpoch(Intent);
}

FString FFlowField::GetDebugInfo(const FIntPoint& TileXY, EFlowIntent Intent) const
{
    int32 S=-1, G=-1;
    const bool ok = Grid::Flow::GetBuiltMeta(TileXY, Intent, S, G);
    return FString::Printf(TEXT("FlowField[%d] Tile(%d,%d) - BuiltStatic:%d BuiltGoals:%d%s"), 
        (int32)Intent, TileXY.X, TileXY.Y, S, G, ok?TEXT(""):TEXT(" (no data)"));
}

void FFlowField::Reset()
{
    CurrentIntent = EFlowIntent::Players;
}
