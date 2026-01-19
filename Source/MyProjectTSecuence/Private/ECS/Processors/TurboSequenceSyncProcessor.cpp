#include "ECS/Processors/TurboSequenceSyncProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"

#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "MassCommonFragments.h"
#include "ECS/Fragments/TurboSequenceInstanceFragment.h"
#include "ECS/Tags/ZombiTag.h"
// TS
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Lf/Public/TurboSequence_MeshAsset_Lf.h"
// Utilities

#include "EngineUtils.h"
UTurboSequenceSyncProcessor::UTurboSequenceSyncProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	bRequiresGameThreadExecution = true;
	// Ensure Sync runs before Solve within the same phase
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
	UE_LOG(LogTemp, Log, TEXT("TurboSequenceSyncProcessor: Initialized (Phase=PostPhysics)"));
}

void UTurboSequenceSyncProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTurboSequenceInstanceFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTurboSequenceSyncProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World) return;

	// Resolve Manager instance on GameThread if still missing (BP may not have assigned Instance yet)
	static bool bLoggedManagerMissingOnce = false;
	if (!ATurboSequence_Manager_Lf::Instance)
	{
		for (TActorIterator<ATurboSequence_Manager_Lf> It(World); It; ++It)
		{
			ATurboSequence_Manager_Lf::Instance = *It;
			UE_LOG(LogTemp, Log, TEXT("TS Sync: Found and assigned TurboSequence Manager instance from world"));
			break;
		}
		if (!ATurboSequence_Manager_Lf::Instance && !bLoggedManagerMissingOnce)
		{
			bLoggedManagerMissingOnce = true;
			UE_LOG(LogTemp, Warning, TEXT("TS Sync: TurboSequence Manager Instance is NULL (ensure BP manager is in PIE world and BeginPlay has run)"));
		}
	}

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [World](FMassExecutionContext& Context)
	{
		const TConstArrayView<FTransformFragment> TransformFragments = Context.GetFragmentView<FTransformFragment>();
		const TArrayView<FTurboSequenceInstanceFragment> TSFragments = Context.GetMutableFragmentView<FTurboSequenceInstanceFragment>();

		const int32 NumEntities = Context.GetNumEntities();
		for (int32 i = 0; i < NumEntities; ++i)
		{
			const FTransform& Xf = TransformFragments[i].GetTransform();
			FTurboSequenceInstanceFragment& TS = TSFragments[i];

			if (!TS.bInstanceCreated)
			{
				if (!TS.MeshAsset)
				{
					// MeshAsset must be assigned by the spawner; skip entity until available
					continue;
				}
				else if (!ATurboSequence_Manager_Lf::Instance)
				{
					UE_LOG(LogTemp, Warning, TEXT("TS Sync: TurboSequence Manager Instance is NULL"));
				}
				else if (TS.MeshAsset && ATurboSequence_Manager_Lf::Instance)
				{
					FTurboSequence_MeshSpawnData_Lf SpawnData;
					SpawnData.RootMotionMesh.Mesh = TS.MeshAsset;
					FTurboSequence_MinimalMeshData_Lf MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(SpawnData, Xf, World);
					if (MeshData.IsMeshDataValid())
					{
						TS.MeshData = MeshData;
						TS.bInstanceCreated = true;
						ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_RawID_Concurrent(0, MeshData.RootMotionMeshID);
						ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_RawID_Concurrent(MeshData.RootMotionMeshID, Xf, true);
						UE_LOG(LogTemp, Log, TEXT("TS Sync: Instance created OK (ID=%d)"), MeshData.RootMotionMeshID);
						if (TS.MeshAsset->OverrideDefaultAnimation)
						{
							FTurboSequence_AnimPlaySettings_Lf Settings;
							ATurboSequence_Manager_Lf::PlayAnimation_RawID_Concurrent(MeshData.RootMotionMeshID, TS.MeshAsset->OverrideDefaultAnimation, Settings);
							UE_LOG(LogTemp, Log, TEXT("TS Sync: Play OverrideDefaultAnimation"));
						}
					}
					else
					{
						UE_LOG(LogTemp, Error, TEXT("TS Sync: AddSkinnedMeshInstance returned invalid MeshData"));
					}
				}
			}
			else
			{
				if (TS.MeshData.IsMeshDataValid())
				{
					ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_RawID_Concurrent(TS.MeshData.RootMotionMeshID, Xf, false);
					UE_LOG(LogTemp, VeryVerbose, TEXT("TS Sync: Updated transform for ID=%d"), TS.MeshData.RootMotionMeshID);
				}
			}
		}
	});
}
