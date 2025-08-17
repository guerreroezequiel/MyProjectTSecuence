// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiIdleProcessor.h"
#include "MassExecutionContext.h"

UZombiIdleProcessor::UZombiIdleProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    // ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Behavior); // Removido - no existe en UE5.5
}

void UZombiIdleProcessor::ConfigureQueries()
{
    // Query especializado para entidades inactivas - BAJA PRIORIDAD DOP
    IdleQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    IdleQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    IdleQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);

    // Tags para filtrado DOP
    IdleQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    IdleQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    IdleQuery.RegisterWithProcessor(*this);
}

void UZombiIdleProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // OPTIMIZACIÓN DOP: Solo procesar cada N frames para entidades idle
    static int32 FrameCounter = 0;
    if (++FrameCounter % 4 != 0) // Solo cada 4 frames (15 FPS en lugar de 60)
    {
        return;
    }

    // Solo procesar entidades que están en estado idle
    IdleQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                 {
        const int32 NumEntities = Context.GetNumEntities();
        auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiStateFragment& StateFragment = StateFragments[i];
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];

            // Solo procesar si realmente está idle
            if (StateFragment.IsIdle())
            {
                ProcessIdleLogic(StateFragment, TransformFragment, MovementFragment, DeltaTime * 4); // Compensar por frame skip
            }
        } });
}

void UZombiIdleProcessor::ProcessIdleLogic(FZombiStateFragment &StateFragment,
                                           FZombiTransformFragment &TransformFragment,
                                           FZombiMovementFragment &MovementFragment,
                                           float DeltaTime)
{
    // SIMPLE: Solo asegurar que esté parado
    if (MovementFragment.GetSpeed() > 0)
    {
        MovementFragment.Stop();
    }

    // SIMPLE: Solo rotar un poco hacia los costados cada tanto
    if (ShouldPlayIdleAnimation(StateFragment))
    {
        // Rotar un poco hacia la izquierda o derecha
        float RandomRotation = FMath::RandRange(-30.0f, 30.0f); // ±30 grados
        float CurrentYaw = TransformFragment.GetYaw();
        float NewYaw = CurrentYaw + RandomRotation;
        TransformFragment.SetYaw(NewYaw);

        // Resetear timer
        StateFragment.SetStateTimerSeconds(0.0f);

        // DEBUG
        static int32 RotationCounter = 0;
        if (++RotationCounter % 10 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("😴 IdleProcessor: Rotación idle - Yaw: %.1f → %.1f"), CurrentYaw, NewYaw);
        }
    }
}

bool UZombiIdleProcessor::ShouldPlayIdleAnimation(const FZombiStateFragment &StateFragment)
{
    float TimeSinceLastAnimation = StateFragment.GetStateTimerSeconds();
    float RandomAnimationTime = FMath::RandRange(IDLE_ANIMATION_MIN_TIME, IDLE_ANIMATION_MAX_TIME);

    return TimeSinceLastAnimation >= RandomAnimationTime;
}