// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
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
    // Query optimizada para comportamiento - Enfoque híbrido
    // Solo entidades activas y no muertas
    BehaviorQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);

    // Tags base para enfoque híbrido
    BehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    BehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Registrar query
    BehaviorQuery.RegisterWithProcessor(*this);
}

void UZombiBehaviorProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    // Obtener referencia al jugador (cached para performance)
    if (!PlayerPawn)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }

    if (!PlayerPawn)
    {
        return; // No hay jugador, no procesar
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa entidades activas para decisiones de IA
    BehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                     {
        TArrayView<FZombiBehaviorFragment> BehaviorFragments = Context.GetMutableFragmentView<FZombiBehaviorFragment>();
        TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];
            const FZombiCoreFragment& CoreFragment = CoreFragments[i];

            // Solo procesar si está vivo
            if (!BehaviorFragment.IsDead())
            {
                // Calcular distancia al jugador (simplificado)
                float DistanceToPlayer = CalculateDistanceToPlayer(CoreFragment.Position);
                BehaviorFragment.SetStateData(DistanceToPlayer);

                // Actualizar timers de comportamiento
                UpdateActionTimers(BehaviorFragment, DeltaTime);

                // Evaluar transiciones de estado (simplificado)
                EvaluateStateTransitions(BehaviorFragment, DistanceToPlayer);

                // Actualizar estado actual
                UpdateCurrentState(BehaviorFragment, DistanceToPlayer, DeltaTime);

                // Actualizar comportamiento de horda
                UpdateHordeBehavior(BehaviorFragment, CoreFragment, DeltaTime);
            }
        } });
}

void UZombiBehaviorProcessor::EvaluateStateTransitions(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer)
{
    // Lógica de transición de estados simplificada
    EZombiState CurrentState = BehaviorFragment.GetState();

    // Lógica de Seek y Chase basada en distancia
    if (DistanceToPlayer < 50.0f)
    {
        // Muy cerca del jugador - Attack (futuro)
        if (CurrentState != EZombiState::Attack)
        {
            BehaviorFragment.SetState(EZombiState::Chase); // Por ahora Chase, después Attack
        }
    }
    else if (DistanceToPlayer < 200.0f)
    {
        // Cerca del jugador - Chase
        if (CurrentState != EZombiState::Chase)
        {
            BehaviorFragment.SetState(EZombiState::Chase);
        }
    }
    else if (DistanceToPlayer < 800.0f)
    {
        // Jugador visible - Seek
        if (CurrentState != EZombiState::Seek)
        {
            BehaviorFragment.SetState(EZombiState::Seek);
        }
    }
    else
    {
        // Jugador muy lejos - comportamiento normal
        if (CurrentState == EZombiState::Chase || CurrentState == EZombiState::Seek)
        {
            // Perdió al jugador, volver a WalkAround
            BehaviorFragment.SetState(EZombiState::WalkAround);
            BehaviorFragment.SetStateTimer(0.0f);
        }
    }
}

void UZombiBehaviorProcessor::UpdateCurrentState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime)
{
    // Actualizar timer del estado actual
    float CurrentTimer = BehaviorFragment.GetStateTimer();
    CurrentTimer += DeltaTime;
    BehaviorFragment.SetStateTimer(CurrentTimer);

    // Lógica específica del estado actual
    EZombiState CurrentState = BehaviorFragment.GetState();

    switch (CurrentState)
    {
    case EZombiState::Chase:
        // Lógica de persecución
        UpdateChaseState(BehaviorFragment, DistanceToPlayer, DeltaTime);
        break;

    case EZombiState::Seek:
        // Lógica de búsqueda
        UpdateSeekState(BehaviorFragment, DistanceToPlayer, DeltaTime);
        break;

    case EZombiState::WalkAround:
        // Lógica de caminar aleatoriamente
        UpdateWalkAroundState(BehaviorFragment, DeltaTime);
        break;

    case EZombiState::Idle:
        // Lógica de idle (esperando)
        UpdateIdleState(BehaviorFragment, DeltaTime);
        break;

    default:
        break;
    }
}

void UZombiBehaviorProcessor::UpdateChaseState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime)
{
    // Verificar si debe dejar de perseguir (jugador muy lejos)
    if (DistanceToPlayer > 800.0f)
    {
        BehaviorFragment.SetState(EZombiState::WalkAround);
        BehaviorFragment.SetStateTimer(0.0f);
        return;
    }

    // Mantener persecución - el MovementProcessor se encargará del movimiento hacia el jugador
    // Los datos de distancia ya están guardados en StateData
}

void UZombiBehaviorProcessor::UpdateSeekState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime)
{
    // Verificar si debe dejar de buscar (jugador muy lejos)
    if (DistanceToPlayer > 800.0f)
    {
        BehaviorFragment.SetState(EZombiState::WalkAround);
        BehaviorFragment.SetStateTimer(0.0f);
        return;
    }

    // Mantener búsqueda - el MovementProcessor se encargará del movimiento hacia el jugador
    // Los datos de distancia ya están guardados en StateData
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

void UZombiBehaviorProcessor::UpdateIdleState(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    // Lógica de idle (esperando)
    float Timer = BehaviorFragment.GetStateTimer();

    // Después de 2 segundos en idle, cambiar a WalkAround
    if (Timer > 2.0f)
    {
        BehaviorFragment.SetState(EZombiState::WalkAround);
        BehaviorFragment.SetStateTimer(0.0f);
    }
}

void UZombiBehaviorProcessor::UpdateHordeBehavior(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, float DeltaTime)
{
    // TODO: Implementar lógica de horda cuando sea necesario
}

void UZombiBehaviorProcessor::UpdateActionTimers(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime)
{
    // Actualizar timer de acciones
    BehaviorFragment.ActionTimer += DeltaTime;

    // Limpiar acciones que han expirado
    if (BehaviorFragment.ActionTimer > 2.0f)
    {
        BehaviorFragment.ClearAllActions();
    }
}

float UZombiBehaviorProcessor::CalculateDistanceToPlayer(const FVector &ZombieLocation)
{
    if (!PlayerPawn)
    {
        return 9999.0f; // Distancia muy grande si no hay jugador
    }

    // Calcular distancia 2D (isométrico)
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector DistanceVector = ZombieLocation - PlayerLocation;
    DistanceVector.Z = 0.0f; // Ignorar altura para isométrico

    return DistanceVector.Size();
}