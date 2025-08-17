// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
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

            // Guardar datos del estímulo en StateData
            StateFragment.SetStateData(static_cast<uint32>(StimuliFragment.StimulusDistance));

            // Log para debugging
            UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: Zombie cambió a Chase por estímulo del jugador - Distancia: %.1f, Intensidad: %d"),
                   StimuliFragment.StimulusDistance, StimuliFragment.TotalStimulusIntensity);
        }
        return;
    }

    // DEBUG: Log cuando no hay estímulos del jugador
    static int32 DebugCounter = 0;
    if (++DebugCounter % 300 == 0) // Log cada 30 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: Zombie sin estímulos - HasPlayerStimulus: %s, HasAnyStimulus: %s"),
               StimuliFragment.HasPlayerStimulus() ? TEXT("Sí") : TEXT("No"),
               StimuliFragment.HasAnyStimulus() ? TEXT("Sí") : TEXT("No"));
    }

    // Verificar si está persiguiendo (lógica simplificada)
    if (bIsChasing)
    {
        // Mantener estado de persecución
        return;
    }

    // Estado por defecto: caminar aleatoriamente
    if (!bIsWalking)
    {
        StateFragment.SetToWalking();
        MovementFragment.StartMoving(TransformFragment.GetForwardVector(), 50); // Velocidad normal para caminar
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

    // Log para debugging
    UE_LOG(LogTemp, Verbose, TEXT("🧠 ChaseState: Zombie persiguiendo - Distancia: %.1f"), DistanceToPlayer);
}

void UZombiBehaviorProcessor::UpdateWalkAroundState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime)
{
    // Lógica de caminar aleatoriamente
    float Timer = StateFragment.GetStateTimerSeconds();

    // Cambiar dirección cada 3 segundos
    if (Timer > 3.0f)
    {
        StateFragment.SetStateTimerSeconds(0.0f);

        // Generar nueva dirección aleatoria
        FVector RandomDirection = FVector(
                                      FMath::RandRange(-1.0f, 1.0f),
                                      FMath::RandRange(-1.0f, 1.0f),
                                      0.0f)
                                      .GetSafeNormal();

        MovementFragment.SetDirection(RandomDirection);
    }
}

void UZombiBehaviorProcessor::UpdateIdleState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, float DeltaTime)
{
    // Lógica de estado inactivo
    float Timer = StateFragment.GetStateTimerSeconds();

    // Cambiar a caminar después de 2 segundos de inactividad
    if (Timer > 2.0f)
    {
        StateFragment.SetToWalking();
        MovementFragment.StartMoving(MovementFragment.GetDirection(), 30); // Velocidad baja
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