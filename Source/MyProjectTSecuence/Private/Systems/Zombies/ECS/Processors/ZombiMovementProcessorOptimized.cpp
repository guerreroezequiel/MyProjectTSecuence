// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiMovementProcessorOptimized.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"

UZombiMovementProcessorOptimized::UZombiMovementProcessorOptimized()
{
    // Configurar el procesador para ejecutarse en el grupo de movimiento

    ExecutionOrder.ExecuteAfter.Add(TEXT("MassBehavior"));
    ExecutionOrder.ExecuteBefore.Add(TEXT("MassTransform"));
}

void UZombiMovementProcessorOptimized::ConfigureQueries()
{
    // Query principal para entidades con movimiento
    MovementQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    MovementQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    MovementQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    MovementQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
    MovementQuery.RegisterWithProcessor(*this);

    // Query para entidades persiguiendo (alta prioridad)
    ChasingQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    ChasingQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    ChasingQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    ChasingQuery.AddTagRequirement<FChasingTag>(EMassFragmentPresence::All);
    ChasingQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    ChasingQuery.RegisterWithProcessor(*this);

    // Query para entidades caminando (prioridad media)
    WalkingQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    WalkingQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    WalkingQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    WalkingQuery.AddTagRequirement<FWalkingTag>(EMassFragmentPresence::All);
    WalkingQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    WalkingQuery.RegisterWithProcessor(*this);

    // Query para entidades inactivas (baja prioridad)
    IdleQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    IdleQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    IdleQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    IdleQuery.AddTagRequirement<FIdleTag>(EMassFragmentPresence::All);
    IdleQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    IdleQuery.RegisterWithProcessor(*this);
}

void UZombiMovementProcessorOptimized::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // NUEVO: Usar un solo query y procesar por StateFlags (DOP híbrido)
    MovementQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                     {
        const int32 NumEntities = Context.GetNumEntities();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            // Procesar según el estado actual usando flags
            if (StateFragment.IsChasing())
            {
                ApplyChasingMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
            }
            else if (StateFragment.IsWalking())
            {
                ApplyWalkingMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
            }
            else if (StateFragment.IsIdle())
            {
                ApplyIdleMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
            }
        } });
}

void UZombiMovementProcessorOptimized::ProcessChasingMovement(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float DeltaTime)
{
    // Procesar entidades persiguiendo con máxima frecuencia (60 FPS)
    ChasingQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                    {
        const int32 NumEntities = Context.GetNumEntities();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            ApplyChasingMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
        } });
}

void UZombiMovementProcessorOptimized::ProcessWalkingMovement(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float DeltaTime)
{
    // Procesar entidades caminando con frecuencia media (30 FPS)
    WalkingQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                    {
        const int32 NumEntities = Context.GetNumEntities();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            ApplyWalkingMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
        } });
}

void UZombiMovementProcessorOptimized::ProcessIdleMovement(FMassEntityManager &EntityManager, FMassExecutionContext &Context, float DeltaTime)
{
    // Procesar entidades inactivas con baja frecuencia (15 FPS)
    IdleQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                 {
        const int32 NumEntities = Context.GetNumEntities();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
        auto StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            ApplyIdleMovement(MovementFragment, TransformFragment, StateFragment, DeltaTime);
        } });
}

void UZombiMovementProcessorOptimized::ApplyChasingMovement(FZombiMovementFragment &MovementFragment,
                                                            FZombiTransformFragment &TransformFragment,
                                                            const FZombiStateFragment &StateFragment,
                                                            float DeltaTime)
{
    // SIMPLIFICADO: Solo aplicar movimiento físico, ChaseProcessor maneja la lógica
    if (MovementFragment.GetSpeed() > 0)
    {
        FVector Direction = MovementFragment.GetDirection();
        float Speed = MovementFragment.GetEffectiveSpeed();

        // Aplicar movimiento físico
        FVector Movement = Direction * Speed * DeltaTime;
        TransformFragment.AddPosition(Movement);

        // Aplicar rotación hacia la dirección de movimiento
        if (!Direction.IsZero())
        {
            float TargetYaw = Direction.Rotation().Yaw;
            float CurrentYaw = TransformFragment.GetYaw();
            float NewYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, 360.0f);
            TransformFragment.SetYaw(NewYaw);
        }

        // DEBUG: Log simplificado
        static int32 DebugCounter = 0;
        if (++DebugCounter % 120 == 0) // Cada 2 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🚶 MovementProcessor: Aplicando movimiento - Speed: %d, Dir: %s"),
                   static_cast<int32>(Speed), *Direction.ToString());
        }
    }
}

