#include "ECS/Processors/ZombieAnimSelectProcessor.h"

#include "ECS/Fragments/AnimLocomotionFragment.h"
#include "ECS/Fragments/AnimRequestFragment.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/MoveFragment.h"
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

static TAutoConsoleVariable<float> CVarTsZombieAnimEnterWalkSpeed(
	TEXT("ts.Zombie.Anim.EnterWalkSpeed"),
	10.0f,
	TEXT("Hysteresis enter-walk speed threshold"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimExitWalkSpeed(
	TEXT("ts.Zombie.Anim.ExitWalkSpeed"),
	6.0f,
	TEXT("Hysteresis exit-walk speed threshold"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimWalkSpeedRef(
	TEXT("ts.Zombie.Anim.WalkSpeedRef"),
	150.0f,
	TEXT("Reference speed for walk animation play rate"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimMinRate(
	TEXT("ts.Zombie.Anim.MinRate"),
	0.8f,
	TEXT("Minimum play rate for locomotion"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimMaxRate(
	TEXT("ts.Zombie.Anim.MaxRate"),
	1.25f,
	TEXT("Maximum play rate for locomotion"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieAnimEpsPlayRate(
	TEXT("ts.Zombie.Anim.EpsPlayRate"),
	0.05f,
	TEXT("Play rate epsilon for dirty updates"),
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
	EntityQuery.AddRequirement<FMoveFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FFlowReadFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FZombieStateFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTileLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FAnimLocomotionFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddRequirement<FAnimRequestFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UZombieAnimSelectProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float WarmRate = CVarTsZombieAnimWarmRate.GetValueOnAnyThread();
	const float ColdRate = CVarTsZombieAnimColdRate.GetValueOnAnyThread();
	const float EnterWalkSpeed = CVarTsZombieAnimEnterWalkSpeed.GetValueOnAnyThread();
	const float ExitWalkSpeed = CVarTsZombieAnimExitWalkSpeed.GetValueOnAnyThread();
	const float WalkSpeedRef = FMath::Max(1.0f, CVarTsZombieAnimWalkSpeedRef.GetValueOnAnyThread());
	const float MinRate = CVarTsZombieAnimMinRate.GetValueOnAnyThread();
	const float MaxRate = CVarTsZombieAnimMaxRate.GetValueOnAnyThread();
	const float EpsRate = FMath::Max(0.0f, CVarTsZombieAnimEpsPlayRate.GetValueOnAnyThread());

	UAnimSequence* LocalIdle = AnimIdle;
	UAnimSequence* LocalWalk = AnimWalk;
	UAnimSequence* LocalRun = AnimRun;
	UAnimSequence* LocalDead = AnimDead;

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [WarmRate, ColdRate, EnterWalkSpeed, ExitWalkSpeed, WalkSpeedRef, MinRate, MaxRate, EpsRate, LocalIdle, LocalWalk, LocalRun, LocalDead](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FMoveFragment> Moves = Ctx.GetFragmentView<FMoveFragment>();
		const TConstArrayView<FFlowReadFragment> Flows = Ctx.GetFragmentView<FFlowReadFragment>();
		const TConstArrayView<FZombieStateFragment> States = Ctx.GetFragmentView<FZombieStateFragment>();
		const TConstArrayView<FTileLODFragment> LODs = Ctx.GetFragmentView<FTileLODFragment>();
		const TArrayView<FAnimLocomotionFragment> Locos = Ctx.GetMutableFragmentView<FAnimLocomotionFragment>();
		const TArrayView<FAnimRequestFragment> Requests = Ctx.GetMutableFragmentView<FAnimRequestFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			const FMoveFragment& Move = Moves[i];
			const FFlowReadFragment& Flow = Flows[i];
			const FZombieStateFragment& State = States[i];
			const FTileLODFragment& LOD = LODs[i];
			FAnimLocomotionFragment& Loco = Locos[i];
			FAnimRequestFragment& Req = Requests[i];

			const float Speed = Flow.bValid ? FMath::Max(0.0f, Move.Speed) : 0.0f;

			UAnimSequence* NextAnim = nullptr;
			if (State.Action == EZombieActionState::Dead)
			{
				NextAnim = LocalDead;
			}
			else
			{
				if (Loco.CurrentState == EAnimLocomotionState::Idle)
				{
					if (Speed >= EnterWalkSpeed)
					{
						Loco.CurrentState = EAnimLocomotionState::Walk;
					}
				}
				else
				{
					if (Speed <= ExitWalkSpeed)
					{
						Loco.CurrentState = EAnimLocomotionState::Idle;
					}
				}

				NextAnim = (Loco.CurrentState == EAnimLocomotionState::Walk) ? LocalWalk : LocalIdle;
			}

			if (!NextAnim)
			{
				NextAnim = Req.DesiredAnim;
			}

			float BaseRate = 1.0f;
			if (State.Action != EZombieActionState::Dead && Loco.CurrentState == EAnimLocomotionState::Walk)
			{
				BaseRate = FMath::Clamp(Speed / WalkSpeedRef, MinRate, MaxRate);
			}

			float LodMult = 1.0f;
			switch (LOD.LOD)
			{
			case ETileLOD::Hot: LodMult = 1.0f; break;
			case ETileLOD::Warm: LodMult = WarmRate; break;
			case ETileLOD::Cold: LodMult = ColdRate; break;
			default: break;
			}

			const float NextRate = BaseRate * LodMult;

			const bool bAnimChanged = (Req.DesiredAnim != NextAnim);
			const bool bRateChanged = (FMath::Abs(Req.DesiredPlayRate - NextRate) >= EpsRate);
			if (bAnimChanged || bRateChanged)
			{
				Req.DesiredAnim = NextAnim;
				Req.DesiredPlayRate = NextRate;
				Req.bLoop = true;
				Req.bDirty = true;
			}

			Loco.LastPlayRate = NextRate;
		}
	});
}
