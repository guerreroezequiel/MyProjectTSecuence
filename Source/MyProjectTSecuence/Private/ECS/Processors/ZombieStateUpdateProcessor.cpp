#include "ECS/Processors/ZombieStateUpdateProcessor.h"

#include "ECS/Fragments/ZombieStateFragment.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/MoveFragment.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "ECS/Processors/MoveIntegrateProcessor.h"
#include "MassExecutionContext.h"

static TAutoConsoleVariable<float> CVarTsZombieEpsSpeed(
	TEXT("ts.Zombie.State.EpsSpeed"),
	1.0f,
	TEXT("Speed epsilon used to classify Idle vs moving"),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarTsZombieRunThreshold(
	TEXT("ts.Zombie.State.RunThreshold"),
	250.0f,
	TEXT("Speed threshold used to classify Walk vs Run"),
	ECVF_Default);

UZombieStateUpdateProcessor::UZombieStateUpdateProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	ExecutionOrder.ExecuteAfter.Add(UMoveIntegrateProcessor::StaticClass()->GetFName());
}

void UZombieStateUpdateProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FFlowReadFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FMoveFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FZombieStateFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UZombieStateUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	const float EpsSpeed = FMath::Max(0.0f, CVarTsZombieEpsSpeed.GetValueOnAnyThread());
	const float RunThreshold = FMath::Max(EpsSpeed, CVarTsZombieRunThreshold.GetValueOnAnyThread());

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [EpsSpeed, RunThreshold](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FFlowReadFragment> FlowReads = Ctx.GetFragmentView<FFlowReadFragment>();
		const TConstArrayView<FMoveFragment> Moves = Ctx.GetFragmentView<FMoveFragment>();
		const TConstArrayView<FZombiCoreFragment> Cores = Ctx.GetFragmentView<FZombiCoreFragment>();
		const TArrayView<FZombieStateFragment> States = Ctx.GetMutableFragmentView<FZombieStateFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			const FFlowReadFragment& Flow = FlowReads[i];
			const FMoveFragment& Move = Moves[i];
			const FZombiCoreFragment& Core = Cores[i];
			FZombieStateFragment& State = States[i];

			if (Core.Health <= 0.0f)
			{
				State.Action = EZombieActionState::Dead;
				State.Loco = EZombieLocoState::Idle;
				continue;
			}

			State.Action = EZombieActionState::None;

			float SpeedWS = Move.Speed;
			if (!Flow.bValid)
			{
				SpeedWS = 0.0f;
			}

			if (SpeedWS <= EpsSpeed)
			{
				State.Loco = EZombieLocoState::Idle;
			}
			else if (SpeedWS < RunThreshold)
			{
				State.Loco = EZombieLocoState::Walk;
			}
			else
			{
				State.Loco = EZombieLocoState::Run;
			}
		}
	});
}
