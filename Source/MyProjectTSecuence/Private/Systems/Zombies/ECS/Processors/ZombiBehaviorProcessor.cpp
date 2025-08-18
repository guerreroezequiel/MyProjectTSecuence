// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Systems/Zombies/ECS/Fragments/ZombiConfigFragment.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"

UZombiBehaviorProcessor::UZombiBehaviorProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Log solo en la primera instancia
    static bool bLoggedConstructor = false;
    if (!bLoggedConstructor)
    {

        bLoggedConstructor = true;
    }
}

void UZombiBehaviorProcessor::ConfigureQueries()
{
    // Query optimizada para comportamiento - solo fragmentos necesarios
    BehaviorQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    BehaviorQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadOnly);
    BehaviorQuery.AddSharedRequirement<FZombiConfigFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);

    // Tags para filtrado rápido
    BehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    BehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

void UZombiBehaviorProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa entidades activas para decisiones de IA
    BehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                     {
        TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        TArrayView<const FZombiTransformFragment> TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
        TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        TArrayView<const FZombiStimuliFragment> StimuliFragments = Context.GetFragmentView<FZombiStimuliFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiStateFragment& StateFragment = StateFragments[i];
            const FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStimuliFragment& StimuliFragment = StimuliFragments[i];

            // Solo procesar si está vivo
            if (!StateFragment.IsDead())
            {
                // Actualizar timers de comportamiento
                UpdateActionTimers(StateFragment, DeltaTime);

                // Evaluar transiciones de estado (DOP-compatible)
                EvaluateStateTransitions(StateFragment, TransformFragment, MovementFragment, StimuliFragment);

                // Actualizar estado actual
                UpdateCurrentState(StateFragment, TransformFragment, MovementFragment, StimuliFragment, DeltaTime);

                // Actualizar comportamiento de horda
                UpdateHordeBehavior(StateFragment, TransformFragment, DeltaTime);
            }
        } });
}

void UZombiBehaviorProcessor::EvaluateStateTransitions(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment)
{
    // Lógica de transición de estados basada en condiciones usando flags
    bool bIsChasing = StateFragment.IsChasing();
    bool bIsWalking = StateFragment.IsWalking();
    bool bIsIdle = StateFragment.IsIdle();

    // NUEVO: Verificar estímulos del jugador (DOP-compatible)
    if (StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus())
    {
        // Si tiene estímulo del jugador, cambiar a estado de persecución
        if (!bIsChasing)
        {
            StateFragment.SetToChasing();
            MovementFragment.StartChasing(StimuliFragment.StimulusDirection, 150); // Velocidad alta para persecución

            // NOTA: Tags no necesarios con enfoque híbrido DOP

            // Guardar datos del estímulo en StateData
            StateFragment.SetStateData(static_cast<uint32>(StimuliFragment.StimulusDistance));
        }
        return;
    }

    // Sin logs en hot-path

    // Verificar si está persiguiendo y debe salir del estado
    if (bIsChasing)
    {
        // NUEVO: Verificar condiciones para salir del Chase
        float ChaseTimer = StateFragment.GetStateTimerSeconds();

        // Salir del Chase si:
        // 1. No hay estímulo del jugador
        // 2. El estímulo expiró
        // 3. Ha estado persiguiendo demasiado tiempo (5 segundos)
        if (!StimuliFragment.HasPlayerStimulus() ||
            StimuliFragment.IsStimulusExpired() ||
            ChaseTimer > 5.0f)
        {
            // Decidir entre Idle o WalkAround aleatoriamente
            float RandomChoice = FMath::RandRange(0.0f, 1.0f);
            if (RandomChoice < 0.6f) // 60% chance de WalkAround
            {
                StateFragment.SetToWalking();
                MovementFragment.StartMoving(MovementFragment.GetDirection(), 50);
            }
            else // 40% chance de Idle
            {
                StateFragment.SetToIdle();
                MovementFragment.Stop();
            }
        }
        return;
    }

    // Transiciones entre Idle y WalkAround
    if (bIsIdle)
    {
        float IdleTimer = StateFragment.GetStateTimerSeconds();
        // Idle → WalkAround después de 2-4 segundos
        if (IdleTimer > FMath::RandRange(2.0f, 4.0f))
        {
            StateFragment.SetToWalking();
            // Generar dirección aleatoria
            FVector RandomDirection = FVector(
                                          FMath::RandRange(-1.0f, 1.0f),
                                          FMath::RandRange(-1.0f, 1.0f),
                                          0.0f)
                                          .GetSafeNormal();
            MovementFragment.StartMoving(RandomDirection, 50);
        }
    }
    else if (bIsWalking)
    {
        float WalkTimer = StateFragment.GetStateTimerSeconds();
        // WalkAround → Idle después de 5-8 segundos
        if (WalkTimer > FMath::RandRange(5.0f, 8.0f))
        {
            StateFragment.SetToIdle();
            MovementFragment.Stop();
        }
    }
    else
    {
        // Estado por defecto: empezar en Idle
        StateFragment.SetToIdle();
        MovementFragment.Stop();
    }
}

