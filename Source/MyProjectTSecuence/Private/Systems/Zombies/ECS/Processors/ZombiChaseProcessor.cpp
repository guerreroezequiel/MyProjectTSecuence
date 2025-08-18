// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiChaseProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UZombiChaseProcessor::UZombiChaseProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    // ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Behavior); // Removido - no existe en UE5.5

    CachedPlayerLocation = FVector::ZeroVector;
    LastPlayerLocationUpdate = 0.0f;
}

void UZombiChaseProcessor::ConfigureQueries()
{
    // Query especializado para entidades persiguiendo
    ChaseQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    ChaseQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    ChaseQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    ChaseQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadOnly);

    // Tags para filtrado DOP
    ChaseQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    ChaseQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    ChaseQuery.RegisterWithProcessor(*this);
}

void UZombiChaseProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Solo procesar entidades que están en estado de persecución
    ChaseQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                  {
        const int32 NumEntities = Context.GetNumEntities();
        auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        auto TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StimuliFragments = Context.GetFragmentView<FZombiStimuliFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiStateFragment& StateFragment = StateFragments[i];
            const FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStimuliFragment& StimuliFragment = StimuliFragments[i];

            // Solo procesar si realmente está persiguiendo
            if (StateFragment.IsChasing())
            {
                ProcessChaseLogic(StateFragment, TransformFragment, MovementFragment, StimuliFragment, DeltaTime);
            }
        } });
}

void UZombiChaseProcessor::ProcessChaseLogic(FZombiStateFragment &StateFragment,
                                             const FZombiTransformFragment &TransformFragment,
                                             FZombiMovementFragment &MovementFragment,
                                             const FZombiStimuliFragment &StimuliFragment,
                                             float DeltaTime)
{
    // Calcular dirección hacia el objetivo
    FVector ChaseDirection = this->CalculateChaseDirection(TransformFragment.GetPosition(), StimuliFragment);

    if (ChaseDirection.IsZero())
    {
        // No hay objetivo válido, parar persecución
        MovementFragment.Stop();
        return;
    }

    // Calcular distancia al objetivo
    FVector PlayerLocation = this->CachedPlayerLocation;
    if (StimuliFragment.HasPlayerStimulus())
    {
        PlayerLocation = StimuliFragment.StrongestStimulus.Position;
    }

    float DistanceToTarget = FVector::Dist(TransformFragment.GetPosition(), PlayerLocation);

    // Verificar si debe continuar persiguiendo
    if (!this->ShouldContinueChasing(StateFragment, StimuliFragment, DistanceToTarget))
    {
        // Dejar de perseguir - BehaviorProcessor manejará la transición de estado
        MovementFragment.Stop();
        return;
    }

    // Calcular velocidad de persecución
    uint8 ChaseSpeed = this->CalculateChaseSpeed(DistanceToTarget, StimuliFragment);

    // Aplicar movimiento de persecución
    MovementFragment.StartChasing(ChaseDirection, ChaseSpeed);

    // Sin logs en hot-path
}

FVector UZombiChaseProcessor::CalculateChaseDirection(const FVector &ZombiePosition,
                                                      const FZombiStimuliFragment &StimuliFragment)
{
    // Prioridad 1: Estímulo del jugador
    if (StimuliFragment.HasPlayerStimulus())
    {
        return (StimuliFragment.StrongestStimulus.Position - ZombiePosition).GetSafeNormal();
    }

    // Prioridad 2: Cualquier estímulo disponible
    if (StimuliFragment.HasAnyStimulus())
    {
        return StimuliFragment.StimulusDirection;
    }

    // Fallback: Obtener posición del jugador directamente
    if (APawn *PlayerCharacter = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        CachedPlayerLocation = PlayerCharacter->GetActorLocation();
        LastPlayerLocationUpdate = GetWorld()->GetTimeSeconds();
        return (CachedPlayerLocation - ZombiePosition).GetSafeNormal();
    }

    return FVector::ZeroVector;
}

uint8 UZombiChaseProcessor::CalculateChaseSpeed(float DistanceToTarget, const FZombiStimuliFragment &StimuliFragment)
{
    // Velocidad base de persecución
    uint8 BaseSpeed = 150;

    // Ajustar velocidad según distancia
    if (DistanceToTarget > 500.0f)
    {
        return BaseSpeed; // Velocidad máxima para distancias largas
    }
    else if (DistanceToTarget > 200.0f)
    {
        return static_cast<uint8>(BaseSpeed * 0.8f); // Velocidad media
    }
    else if (DistanceToTarget > 100.0f)
    {
        return static_cast<uint8>(BaseSpeed * 0.6f); // Velocidad reducida
    }
    else
    {
        return static_cast<uint8>(BaseSpeed * 0.4f); // Velocidad mínima cerca del objetivo
    }
}

bool UZombiChaseProcessor::ShouldContinueChasing(const FZombiStateFragment &StateFragment,
                                                 const FZombiStimuliFragment &StimuliFragment,
                                                 float DistanceToTarget)
{
    // Parar si está demasiado cerca (rango de ataque)
    if (DistanceToTarget < 80.0f)
    {
        return false; // Cambiar a attack state o idle
    }

    // Parar si no hay estímulos válidos
    if (!StimuliFragment.HasPlayerStimulus() && !StimuliFragment.HasAnyStimulus())
    {
        return false;
    }

    // Parar si ha estado persiguiendo demasiado tiempo
    if (StateFragment.GetStateTimerSeconds() > 8.0f)
    {
        return false;
    }

    // Parar si está demasiado lejos
    if (DistanceToTarget > 1500.0f)
    {
        return false;
    }

    return true;
}
