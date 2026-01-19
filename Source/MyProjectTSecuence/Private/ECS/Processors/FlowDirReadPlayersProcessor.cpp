// Copyright 2024 MyProjectTSec

#include "ECS/Processors/FlowDirReadPlayersProcessor.h"
#include "ECS/Processors/MoveIntegrateProcessor.h"
#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/FlowField/FlowFieldStorageSubsystem.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Tags/ZombiTag.h"

UFlowDirReadPlayersProcessor::UFlowDirReadPlayersProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    // Debe ejecutar antes que MoveIntegrateProcessor
    ExecutionOrder.ExecuteBefore.Add(UMoveIntegrateProcessor::StaticClass()->GetFName());
    UE_LOG(LogTemp, Log, TEXT("FlowDirReadPlayersProcessor: Initialized (Phase=PrePhysics)"));
}

void UFlowDirReadPlayersProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FCellLocationFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FFlowReadFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
    EntityQuery.RegisterWithProcessor(*this);
}

void UFlowDirReadPlayersProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = EntityManager.GetWorld();
    UFlowFieldStorageSubsystem* StorageSubsystem = UFlowFieldStorageSubsystem::Get(World);

    EntityQuery.ForEachEntityChunk(EntityManager, Context, [StorageSubsystem](FMassExecutionContext& Context)
    {
        const TConstArrayView<FCellLocationFragment> CellLocationFragments = Context.GetFragmentView<FCellLocationFragment>();
        const TArrayView<FFlowReadFragment> FlowReadFragments = Context.GetMutableFragmentView<FFlowReadFragment>();
        
        const int32 NumEntities = Context.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i)
        {
            const FCellLocationFragment& CellLocation = CellLocationFragments[i];
            FFlowReadFragment& FlowRead = FlowReadFragments[i];

            // Inicializar como no válido por defecto
            FlowRead.bValid = false;

            if (CellLocation.bValid)
            {
                // Obtener una vista del campo por Tile e Intent via Subsystem
                Grid::Flow::FFieldView View;
                if (StorageSubsystem)
                {
                    View = StorageSubsystem->TryGetFieldView(CellLocation.TileXY, EFlowIntent::Players);
                }
                if (View.bValid && View.DirPtr)
                {
                    const TArray<FVector2D>& DirField = *View.DirPtr;
                    const int32 Idx = CellLocation.CellIndex;
                    if (DirField.IsValidIndex(Idx))
                    {
                        const FVector2D FlowDir = DirField[Idx];
                        if (!FlowDir.IsNearlyZero())
                        {
                            FlowRead.DirWS = FVector(FlowDir.X, FlowDir.Y, 0.0f).GetSafeNormal();
                            FlowRead.EpochSeen = View.Epoch; // usar epoch combinado del storage
                            FlowRead.bValid = true;
                        }
                    }
                }
            }
        }
    });
}
