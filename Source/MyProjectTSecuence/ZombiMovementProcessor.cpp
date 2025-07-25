// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMovementProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

// Constructor del procesador de movimiento
UZombiMovementProcessor::UZombiMovementProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;
}

// Configura el query para requerir los fragmentos de movimiento y estado
void UZombiMovementProcessor::ConfigureQueries()
{
    MovementQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
}

// Ejecuta el procesamiento de movimiento para todas las entidades
void UZombiMovementProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa todas las entidades que cumplen el query
    MovementQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                     {
        const TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        const TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            FZombiStateFragment& StateFragment = StateFragments[i];

            // Solo procesa movimiento si el zombi no está muerto
            if (StateFragment.State != EZombiState::Death)
            {
                // Actualiza el timer de cambio de dirección
                MovementFragment.DirectionChangeTimer += DeltaTime;

                // Cambia dirección aleatoriamente
                if (MovementFragment.DirectionChangeTimer >= MovementFragment.DirectionChangeInterval)
                {
                    MovementFragment.MovementDirection = GenerateRandomDirection();
                    MovementFragment.DirectionChangeTimer = 0.0f;
                }

                // Calcula el movimiento
                FVector NewPosition = MovementFragment.Position + 
                    MovementFragment.MovementDirection * MovementFragment.MovementSpeed * DeltaTime;

                // Mantiene al zombi dentro del área de movimiento
                NewPosition = ClampToMovementArea(NewPosition, MovementFragment.MovementCenter, MovementFragment.MovementRadius);

                // Actualiza la posición
                MovementFragment.Position = NewPosition;

                // Calcula la rotación hacia la dirección de movimiento
                if (!MovementFragment.MovementDirection.IsNearlyZero())
                {
                    FRotator TargetRotation = MovementFragment.MovementDirection.Rotation();
                    FRotator CurrentRotation = MovementFragment.Rotation;

                    // Interpola suavemente la rotación
                    MovementFragment.Rotation = FMath::RInterpTo(
                        CurrentRotation, 
                        TargetRotation, 
                        DeltaTime, 
                        MovementFragment.RotationSpeed / 90.0f // Normaliza la velocidad de rotación
                    );
                }

                // Actualiza el estado según si se está moviendo o no
                FVector MovementDelta = NewPosition - MovementFragment.Position;
                if (MovementDelta.Size() > 1.0f) // Si se movió más de 1 unidad
                {
                    if (StateFragment.State != EZombiState::Walk)
                    {
                        StateFragment.State = EZombiState::Walk;
                    }
                }
                else
                {
                    if (StateFragment.State != EZombiState::Idle)
                    {
                        StateFragment.State = EZombiState::Idle;
                    }
                }

                // Actualiza la instancia visual de TurboSequence
                UpdateTurboSequenceInstance(MovementFragment);
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

// Verifica si el zombi está dentro del radio de movimiento
bool UZombiMovementProcessor::IsWithinMovementRadius(const FVector &Position, const FVector &Center, float Radius) const
{
    return FVector::Dist2D(Position, Center) <= Radius;
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

// Crea o actualiza la instancia visual de TurboSequence
void UZombiMovementProcessor::UpdateTurboSequenceInstance(const FZombiMovementFragment &MovementFragment)
{
    // Por ahora, solo actualizamos la transformación si ya existe una instancia
    // La creación de instancias se manejará en un procesador separado o en el spawner
    // Esto evita problemas con la API de TurboSequence y mejora el rendimiento

    // TODO: Implementar la sincronización con TurboSequence cuando sea necesario
    // Por ahora, solo mantenemos los datos de movimiento en el fragmento
}
