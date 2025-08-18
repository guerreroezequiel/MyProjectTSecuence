// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiUnifiedBehaviorProcessor.h"
#include "MassExecutionContext.h"

UZombiUnifiedBehaviorProcessor::UZombiUnifiedBehaviorProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("ZombiBehavior");
    ExecutionOrder.ExecuteAfter.Add(TEXT("StimulusProcessor"));
}

void UZombiUnifiedBehaviorProcessor::ConfigureQueries()
{
    // UNA SOLA QUERY: Todos los fragments necesarios para comportamiento completo
    UnifiedBehaviorQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    UnifiedBehaviorQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    UnifiedBehaviorQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    UnifiedBehaviorQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadOnly);

    // Tags de filtrado
    UnifiedBehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    UnifiedBehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    UnifiedBehaviorQuery.RegisterWithProcessor(*this);
}

void UZombiUnifiedBehaviorProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante PIE
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetWorld()->HasBegunPlay())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Debug
    static int32 ExecuteCounter = 0;
    if (++ExecuteCounter % 300 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("🎯 UnifiedBehavior: EJECUTÁNDOSE | Count: %d"), ExecuteCounter);
    }

    // PROCESAR TODOS LOS ZOMBIES EN UN SOLO LOOP OPTIMIZADO
    UnifiedBehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                            {
        const int32 NumEntities = Context.GetNumEntities();
        if (NumEntities == 0) return;

        // Debug
        static int32 ChunkCounter = 0;
        if (++ChunkCounter % 150 == 0)
        {
            UE_LOG(LogTemp, Warning, TEXT("🎯 UnifiedBehavior Chunk: %d entidades"), NumEntities);
        }

        // Obtener todos los fragments de una vez (DOP/ECS optimizado)
        auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        auto TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StimuliFragments = Context.GetFragmentView<FZombiStimuliFragment>();

        // LOOP PRINCIPAL: Procesar cada entidad con el pipeline completo
        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiStateFragment &StateFragment = StateFragments[i];
            const FZombiTransformFragment &TransformFragment = TransformFragments[i];
            FZombiMovementFragment &MovementFragment = MovementFragments[i];
            const FZombiStimuliFragment &StimuliFragment = StimuliFragments[i];

            // ACTUALIZAR TIMER (centralizado)
            float CurrentTimer = StateFragment.GetStateTimerSeconds();
            CurrentTimer += DeltaTime;
            StateFragment.SetStateTimerSeconds(CurrentTimer);

            // PIPELINE DE COMPORTAMIENTO:
            
            // FASE 1: ¿Hay estímulo que override el comportamiento automático?
            if (ShouldOverrideWithStimulus(StimuliFragment))
            {
                EvaluateStimulusResponse(StateFragment, StimuliFragment, MovementFragment);
            }
            else
            {
                // FASE 2: Comportamiento automático (Idle ↔ WalkAround)
                ExecuteAutomaticBehavior(StateFragment, TransformFragment, MovementFragment, DeltaTime);
            }

            // FASE 3: Sincronizar animaciones (TurboSequence)
            // TODO: Implementar después
            // SynchronizeAnimationState(StateFragment, MovementFragment);

            // Debug individual (poco frecuente)
            static int32 EntityDebugCounter = 0;
            if (++EntityDebugCounter % 600 == 0 && i == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("🎯 Entity[0]: %s | Timer=%.1f | HasStimulus=%s"),
                       StateFragment.IsIdle() ? TEXT("Idle") : 
                       StateFragment.IsWalking() ? TEXT("Walk") : 
                       StateFragment.IsChasing() ? TEXT("Chase") : TEXT("Unknown"),
                       CurrentTimer,
                       ShouldOverrideWithStimulus(StimuliFragment) ? TEXT("✓") : TEXT("✗"));
            }
        } });
}

void UZombiUnifiedBehaviorProcessor::EvaluateStimulusResponse(FZombiStateFragment &StateFragment,
                                                              const FZombiStimuliFragment &StimuliFragment,
                                                              FZombiMovementFragment &MovementFragment)
{
    // STIMULUS OVERRIDE: Player detected → Chase
    if (StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus())
    {
        if (!StateFragment.IsChasing())
        {
            StateFragment.SetToChasing();
            MovementFragment.StartChasing(StimuliFragment.StimulusDirection, CHASE_SPEED);

            static int32 ChaseCounter = 0;
            if (++ChaseCounter % 60 == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("🎯 STIMULUS OVERRIDE: → Chase | Distance=%.1f"),
                       StimuliFragment.StimulusDistance);
            }
        }
    }
    // TODO: Agregar otros tipos de estímulos (Flee, Investigate, etc.)
}

