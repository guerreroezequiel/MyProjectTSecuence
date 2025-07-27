// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiCoreFragment.h"
#include "ZombiCombatFragment.h"
#include "ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuenceCharacter.h"

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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiBehaviorProcessor: Procesador de IA inicializado"));
        bLoggedConstructor = true;
    }
}

void UZombiBehaviorProcessor::ConfigureQueries()
{
    // Query optimizada para comportamiento - solo fragmentos necesarios
    BehaviorQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
    BehaviorQuery.AddRequirement<FZombiCombatFragment>(EMassFragmentAccess::ReadOnly);
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
        TArrayView<const FZombiCombatFragment> CombatFragments = Context.GetFragmentView<FZombiCombatFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];
            const FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiCombatFragment& CombatFragment = CombatFragments[i];

            // Solo procesar si está vivo
            if (!BehaviorFragment.IsDead())
            {
                // Actualizar timers de comportamiento
                UpdateActionTimers(BehaviorFragment, DeltaTime);

                // Actualizar estado de comportamiento
                UpdateBehaviorState(BehaviorFragment, CoreFragment, CombatFragment, DeltaTime);

                // Actualizar comportamiento de horda
                UpdateHordeBehavior(BehaviorFragment, CoreFragment, DeltaTime);
            }
        } });
}

void UZombiBehaviorProcessor::UpdateBehaviorState(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, const FZombiCombatFragment &CombatFragment, float DeltaTime)
{
    // Actualizar timer del estado actual
    BehaviorFragment.StateTimer += DeltaTime;

    // Lógica de transición de estados basada en condiciones
    EZombiState CurrentState = BehaviorFragment.GetState();

    // Verificar si está muerto
    if (CombatFragment.IsDead())
    {
        if (CurrentState != EZombiState::Dead)
        {
            BehaviorFragment.SetState(EZombiState::Dead);
            BehaviorFragment.ClearAllActions();
        }
        return;
    }

    // Verificar si está recibiendo daño
    if (CombatFragment.LastDamageTime < 0.5f) // Acaba de recibir daño
    {
        if (CurrentState != EZombiState::TakeDamage)
        {
            BehaviorFragment.SetState(EZombiState::TakeDamage);
            BehaviorFragment.SetDamagedAction(true);
        }
        return;
    }

    // Verificar si está atacando
    if (BehaviorFragment.IsAttackingAction())
    {
        if (CurrentState != EZombiState::Attack)
        {
            BehaviorFragment.SetState(EZombiState::Attack);
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

    // Actualizar timer general de comportamiento
    BehaviorFragment.BehaviorTimer += DeltaTime;
}