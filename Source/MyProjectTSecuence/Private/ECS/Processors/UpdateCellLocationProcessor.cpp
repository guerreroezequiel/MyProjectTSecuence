// Copyright 2024 MyProjectTSec

#include "ECS/Processors/UpdateCellLocationProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "ECS/Processors/FlowDirReadPlayersProcessor.h"

UUpdateCellLocationProcessor::UUpdateCellLocationProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    // Debe ejecutar antes que FlowDirReadPlayersProcessor
    ExecutionOrder.ExecuteBefore.Add(UFlowDirReadPlayersProcessor::StaticClass()->GetFName());
    UE_LOG(LogTemp, Log, TEXT("UpdateCellLocationProcessor: Initialized (Phase=PrePhysics)"));
}

void UUpdateCellLocationProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FCellLocationFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
    EntityQuery.RegisterWithProcessor(*this);
}

void UUpdateCellLocationProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Context)
    {
        const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
        const TArrayView<FCellLocationFragment> CellLocationFragments = Context.GetMutableFragmentView<FCellLocationFragment>();
        
        const int32 NumEntities = Context.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i)
        {
            const FVector& Position = TransformFragments[i].GetTransform().GetLocation();
            FCellLocationFragment& CellLocation = CellLocationFragments[i];
            
            // Convert world position to cell coordinates
            const FIntPoint CellXY = GridWorld::WorldToCellXY(Position);
            const FIntPoint TileXY = GridWorld::CellToTileXY(CellXY);
            const FIntPoint LocalCellXY = GridWorld::LocalCellInTile(CellXY);
            
            // Calculate local cell index within the tile
            const int32 TileDim = GridConfig::TileDim;
            const int32 CellIndex = LocalCellXY.Y * TileDim + LocalCellXY.X;

            const bool bInsideLocal = (LocalCellXY.X >= 0 && LocalCellXY.X < TileDim && LocalCellXY.Y >= 0 && LocalCellXY.Y < TileDim);
            const bool bIndexOk = (CellIndex >= 0 && CellIndex < (TileDim * TileDim));

            if (bInsideLocal && bIndexOk)
            {
                // Solo escribir si cambió para evitar churn
                if (CellLocation.TileXY != TileXY || CellLocation.CellIndex != CellIndex || !CellLocation.bValid)
                {
                    CellLocation.TileXY = TileXY;
                    CellLocation.CellIndex = CellIndex;
                }
                CellLocation.bValid = true;
            }
            else
            {
                CellLocation.bValid = false;
            }
        }
    });
}