void UZombiUnifiedBehaviorProcessor::ExecuteAutomaticBehavior(FZombiStateFragment &StateFragment,
                                                              const FZombiTransformFragment &TransformFragment,
                                                              FZombiMovementFragment &MovementFragment,
                                                              float DeltaTime)
{
    // COMPORTAMIENTO AUTOMÁTICO: Default loop sin estímulos externos

    if (StateFragment.IsIdle())
    {
        // Idle → WalkAround después de un tiempo
        if (ShouldTransitionFromIdle(StateFragment))
        {
            StateFragment.SetToWalking();
            FVector RandomDirection = GenerateRandomDirection();
            MovementFragment.StartMoving(RandomDirection, DEFAULT_WALK_SPEED);

            UE_LOG(LogTemp, Warning, TEXT("🎯 AUTO: Idle → WalkAround | Timer=%.1f Dir=%s"),
                   StateFragment.GetStateTimerSeconds(), *RandomDirection.ToString());
        }
        else
        {
            // Ejecutar comportamiento Idle
            ExecuteIdleBehavior(StateFragment, TransformFragment, MovementFragment);
        }
    }
    else if (StateFragment.IsWalking())
    {
        // WalkAround → Idle después de un tiempo
        if (ShouldTransitionFromWalk(StateFragment))
        {
            StateFragment.SetToIdle();
            MovementFragment.Stop();

            UE_LOG(LogTemp, Warning, TEXT("🎯 AUTO: WalkAround → Idle | Timer=%.1f"),
                   StateFragment.GetStateTimerSeconds());
        }
        else
        {
            // Ejecutar comportamiento WalkAround
            ExecuteWalkAroundBehavior(StateFragment, TransformFragment, MovementFragment);
        }
    }
    else if (StateFragment.IsChasing())
    {
        // Chase → volver a comportamiento automático si se pierde el estímulo
        // (Esto se maneja en EvaluateStimulusResponse)

        // Volver a Idle/Walk si no hay estímulo
        StateFragment.SetToIdle();
        MovementFragment.Stop();
        UE_LOG(LogTemp, Log, TEXT("🎯 Chase perdido → Idle"));
    }
}

void UZombiUnifiedBehaviorProcessor::ExecuteIdleBehavior(FZombiStateFragment &StateFragment,
                                                         const FZombiTransformFragment &TransformFragment,
                                                         FZombiMovementFragment &MovementFragment)
{
    // Asegurar que esté detenido
    if (MovementFragment.GetSpeed() > 0)
    {
        MovementFragment.Stop();
    }

    // TODO: Rotaciones ocasionales, animaciones idle, etc.
}

void UZombiUnifiedBehaviorProcessor::ExecuteWalkAroundBehavior(FZombiStateFragment &StateFragment,
                                                               const FZombiTransformFragment &TransformFragment,
                                                               FZombiMovementFragment &MovementFragment)
{
    // Cambiar dirección ocasionalmente
    float TimeSinceDirectionChange = StateFragment.GetStateTimerSeconds();
    if (TimeSinceDirectionChange > DIRECTION_CHANGE_TIME)
    {
        FVector NewDirection = GenerateRandomDirection();
        MovementFragment.SetDirection(NewDirection);
        StateFragment.SetStateTimerSeconds(0.0f); // Reset solo para direction change

        UE_LOG(LogTemp, Log, TEXT("🎯 WalkAround: Nueva dirección | Dir=%s"), *NewDirection.ToString());
    }
}

FVector UZombiUnifiedBehaviorProcessor::GenerateRandomDirection()
{
    float RandomAngle = FMath::RandRange(0.0f, 2.0f * PI);
    return FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f).GetSafeNormal();
}

bool UZombiUnifiedBehaviorProcessor::ShouldTransitionFromIdle(const FZombiStateFragment &StateFragment)
{
    return StateFragment.GetStateTimerSeconds() >= IDLE_TO_WALK_TIME;
}

bool UZombiUnifiedBehaviorProcessor::ShouldTransitionFromWalk(const FZombiStateFragment &StateFragment)
{
    return StateFragment.GetStateTimerSeconds() >= WALK_TO_IDLE_TIME;
}

bool UZombiUnifiedBehaviorProcessor::ShouldOverrideWithStimulus(const FZombiStimuliFragment &StimuliFragment)
{
    // Por ahora solo Player stimulus, pero se puede extender
    return false; // TEMPORAL: Desactivar stimulus para testing del comportamiento automático
    // return StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus();
}