void UZombiMovementProcessorOptimized::ApplyWalkingMovement(FZombiMovementFragment &MovementFragment,
                                                            FZombiTransformFragment &TransformFragment,
                                                            const FZombiStateFragment &StateFragment,
                                                            float DeltaTime)
{
    // Validar movimiento
    if (!ValidateMovement(MovementFragment, TransformFragment))
    {
        FixInvalidMovement(MovementFragment, TransformFragment);
        return;
    }

    // Aplicar movimiento caminando
    if (MovementFragment.GetSpeed() > 0.0f)
    {
        FVector ForwardDirection = TransformFragment.GetForwardVector();
        TransformFragment.AddPosition(ForwardDirection * MovementFragment.GetSpeed() * DeltaTime);

        // Rotación suave hacia la dirección de movimiento
        if (MovementFragment.GetDirection().SizeSquared() > 0.0f)
        {
            float TargetYaw = MovementFragment.GetDirection().Rotation().Yaw;
            float CurrentYaw = TransformFragment.GetYaw();
            float NewYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, 120.0f);
            TransformFragment.SetYaw(NewYaw);
        }
    }
}

void UZombiMovementProcessorOptimized::ApplyIdleMovement(FZombiMovementFragment &MovementFragment,
                                                         FZombiTransformFragment &TransformFragment,
                                                         const FZombiStateFragment &StateFragment,
                                                         float DeltaTime)
{
    // Entidades inactivas - mínimo procesamiento
    // Solo validación básica y mantenimiento de posición
    if (!ValidateMovement(MovementFragment, TransformFragment))
    {
        FixInvalidMovement(MovementFragment, TransformFragment);
    }
}

FVector UZombiMovementProcessorOptimized::GetPlayerLocation() const
{
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Usar cache si está actualizado
    if (CurrentTime - LastPlayerLocationUpdate < PLAYER_LOCATION_CACHE_DURATION)
    {
        return CachedPlayerLocation;
    }

    // Actualizar cache
    CachedPlayerLocation = GetPlayerLocationFromStimulus();
    LastPlayerLocationUpdate = CurrentTime;

    return CachedPlayerLocation;
}

FVector UZombiMovementProcessorOptimized::GetPlayerLocationFromStimulus() const
{
    // Intentar obtener la posición del jugador del StimulusSubsystem
    if (UWorld *World = GetWorld())
    {
        if (UStimulusSubsystem *StimulusSubsystem = World->GetSubsystem<UStimulusSubsystem>())
        {
            const TArray<FStimulusData> &PlayerStimuli = StimulusSubsystem->GetPlayerStimuli();
            if (PlayerStimuli.Num() > 0)
            {
                return PlayerStimuli[0].Position;
            }
        }
    }

    // Fallback: obtener directamente del jugador
    if (AMyProjectTSecuenceCharacter *PlayerCharacter = Cast<AMyProjectTSecuenceCharacter>(
            UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        return PlayerCharacter->GetActorLocation();
    }

    return FVector::ZeroVector;
}

bool UZombiMovementProcessorOptimized::ValidateMovement(const FZombiMovementFragment &MovementFragment,
                                                        const FZombiTransformFragment &TransformFragment) const
{
    // Validar posición
    if (TransformFragment.GetPosition().ContainsNaN())
    {
        return false;
    }

    // Validar dirección
    if (MovementFragment.GetDirection().ContainsNaN())
    {
        return false;
    }

    // Validar velocidad
    if (MovementFragment.GetSpeed() > 255) // uint8 max
    {
        return false;
    }

    // Validar que la posición no esté muy lejos del origen
    if (TransformFragment.GetPosition().Size() > 10000.0f)
    {
        return false;
    }

    return true;
}

void UZombiMovementProcessorOptimized::FixInvalidMovement(FZombiMovementFragment &MovementFragment,
                                                          FZombiTransformFragment &TransformFragment) const
{
    // Corregir posición inválida
    if (TransformFragment.GetPosition().ContainsNaN())
    {
        TransformFragment.SetPosition(FVector::ZeroVector);
    }

    // Corregir dirección inválida
    if (MovementFragment.GetDirection().ContainsNaN())
    {
        MovementFragment.SetDirection(FVector::ForwardVector);
    }

    // Corregir velocidad inválida
    if (MovementFragment.GetSpeed() > 255)
    {
        MovementFragment.SetSpeed(static_cast<uint8>(50)); // Velocidad por defecto
    }

    // Corregir posición muy lejana
    if (TransformFragment.GetPosition().Size() > 10000.0f)
    {
        TransformFragment.SetPosition(FVector::ZeroVector);
    }
}
