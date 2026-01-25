#include "ECS/Subsystems/TurboSequenceECSSubsystem.h"

#include "TurboSequence_Manager_Lf.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UTurboSequenceECSSubsystem* UTurboSequenceECSSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UTurboSequenceECSSubsystem>() : nullptr;
}

ATurboSequence_Manager_Lf* UTurboSequenceECSSubsystem::EnsureManager_GameThread()
{
	check(IsInGameThread());

	if (CachedManager.IsValid())
	{
		return CachedManager.Get();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<ATurboSequence_Manager_Lf> It(World); It; ++It)
	{
		CachedManager = *It;
		break;
	}

	if (!CachedManager.IsValid())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		CachedManager = World->SpawnActor<ATurboSequence_Manager_Lf>(ATurboSequence_Manager_Lf::StaticClass(), FTransform::Identity, Params);
	}

	return CachedManager.Get();
}

void UTurboSequenceECSSubsystem::SolveGroup_GameThread(float DeltaTime, int32 GroupIndex)
{
	check(IsInGameThread());

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	EnsureManager_GameThread();

	FTurboSequence_UpdateContext_Lf UpdateContext;
	UpdateContext.GroupIndex = GroupIndex;
	ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, World, UpdateContext);
}
