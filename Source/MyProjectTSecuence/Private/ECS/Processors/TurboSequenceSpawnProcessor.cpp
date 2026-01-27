#include "ECS/Processors/TurboSequenceSpawnProcessor.h"

#include "ECS/Fragments/TurboSequenceFragment.h"
#include "ECS/Tags/TurboSequenceTag.h"
#include "ECS/Processors/TurboSequenceUpdateProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "ECS/Processors/TurboSequenceDestroyProcessor.h"
#include "MassCommonFragments.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"

namespace
{
	constexpr float TurboSequenceYawOffsetDeg = -90.0f;
}

UTurboSequenceSpawnProcessor::UTurboSequenceSpawnProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	bRequiresGameThreadExecution = true;
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceUpdateProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceDestroyProcessor::StaticClass()->GetFName());
}

void UTurboSequenceSpawnProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FTurboSequenceTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceSpawnProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World)
	{
		return;
	}

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [World](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FTransformFragment> Transforms = Ctx.GetFragmentView<FTransformFragment>();
		const TArrayView<FTurboSequenceFragment> TSFrags = Ctx.GetMutableFragmentView<FTurboSequenceFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			FTurboSequenceFragment& Frag = TSFrags[i];
			if (Frag.bHasInstance)
			{
				continue;
			}

			if (!Frag.SpawnData.IsSpawnDataValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("TurboSequenceSpawnProcessor: SpawnData inválido (Mesh=null) en entity local idx %d"), i);
				continue;
			}

			FTransform SpawnXf = Transforms[i].GetTransform();
			FRotator R = SpawnXf.Rotator();
			R.Yaw += TurboSequenceYawOffsetDeg;
			SpawnXf.SetRotation(R.Quaternion());
			Frag.Instance = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(Frag.SpawnData, SpawnXf, World);
			if (Frag.Instance.IsMeshDataValid())
			{
				ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(Frag.UpdateGroupIndex, Frag.Instance);
				Frag.AnimData = ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(Frag.Instance, Frag.Anim, Frag.AnimSettings);
				Frag.bHasInstance = true;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("TurboSequenceSpawnProcessor: AddSkinnedMeshInstance falló en entity local idx %d"), i);
				Frag.bHasInstance = false;
			}
		}
	});
}
