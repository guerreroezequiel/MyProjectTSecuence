// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"

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
    BehaviorQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);

    BehaviorQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadOnly);
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
        TArrayView<FZombiBehaviorFragment> BehaviorFragments = Context.GetMutableFragmentView<FZombiBehaviorFragment>();
        TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

        TArrayView<const FZombiStimuliFragment> StimuliFragments = Context.GetFragmentView<FZombiStimuliFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];
            const FZombiCoreFragment& CoreFragment = CoreFragments[i];

            const FZombiStimuliFragment& StimuliFragment = StimuliFragments[i];

            // Solo procesar si está vivo
            if (!BehaviorFragment.IsDead())
            {
                // Actualizar timers de comportamiento
                UpdateActionTimers(BehaviorFragment, DeltaTime);

                // Evaluar transiciones de estado (DOP-compatible)
                EvaluateStateTransitions(BehaviorFragment, CoreFragment, StimuliFragment);

                // Actualizar estado actual
                UpdateCurrentState(BehaviorFragment, CoreFragment, StimuliFragment, DeltaTime);

                // Actualizar comportamiento de horda
                UpdateHordeBehavior(BehaviorFragment, CoreFragment, DeltaTime);
            }
        } });
}

void UZombiBehaviorProcessor::EvaluateStateTransitions(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, const FZombiStimuliFragment &StimuliFragment)
{
    // Lógica de transición de estados basada en condiciones
    EZombiState CurrentState = BehaviorFragment.GetState();







    // NUEVO: Verificar estímulos del jugador (DOP-compatible)
    if (StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus())
    {
        // Si tiene estímulo del jugador, cambiar a estado de persecución
        if (CurrentState != EZombiState::Chase)
        {
            BehaviorFragment.SetState(EZombiState::Chase);
            // Guardar datos del estímulo en StateData
            BehaviorFragment.SetStateData(StimuliFragment.StimulusDistance);

            // Log para debugging
            UE_LOG(LogTemp, Verbose, TEXT("🧠 BehaviorProcessor: Zombie cambió a Chase por estímulo del jugador - Distancia: %.1f, Intensidad: %d"),
                   StimuliFragment.StimulusDistance, StimuliFragment.TotalStimulusIntensity);
        }
        return;
    }

    // Verificar si está persiguiendo (lógica simplificada)
    if (BehaviorFragment.IsChasing())
    {
        // Mantener estado de persecución
        return;
    }

    // Estado por defecto: caminar aleatoriamente
    if (CurrentState != EZombiState::WalkAround)
    {
        BehaviorFragment.SetState(EZombiState::WalkAround);
    }
}

void UZombiBehaviorProcessor::UpdateCurrentState(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Actualizar timer del estado actual (DOP-compatible)
    float CurrentTimer = BehaviorFragment.GetStateTimer();
    CurrentTimer += DeltaTime;
    BehaviorFragment.SetStateTimer(CurrentTimer);

    // Lógica específica del estado actual
    EZombiState CurrentState = BehaviorFragment.GetState();

    switch (CurrentState)
    {
    case EZombiState::Chase:
        // Lógica de persecución
        UpdateChaseState(BehaviorFragment, StimuliFragment, DeltaTime);
        break;

    case EZombiState::WalkAround:
        // Lógica de caminar aleatoriamente
        UpdateWalkAroundState(BehaviorFragment, DeltaTime);
        break;



    default:
        break;
    }
}

void UZombiBehaviorProcessor::UpdateChaseState(FZombiBehaviorFragment &BehaviorFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Verificar si debe dejar de perseguir
    if (!StimuliFragment.HasPlayerStimulus() || StimuliFragment.IsStimulusExpired())
    {
        BehaviorFragment.SetState(EZombiState::WalkAround);
        return;
    }

    // Actualizar datos de persecución
    float DistanceToPlayer = StimuliFragment.StimulusDistance;
    BehaviorFragment.SetStateData(DistanceToPlayer);

    // Log para debugging
    UE_LOG(LogTemp, Verbose, TEXT("🧠 ChaseState: Zombie persiguiendo - Distancia: %.1f"), DistanceToPlayer);
}

void UZombiBehaviorProcessor::UpdateWalkAroundState(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    // Lógica de caminar aleatoriamente
    float Timer = BehaviorFragment.GetStateTimer();

    // Cambiar dirección cada 3 segundos
    if (Timer > 3.0f)
    {
        BehaviorFragment.SetStateTimer(0.0f);
        // La dirección se maneja en el MovementProcessor
    }
}



void UZombiBehaviorProcessor::UpdateHordeBehavior(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, float DeltaTime)
{
    // Lógica simplificada de horda
    if (BehaviorFragment.IsInHorde())
    {
        // Si está en una horda, actualizar comportamiento grupal
        if (BehaviorFragment.GetHordeBehavior() == EZombiHordeBehavior::Following)
        {
            // Lógica para seguir al líder
            // TODO: Implementar lógica de seguimiento
        }
        else if (BehaviorFragment.GetHordeBehavior() == EZombiHordeBehavior::Swarming)
        {
            // Lógica de enjambre
            // TODO: Implementar lógica de enjambre
        }
    }
}

void UZombiBehaviorProcessor::UpdateActionTimers(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    // Actualizar timer de acciones
    BehaviorFragment.ActionTimer += DeltaTime;

    // Limpiar acciones que han expirado
    if (BehaviorFragment.ActionTimer > 2.0f) // Acciones duran máximo 2 segundos
    {
        BehaviorFragment.ClearAllActions();
    }

    // Actualizar timer de comportamiento (usando StateTimer)
    float CurrentTimer = BehaviorFragment.GetStateTimer();
    CurrentTimer += DeltaTime;
    BehaviorFragment.SetStateTimer(CurrentTimer);
}