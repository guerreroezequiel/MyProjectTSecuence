#include "ECS/Processors/TurboSequenceAnimApplyProcessor.h"

#include "ECS/Fragments/AnimRequestFragment.h"
#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Processors/ZombieAnimSelectProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "ECS/Processors/TurboSequenceDestroyProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"

UTurboSequenceAnimApplyProcessor::UTurboSequenceAnimApplyProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteAfter.Add(UZombieAnimSelectProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceDestroyProcessor::StaticClass()->GetFName());
}

void UTurboSequenceAnimApplyProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAnimRequestFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FTurboSequenceTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceAnimApplyProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Ctx)
	{
		const TArrayView<FTurboSequenceFragment> TSFrags = Ctx.GetMutableFragmentView<FTurboSequenceFragment>();
		const TArrayView<FAnimRequestFragment> Requests = Ctx.GetMutableFragmentView<FAnimRequestFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			FTurboSequenceFragment& TS = TSFrags[i];
			FAnimRequestFragment& Req = Requests[i];

			if (!TS.bHasInstance || !TS.Instance.IsMeshDataValid())
			{
				continue;
			}

			if (!Req.bDirty)
			{
				continue;
			}

			if (!Req.DesiredAnim)
			{
				continue;
			}

			TS.AnimSettings.AnimationSpeed = Req.DesiredPlayRate;

			const bool bSameAnim = (TS.Anim == Req.DesiredAnim);
			if (bSameAnim && TS.AnimData.IsAnimCollectionValid())
			{
				ATurboSequence_Manager_Lf::TweakAnimationCollection_Concurrent(TS.AnimSettings, TS.AnimData);
			}
			else
			{
				TS.AnimData = ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(TS.Instance, Req.DesiredAnim, TS.AnimSettings);
				TS.Anim = Req.DesiredAnim;
			}

			Req.bDirty = false;
		}
	});
}