void UZombiBehaviorProcessor::UpdateCurrentState(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Actualizar timer del estado actual (DOP-compatible)
    float CurrentTimer = StateFragment.GetStateTimerSeconds();
    CurrentTimer += DeltaTime;
    StateFragment.SetStateTimerSeconds(CurrentTimer);

    // Lógica específica del estado actual usando flags
    if (StateFragment.IsChasing())
    {
        // Lógica de persecución
        UpdateChaseState(StateFragment, MovementFragment, StimuliFragment, DeltaTime);
    }
    else if (StateFragment.IsWalking())
    {
        // Lógica de caminar aleatoriamente
        UpdateWalkAroundState(StateFragment, MovementFragment, TransformFragment, DeltaTime);
    }
    else if (StateFragment.IsIdle())
    {
        // Lógica de estado inactivo
        UpdateIdleState(StateFragment, MovementFragment, DeltaTime);
    }
}

void UZombiBehaviorProcessor::UpdateChaseState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Verificar si debe dejar de perseguir
    if (!StimuliFragment.HasPlayerStimulus() || StimuliFragment.IsStimulusExpired())
    {
        StateFragment.SetToWalking();
        MovementFragment.StartMoving(MovementFragment.GetDirection(), 50); // Volver a velocidad normal
        return;
    }

    // Actualizar datos de persecución
    float DistanceToPlayer = StimuliFragment.StimulusDistance;
    StateFragment.SetStateData(static_cast<uint32>(DistanceToPlayer));

    // Actualizar dirección de persecución
    MovementFragment.SetDirection(StimuliFragment.StimulusDirection);

    // Sin logs en hot-path
}

void UZombiBehaviorProcessor::UpdateWalkAroundState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime)
{
    // Lógica de caminar aleatoriamente
    float Timer = StateFragment.GetStateTimerSeconds();

    // Cambiar dirección cada 2-4 segundos para más variedad
    float DirectionChangeInterval = FMath::RandRange(2.0f, 4.0f);
    if (Timer > DirectionChangeInterval)
    {
        // Resetear timer para el próximo cambio de dirección
        StateFragment.SetStateTimerSeconds(0.0f);

        // Generar nueva dirección aleatoria
        FVector RandomDirection = FVector(
                                      FMath::RandRange(-1.0f, 1.0f),
                                      FMath::RandRange(-1.0f, 1.0f),
                                      0.0f)
                                      .GetSafeNormal();

        MovementFragment.SetDirection(RandomDirection);
        MovementFragment.SetSpeed(static_cast<uint8>(FMath::RandRange(30, 70))); // Velocidad variable
    }
}

void UZombiBehaviorProcessor::UpdateIdleState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, float DeltaTime)
{
    // En estado Idle, el zombie está inmóvil
    // Las transiciones se manejan en EvaluateStateTransitions()

    // Asegurarse de que está realmente parado
    MovementFragment.Stop();

    // Comportamiento ocasional: girar la cabeza/cuerpo ligeramente
    float Timer = StateFragment.GetStateTimerSeconds();
    if (Timer > FMath::RandRange(1.0f, 3.0f))
    {
        // Pequeño ajuste de rotación para simular "mirar alrededor"
        // (Este procesamiento se maneja en TransformProcessor)
    }
}

void UZombiBehaviorProcessor::UpdateHordeBehavior(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime)
{
    // Lógica simplificada de horda usando flags
    if (StateFragment.IsFollowing())
    {
        // Lógica para seguir al líder
        // TODO: Implementar lógica de seguimiento usando flags
        StateFragment.SetFollowing(true);
    }
    else if (StateFragment.IsSwarming())
    {
        // Lógica de enjambre
        // TODO: Implementar lógica de enjambre usando flags
        StateFragment.SetSwarming(true);
    }
    else
    {
        // Comportamiento individual por defecto
        StateFragment.SetIndividual(true);
    }
}

void UZombiBehaviorProcessor::UpdateActionTimers(FZombiStateFragment &StateFragment, float DeltaTime)
{
    // Actualizar timer de comportamiento (usando StateTimer)
    float CurrentTimer = StateFragment.GetStateTimerSeconds();
    CurrentTimer += DeltaTime;
    StateFragment.SetStateTimerSeconds(CurrentTimer);

    // Actualizar timer de acciones
    float ActionTimer = StateFragment.GetActionTimerSeconds();
    ActionTimer += DeltaTime;
    StateFragment.SetActionTimerSeconds(ActionTimer);

    // Limpiar acciones que han expirado (usando ActionTimer como referencia)
    if (ActionTimer > 2.0f) // Acciones duran máximo 2 segundos
    {
        StateFragment.ClearAllActions();
        StateFragment.SetActionTimerSeconds(0.0f); // Reset timer
    }
}