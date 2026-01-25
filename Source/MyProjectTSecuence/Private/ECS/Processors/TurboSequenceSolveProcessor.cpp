#include "ECS/Processors/TurboSequenceSolveProcessor.h"

#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Subsystems/TurboSequenceECSSubsystem.h"
#include "ECS/Processors/TurboSequenceSpawnProcessor.h"
#include "ECS/Processors/TurboSequenceUpdateProcessor.h"
#include "ECS/Processors/TurboSequenceDestroyProcessor.h"
#include "MassExecutionContext.h"

UTurboSequenceSolveProcessor::UTurboSequenceSolveProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	bRequiresGameThreadExecution = true;
	ExecutionOrder.ExecuteAfter.Add(UTurboSequenceSpawnProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteAfter.Add(UTurboSequenceUpdateProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceDestroyProcessor::StaticClass()->GetFName());
}

void UTurboSequenceSolveProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddTagRequirement<FTurboSequenceTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceSolveProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World)
	{
		return;
	}

	const uint64 Frame = GFrameCounter;
	if (LastSolvedFrame == Frame)
	{
		return;
	}

	bool bAny = false;
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [&bAny](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FTurboSequenceFragment> TSFrags = Ctx.GetFragmentView<FTurboSequenceFragment>();
		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			const FTurboSequenceFragment& Frag = TSFrags[i];
			if (Frag.bHasInstance && Frag.Instance.IsMeshDataValid())
			{
				bAny = true;
				break;
			}
		}
	});

	if (!bAny)
	{
		return;
	}

	UTurboSequenceECSSubsystem* TSSubsystem = UTurboSequenceECSSubsystem::Get(World);
	if (!TSSubsystem)
	{
		return;
	}

	TSSubsystem->SolveGroup_GameThread(Context.GetDeltaTimeSeconds(), 0);
	LastSolvedFrame = Frame;
}
