#include "ECS/Processors/TurboSequenceUpdateProcessor.h"

#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Processors/TurboSequenceSpawnProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "ECS/Processors/TurboSequenceDestroyProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"

namespace
{
	constexpr float TurboSequenceYawOffsetDeg_Update = -90.0f;
}

UTurboSequenceUpdateProcessor::UTurboSequenceUpdateProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteAfter.Add(UTurboSequenceSpawnProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceDestroyProcessor::StaticClass()->GetFName());
}

void UTurboSequenceUpdateProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FTurboSequenceTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FTransformFragment> Transforms = Ctx.GetFragmentView<FTransformFragment>();
		const TArrayView<FTurboSequenceFragment> TSFrags = Ctx.GetMutableFragmentView<FTurboSequenceFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			FTurboSequenceFragment& Frag = TSFrags[i];
			if (Frag.bHasInstance && Frag.Instance.IsMeshDataValid())
			{
				FTransform Xf = Transforms[i].GetTransform();
				FRotator R = Xf.Rotator();
				R.Yaw += TurboSequenceYawOffsetDeg_Update;
				Xf.SetRotation(R.Quaternion());
				ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(Frag.Instance, Xf, false);
			}
		}
	});
}
