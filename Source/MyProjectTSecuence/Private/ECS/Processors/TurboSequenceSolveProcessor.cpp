#include "ECS/Processors/TurboSequenceSolveProcessor.h"

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
	bRequiresGameThreadExecution = true;
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
	if (!World || !ATurboSequence_Manager_Lf::Instance)
	{
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
	ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, World, UpdateCtx);
}
