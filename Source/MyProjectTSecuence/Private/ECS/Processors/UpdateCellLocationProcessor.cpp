// Copyright 2024 MyProjectTSec

#include "ECS/Processors/UpdateCellLocationProcessor.h"
#include "MassExecutionContext.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Tags/ZombiTag.h"

UUpdateCellLocationProcessor::UUpdateCellLocationProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UUpdateCellLocationProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
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
            const int32 CellIndex = LocalCellXY.Y * GridConfig::TileDim + LocalCellXY.X;
            
            // Update location fragment
            CellLocation.TileXY = TileXY;
            CellLocation.CellIndex = CellIndex;
            CellLocation.bValid = true;
        }
    });
}
