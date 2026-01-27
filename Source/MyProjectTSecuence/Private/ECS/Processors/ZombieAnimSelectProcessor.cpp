#include "ECS/Processors/ZombieAnimSelectProcessor.h"

#include "ECS/Fragments/AnimRequestFragment.h"
#include "ECS/Fragments/TileLODFragment.h"
#include "ECS/Fragments/ZombieStateFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "ECS/Processors/TurboSequenceUpdateProcessor.h"
#include "ECS/Processors/TurboSequenceSolveProcessor.h"
#include "ECS/Processors/TurboSequenceDestroyProcessor.h"
#include "ECS/Processors/TurboSequenceAnimApplyProcessor.h"
#include "MassExecutionContext.h"

static TAutoConsoleVariable<float> CVarTsZombieAnimWarmRate(
	TEXT("ts.Zombie.Anim.WarmRate"),
	0.9f,
	TEXT("Play rate multiplier for Warm LOD"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimColdRate(
	TEXT("ts.Zombie.Anim.ColdRate"),
	0.8f,
	TEXT("Play rate multiplier for Cold LOD"),
	ECVF_Default);

UZombieAnimSelectProcessor::UZombieAnimSelectProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PostPhysics;
	ExecutionOrder.ExecuteAfter.Add(UTurboSequenceUpdateProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceAnimApplyProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceSolveProcessor::StaticClass()->GetFName());
	ExecutionOrder.ExecuteBefore.Add(UTurboSequenceDestroyProcessor::StaticClass()->GetFName());
}

void UZombieAnimSelectProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FZombieStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTileLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAnimRequestFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UZombieAnimSelectProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float WarmRate = CVarTsZombieAnimWarmRate.GetValueOnAnyThread();
	const float ColdRate = CVarTsZombieAnimColdRate.GetValueOnAnyThread();

	UAnimSequence* LocalIdle = AnimIdle;
	UAnimSequence* LocalWalk = AnimWalk;
	UAnimSequence* LocalRun = AnimRun;
	UAnimSequence* LocalDead = AnimDead;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [WarmRate, ColdRate, LocalIdle, LocalWalk, LocalRun, LocalDead](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FZombieStateFragment> States = Ctx.GetFragmentView<FZombieStateFragment>();
		const TConstArrayView<FTileLODFragment> LODs = Ctx.GetFragmentView<FTileLODFragment>();
		const TArrayView<FAnimRequestFragment> Requests = Ctx.GetMutableFragmentView<FAnimRequestFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			const FZombieStateFragment& State = States[i];
			const FTileLODFragment& LOD = LODs[i];
			FAnimRequestFragment& Req = Requests[i];

			UAnimSequence* NextAnim = nullptr;
			if (State.Action == EZombieActionState::Dead)
			{
				NextAnim = LocalDead;
			}
			else
			{
				switch (State.Loco)
				{
				case EZombieLocoState::Idle: NextAnim = LocalIdle; break;
				case EZombieLocoState::Walk: NextAnim = LocalWalk; break;
				case EZombieLocoState::Run: NextAnim = LocalRun; break;
				default: break;
				}
			}

			if (!NextAnim)
			{
				NextAnim = Req.DesiredAnim;
			}

			float NextRate = 1.0f;
			switch (LOD.LOD)
			{
			case ETileLOD::Hot: NextRate = 1.0f; break;
			case ETileLOD::Warm: NextRate = WarmRate; break;
			case ETileLOD::Cold: NextRate = ColdRate; break;
			default: break;
			}

			const bool bAnimChanged = (Req.DesiredAnim != NextAnim);
			const bool bRateChanged = !FMath::IsNearlyEqual(Req.DesiredPlayRate, NextRate, 0.001f);
			if (bAnimChanged || bRateChanged)
			{
				Req.DesiredAnim = NextAnim;
				Req.DesiredPlayRate = NextRate;
				Req.bDirty = true;
			}
		}
	});
}
