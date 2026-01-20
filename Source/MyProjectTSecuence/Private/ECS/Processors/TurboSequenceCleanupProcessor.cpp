#include "ECS/Processors/TurboSequenceCleanupProcessor.h"

#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
#include "ECS/TurboSequence/TurboSequenceWorldSubsystem.h"

UTurboSequenceCleanupProcessor::UTurboSequenceCleanupProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PostPhysics; // GameThread, mismo que Sync/Solve
    bRequiresGameThreadExecution = true;
    // Debe ejecutar antes que Solve para no procesar instancias que serán removidas
    ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
    UE_LOG(LogTemp, Log, TEXT("TurboSequenceCleanupProcessor: Initialized (Phase=PostPhysics)"));
}

void UTurboSequenceCleanupProcessor::ConfigureQueries()
{
    CleanupQuery.AddRequirement<FTurboSequenceInstanceFragment>(EMassFragmentAccess::ReadWrite);
    CleanupQuery.AddTagRequirement<FTSPendingCleanupTag>(EMassFragmentPresence::All);
    CleanupQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceCleanupProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = EntityManager.GetWorld();
    if (!World || !World->IsGameWorld() || World->bIsTearingDown || GIsRequestingExit)
    {
        return;
    }

    // Resolver Manager por World (evitar globals cross-world)
    TWeakObjectPtr<ATurboSequence_Manager_Lf> ManagerWeak;
    if (UTurboSequenceWorldSubsystem* TSWorld = UTurboSequenceWorldSubsystem::Get(World))
    {
        ManagerWeak = TSWorld->GetOrFindManager(World);
    }

    CleanupQuery.ForEachEntityChunk(EntityManager, Context, [World, ManagerWeak](FMassExecutionContext& Ctx)
    {
        // Convertir el weak ptr a strong ptr dentro del lambda
        ATurboSequence_Manager_Lf* Manager = ManagerWeak.Get();
        if (!IsValid(Manager))
        {
            UE_LOG(LogTemp, Warning, TEXT("TS Cleanup: Skipping chunk - Manager is not valid"));
            return;
        }

        const TArrayView<FTurboSequenceInstanceFragment> TSFragments = Ctx.GetMutableFragmentView<FTurboSequenceInstanceFragment>();
        const int32 Num = Ctx.GetNumEntities();
        for (int32 i = 0; i < Num; ++i)
        {
            FTurboSequenceInstanceFragment& TS = TSFragments[i];
            if (TS.bInstanceCreated && TS.MeshData.IsMeshDataValid())
            {
                if (Manager && Manager->IsValidLowLevel() && !Manager->IsPendingKill())
                {
                    ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(TS.MeshData, World);
                    UE_LOG(LogTemp, Log, TEXT("TS Cleanup: Removed instance for pending cleanup entity"));
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("TS Cleanup: Skipping instance removal - Manager is invalid"));
                }
                TS.bInstanceCreated = false;
                TS.MeshData = FTurboSequence_MinimalMeshData_Lf(false);
            }
        }
    });
}

void UTurboSequenceCleanupProcessor::RemoveTSInstanceIfValid(UWorld* World, FTurboSequenceInstanceFragment& TS) const
{
    if (!World || !World->IsGameWorld() || World->bIsTearingDown || GIsRequestingExit)
    {
        return;
    }

    if (TS.bInstanceCreated && TS.MeshData.IsMeshDataValid())
    {
        // Resolver Manager por World (evitar globals cross-world)
        TWeakObjectPtr<ATurboSequence_Manager_Lf> ManagerWeak;
        if (UTurboSequenceWorldSubsystem* TSWorld = UTurboSequenceWorldSubsystem::Get(World))
        {
            ManagerWeak = TSWorld->GetOrFindManager(World);
        }

        // Convertir el weak ptr a strong ptr
        ATurboSequence_Manager_Lf* Manager = ManagerWeak.Get();
        if (IsValid(Manager))
        {
            ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(TS.MeshData, World);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TS Cleanup: RemoveTSInstanceIfValid - Manager is not valid"));
        }
        
        TS.bInstanceCreated = false;
        TS.MeshData = FTurboSequence_MinimalMeshData_Lf(false);
    }
}