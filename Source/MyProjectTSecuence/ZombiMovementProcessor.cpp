// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMovementProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "ZombiTurboSequenceFragment.h"

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
    MovementQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
}

// Ejecuta el procesamiento de movimiento para todas las entidades
void UZombiMovementProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa todas las entidades que cumplen el query
    MovementQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                     {
        const TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        const TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        const float DeltaTime = Context.GetDeltaTimeSeconds();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiMovementFragment &MovementFragment = MovementFragments[i];
            FZombiStateFragment &StateFragment = StateFragments[i];

            // Solo procesa movimiento si el zombi no está muerto
            if (StateFragment.State != EZombiState::Death)
            {
                // Actualiza el timer de cambio de dirección
                MovementFragment.DirectionChangeTimer += DeltaTime;

                // Cambia dirección aleatoriamente y velocidad para probar Blend Space
                if (MovementFragment.DirectionChangeTimer >= MovementFragment.DirectionChangeInterval)
                {
                    // Guardar la dirección anterior para debugging
                    FVector OldDirection = MovementFragment.MovementDirection;

                    MovementFragment.MovementDirection = GenerateRandomDirection();
                    MovementFragment.DirectionChangeTimer = 0.0f;

                    // Cambiar velocidad aleatoriamente para probar Blend Space
                    float SpeedVariation = FMath::RandRange(0.0f, 1.0f);
                    // Usar velocidades fijas para animaciones más predecibles
                    if (SpeedVariation < 0.3f)
                    {
                        MovementFragment.MovementSpeed = 0.0f; // IDLE - velocidad fija
                    }
                    else if (SpeedVariation < 0.7f)
                    {
                        MovementFragment.MovementSpeed = 25.0f; // WALK - velocidad fija
                    }
                    else
                    {
                        MovementFragment.MovementSpeed = 80.0f; // RUN - velocidad fija
                    }

                    // Log para verificar cambios de dirección y velocidad
                    UE_LOG(LogTemp, Log, TEXT("🎮 Cambio de Dirección: Antigua: %s, Nueva: %s, Velocidad: %.2f"),
                           *OldDirection.ToString(), *MovementFragment.MovementDirection.ToString(), MovementFragment.MovementSpeed);
                }

                // PRIMERO: Calcula la rotación hacia la dirección de movimiento deseada
                if (!MovementFragment.MovementDirection.IsNearlyZero())
                {
                    FRotator TargetRotation = MovementFragment.MovementDirection.Rotation();
                    FRotator CurrentRotation = MovementFragment.Rotation;

                    // Si acaba de cambiar dirección, usar rotación más agresiva
                    float RotationSpeedMultiplier = 1.0f;
                    if (MovementFragment.DirectionChangeTimer < 0.5f) // Si cambió dirección recientemente
                    {
                        RotationSpeedMultiplier = 3.0f; // Rotación más rápida
                    }

                    // Interpola suavemente la rotación con velocidad aumentada
                    MovementFragment.Rotation = FMath::RInterpTo(
                        CurrentRotation,
                        TargetRotation,
                        DeltaTime,
                        (MovementFragment.RotationSpeed * RotationSpeedMultiplier) / 180.0f // Velocidad de rotación más rápida
                    );

                    // Log detallado para debugging de rotación
                    static float DetailedRotationDebugTimer = 0.0f;
                    DetailedRotationDebugTimer += DeltaTime;
                    if (DetailedRotationDebugTimer >= 8.0f && MovementFragment.MovementSpeed > 0.0f)
                    {
                        UE_LOG(LogTemp, Log, TEXT("🎮 Rotación Detallada: Velocidad: %.2f, Dirección: %s, Actual: %s, Objetivo: %s, Nueva: %s, Multiplicador: %.1f"),
                               MovementFragment.MovementSpeed,
                               *MovementFragment.MovementDirection.ToString(),
                               *CurrentRotation.ToString(),
                               *TargetRotation.ToString(),
                               *MovementFragment.Rotation.ToString(),
                               RotationSpeedMultiplier);
                        DetailedRotationDebugTimer = 0.0f;
                    }

                    // Log de debugging para rotación (solo ocasionalmente)
                    static float MovementRotationDebugTimer = 0.0f;
                    MovementRotationDebugTimer += DeltaTime;
                    if (MovementRotationDebugTimer >= 10.0f && MovementFragment.MovementSpeed > 0.0f)
                    {
                        UE_LOG(LogTemp, Log, TEXT("🎮 MovementProcessor Rotación: Velocidad: %.2f, Dirección: %s, Rotación: %s"),
                               MovementFragment.MovementSpeed,
                               *MovementFragment.MovementDirection.ToString(),
                               *MovementFragment.Rotation.ToString());
                        MovementRotationDebugTimer = 0.0f;
                    }
                }

                // SEGUNDO: Calcula el movimiento hacia adelante en la dirección de la rotación
                FVector NewPosition = MovementFragment.Position;
                if (MovementFragment.MovementSpeed > 0.0f)
                {
                    // Mover hacia adelante en la dirección de la rotación actual
                    // Usar Vector() para obtener la dirección hacia adelante del zombi
                    FVector ForwardDirection = MovementFragment.Rotation.Vector();
                    NewPosition = MovementFragment.Position +
                                  ForwardDirection * MovementFragment.MovementSpeed * DeltaTime;

                    // Log de movimiento hacia adelante
                    static float ForwardMovementDebugTimer = 0.0f;
                    ForwardMovementDebugTimer += DeltaTime;
                    if (ForwardMovementDebugTimer >= 12.0f && MovementFragment.MovementSpeed > 0.0f)
                    {
                        UE_LOG(LogTemp, Log, TEXT("🎮 Movimiento Hacia Adelante: Velocidad: %.2f, Rotación: %s, Dirección Adelante: %s, Posición: %s, Dirección Deseada: %s"),
                               MovementFragment.MovementSpeed,
                               *MovementFragment.Rotation.ToString(),
                               *ForwardDirection.ToString(),
                               *MovementFragment.Position.ToString(),
                               *MovementFragment.MovementDirection.ToString());
                        ForwardMovementDebugTimer = 0.0f;
                    }
                }

                // Mantiene al zombi dentro del área de movimiento
                NewPosition = ClampToMovementArea(NewPosition, MovementFragment.MovementCenter, MovementFragment.MovementRadius);

                // Actualiza la posición
                MovementFragment.Position = NewPosition;
            }

            // Actualiza el estado según si se está moviendo o no
            if (MovementFragment.MovementSpeed > 0.0f) // Si tiene velocidad, está caminando
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
