#include "ECS/Processors/TurboSequenceDestroyProcessor.h"

#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Tags/HiddenTag.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "TurboSequence_Manager_Lf.h"

UTurboSequenceDestroyProcessor::UTurboSequenceDestroyProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	bRequiresGameThreadExecution = true;
}

void UTurboSequenceDestroyProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FTurboSequenceTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FHiddenTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceDestroyProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World)
	{
		return;
	}

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [World](FMassExecutionContext& Ctx)
	{
		const TArrayView<FTurboSequenceFragment> TSFrags = Ctx.GetMutableFragmentView<FTurboSequenceFragment>();
		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			const FMassEntityHandle Entity = Ctx.GetEntity(i);
			FTurboSequenceFragment& Frag = TSFrags[i];

			if (Frag.bHasInstance && Frag.Instance.IsMeshDataValid())
			{
				ATurboSequence_Manager_Lf::RemoveInstanceFromUpdateGroup_Concurrent(Frag.UpdateGroupIndex, Frag.Instance);
				ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(Frag.Instance, World);
			}

			Frag.Instance = FTurboSequence_MinimalMeshData_Lf();
			Frag.bHasInstance = false;

			// Remove tags so we don't respawn immediately and don't keep destroying every tick.
			Ctx.Defer().RemoveTag<FTurboSequenceTag>(Entity);
			Ctx.Defer().RemoveTag<FHiddenTag>(Entity);
		}
	});
}
