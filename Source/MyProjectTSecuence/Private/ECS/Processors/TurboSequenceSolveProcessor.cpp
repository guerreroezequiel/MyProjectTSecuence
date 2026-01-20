#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "Misc/CoreMisc.h"

#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "ECS/Processors/TurboSequenceSyncProcessor.h"
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MinimalData_Lf.h"
#include "ECS/TurboSequence/TurboSequenceWorldSubsystem.h"

UTurboSequenceSolveProcessor::UTurboSequenceSolveProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PostPhysics;
    //bRequiresGameThreadExecution = true;
    ExecutionFlags = (int32)EProcessorExecutionFlags::All;
    // Ensure Solve runs after Sync within the same phase (expects FName)
    ExecutionOrder.ExecuteAfter.Add(UTurboSequenceSyncProcessor::StaticClass()->GetFName());
    UE_LOG(LogTemp, Log, TEXT("TurboSequenceSolveProcessor: Initialized (Phase=PostPhysics)"));
}

void UTurboSequenceSolveProcessor::ConfigureQueries()
{
    // No entity requirements: this processor runs once per frame to drive TS Solve
}

void UTurboSequenceSolveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = EntityManager.GetWorld();
    if (!World || !World->IsGameWorld() || World->bIsTearingDown || IsEngineExitRequested())
    {
        return;
    }

    // Resolver Manager por World (evitar globals cross-world)
    TWeakObjectPtr<ATurboSequence_Manager_Lf> ManagerWeak;
    if (UTurboSequenceWorldSubsystem* TSWorld = UTurboSequenceWorldSubsystem::Get(World))
    {
        ManagerWeak = TSWorld->GetOrFindManager(World);
    }

    // Convertir el weak ptr a strong ptr
    ATurboSequence_Manager_Lf* Manager = ManagerWeak.Get();
    if (!IsValid(Manager))
    {
        UE_LOG(LogTemp, Warning, TEXT("TS Solve: Skipping - Manager is not valid"));
        return;
    }

    // Guard per-World: ensure we solve once per frame per World
    if (UTurboSequenceWorldSubsystem* TSWorld = UTurboSequenceWorldSubsystem::Get(World))
    {
        if (!TSWorld->ShouldSolveThisFrame(GFrameCounter))
        {
            return; // already solved this frame for this World
        }
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();
    FTurboSequence_UpdateContext_Lf UpdateCtx(0); // Group 0 por defecto
    
    // Validar Manager nuevamente antes de usarlo
    if (IsValid(Manager))
    {
        ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, World, UpdateCtx);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TS Solve: Manager became invalid before SolveMeshes_GameThread call"));
    }
}