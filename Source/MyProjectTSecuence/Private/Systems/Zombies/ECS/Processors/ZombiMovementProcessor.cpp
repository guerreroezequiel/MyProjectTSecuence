// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiMovementProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"

UZombiMovementProcessor::UZombiMovementProcessor()
{
    // Configuración para registro automático en UE5.5.4 - IDÉNTICA a los otros procesadores
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

void UZombiMovementProcessor::ConfigureQueries()
{
    // Query optimizada para movimiento - Enfoque híbrido
    // Solo entidades activas, no muertas y en frustum
    MovementQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);

    // Tags base para enfoque híbrido
    MovementQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    MovementQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Registrar query
    MovementQuery.RegisterWithProcessor(*this);
}

void UZombiMovementProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa entidades activas para movimiento
    MovementQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                     {
        TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Solo procesar movimiento si no está muerto
            if (!BehaviorFragment.IsDead())
            {
                // Procesar movimiento aleatorio solo si no está persiguiendo
                if (!BehaviorFragment.IsChasing())
                {
                    // Actualizar timer de cambio de dirección
                    CoreFragment.BehaviorTimer += DeltaTime;

                    // Cambiar dirección aleatoriamente
                    if (CoreFragment.BehaviorTimer >= CoreFragment.DirectionChangeInterval)
                    {
                        CoreFragment.MovementDirection = GenerateRandomDirection();
                        CoreFragment.BehaviorTimer = 0.0f;

                                            // Cambiar velocidad aleatoriamente
                    float SpeedVariation = FMath::RandRange(0.0f, 1.0f);
                    if (SpeedVariation < 0.3f)
                    {
                        CoreFragment.MovementSpeed = 0.0f;
                    }
                    else if (SpeedVariation < 0.7f)
                    {
                        CoreFragment.MovementSpeed = 25.0f;
                    }
                    else
                    {
                        CoreFragment.MovementSpeed = 80.0f;
                    }
                    }
                }

                // Procesar rotación hacia la dirección de movimiento
                if (!CoreFragment.MovementDirection.IsNearlyZero())
                {
                    if (CoreFragment.MovementSpeed > 1.0f)
                    {
                        FRotator TargetRotation = CoreFragment.MovementDirection.Rotation();
                        FRotator CurrentRotation = CoreFragment.Rotation;

                        // Rotación más agresiva si está persiguiendo
                        float RotationSpeedMultiplier = BehaviorFragment.IsChasing() ? 3.0f : 1.0f;

                        // Interpola suavemente la rotación
                        CoreFragment.Rotation = FMath::RInterpTo(
                            CurrentRotation,
                            TargetRotation,
                            DeltaTime,
                            (CoreFragment.RotationSpeed * RotationSpeedMultiplier) / 180.0f
                        );
                    }
                }

                // Procesar movimiento hacia adelante
                if (CoreFragment.MovementSpeed > 0.0f)
                {
                    FVector ForwardDirection = CoreFragment.Rotation.Vector();
                    CoreFragment.Position += ForwardDirection * CoreFragment.MovementSpeed * DeltaTime;
                }

                // Aplicar restricciones de área solo si no está persiguiendo
                if (!BehaviorFragment.IsChasing())
                {
                    CoreFragment.Position = ClampToMovementArea(CoreFragment.Position, CoreFragment.MovementCenter, CoreFragment.MovementRadius);
                }
            }
        } });
}

// Genera una dirección aleatoria para el movimiento
FVector UZombiMovementProcessor::GenerateRandomDirection() const
{
    // Genera un ángulo aleatorio en el plano XZ (horizontal)
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    float Radians = FMath::DegreesToRadians(RandomAngle);

    return FVector(FMath::Cos(Radians), FMath::Sin(Radians), 0.0f).GetSafeNormal();
}

// Ajusta la posición para mantener al zombi dentro del área
FVector UZombiMovementProcessor::ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const
{
    FVector Direction = Position - Center;
    Direction.Z = 0.0f; // Mantiene la altura original

    if (Direction.Size2D() > Radius)
    {
        Direction = Direction.GetSafeNormal2D() * Radius;
        return Center + Direction;
    }

    return Position;
}
