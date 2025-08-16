#include "Systems/Zombies/ECS/Processors/ZombiUltraConsolidatedProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Animation/AnimationAsset.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"

UZombiUltraConsolidatedProcessor::UZombiUltraConsolidatedProcessor()
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
        UE_LOG(LogTemp, Log, TEXT("🚀 ZombiUltraConsolidatedProcessor: Procesador ultra-consolidado inicializado para 10,000+ entidades"));
        bLoggedConstructor = true;
    }
}

void UZombiUltraConsolidatedProcessor::ConfigureQueries()
{
    // UN SOLO QUERY = TODAS LAS ENTIDADES
    ZombiQuery.AddRequirement<FZombiUltraConsolidatedFragment>(EMassFragmentAccess::ReadWrite);
    ZombiQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    ZombiQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

void UZombiUltraConsolidatedProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Obtener posición del jugador (solo una vez por frame)
    FVector PlayerLocation = FVector::ZeroVector;
    if (APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
    {
        PlayerLocation = PlayerPawn->GetActorLocation();
    }

    // Procesa TODAS las entidades en un solo procesador
    ZombiQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime, CurrentTime, PlayerLocation](FMassExecutionContext &Context)
                                  {
        TArrayView<FZombiUltraConsolidatedFragment> ZombiFragments = Context.GetMutableFragmentView<FZombiUltraConsolidatedFragment>();
        const int32 NumEntities = Context.GetNumEntities();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiUltraConsolidatedFragment& Zombi = ZombiFragments[i];

            // Solo procesar si necesita update
            if (!Zombi.NeedsUpdate(CurrentTime))
            {
                continue;
            }

            // Procesar zombi completo
            ProcessZombi(Zombi, DeltaTime, PlayerLocation);

            // Actualizar tiempo de último update
            Zombi.LastUpdateTime = CurrentTime;
        } });
}

void UZombiUltraConsolidatedProcessor::ProcessZombi(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime, const FVector &PlayerLocation)
{
    // Guardar estado anterior para detectar cambios
    uint8 PreviousState = Zombi.State;

    // Calcular distancia al jugador
    Zombi.DistanceToPlayer = GetDistanceToPlayer(Zombi.Position, PlayerLocation);

    // Solo procesar si está en rango
    if (!Zombi.IsInRange())
    {
        return;
    }

    // Actualizar prioridad basada en distancia
    Zombi.UpdatePriority = CalculatePriority(Zombi.DistanceToPlayer);
    Zombi.UpdateInterval = CalculateUpdateInterval(Zombi.UpdatePriority);

    // Procesar según estado
    switch (Zombi.State)
    {
    case ZombiStates::IDLE:
        ProcessIdleState(Zombi, DeltaTime);
        break;
    case ZombiStates::WALK:
        ProcessWalkState(Zombi, DeltaTime);
        break;
    case ZombiStates::CHASE:
        ProcessChaseState(Zombi, DeltaTime, PlayerLocation);
        break;
    case ZombiStates::ATTACK:
        ProcessAttackState(Zombi, DeltaTime);
        break;
    case ZombiStates::DEAD:
        // No procesar zombis muertos
        return;
    }

    // Actualizar cooldowns
    UpdateCooldowns(Zombi, DeltaTime);

    // Procesar comportamiento
    ProcessBehavior(Zombi, DeltaTime, PlayerLocation);

    // Actualizar transformación
    UpdateTransform(Zombi);

    // Actualizar animación si el estado cambió
    UpdateAnimation(Zombi, PreviousState);
}

void UZombiUltraConsolidatedProcessor::ProcessIdleState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime)
{
    // Cambiar a walk después de un tiempo
    Zombi.BehaviorTimer += DeltaTime;
    if (Zombi.BehaviorTimer > 2.0f)
    {
        Zombi.State = ZombiStates::WALK;
        Zombi.BehaviorTimer = 0.0f;
        Zombi.MovementDirection = GetRandomDirection();
    }
}

void UZombiUltraConsolidatedProcessor::ProcessWalkState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime)
{
    // Verificar si debe perseguir al jugador
    if (ShouldChase(Zombi.DistanceToPlayer))
    {
        Zombi.State = ZombiStates::CHASE;
        Zombi.SetChasing(true);
        Zombi.MovementSpeed = RUN_SPEED;
        return;
    }

    // Movimiento aleatorio
    Zombi.BehaviorTimer += DeltaTime;
    if (Zombi.BehaviorTimer > Zombi.DirectionChangeInterval)
    {
        Zombi.MovementDirection = GetRandomDirection();
        Zombi.BehaviorTimer = 0.0f;
    }

    // Mover
    FVector NewPosition = Zombi.Position + (Zombi.MovementDirection * Zombi.MovementSpeed * DeltaTime);

    // Mantener dentro del área de movimiento
    if (FVector::Dist(NewPosition, FVector::ZeroVector) > MOVEMENT_RADIUS)
    {
        NewPosition = NewPosition.GetSafeNormal() * MOVEMENT_RADIUS;
    }

    Zombi.Position = NewPosition;

    // Rotar hacia la dirección de movimiento
    if (!Zombi.MovementDirection.IsNearlyZero())
    {
        FRotator TargetRotation = Zombi.MovementDirection.Rotation();
        Zombi.Rotation = FMath::RInterpTo(Zombi.Rotation, TargetRotation, DeltaTime, ROTATION_SPEED);
    }
}

void UZombiUltraConsolidatedProcessor::ProcessChaseState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime, const FVector &PlayerLocation)
{
    // Verificar si debe atacar
    if (ShouldAttack(Zombi.DistanceToPlayer))
    {
        Zombi.State = ZombiStates::ATTACK;
        Zombi.SetAttacking(true);
        return;
    }

    // Perseguir al jugador
    FVector DirectionToPlayer = (PlayerLocation - Zombi.Position).GetSafeNormal();
    Zombi.MovementDirection = DirectionToPlayer;

    // Mover hacia el jugador
    FVector NewPosition = Zombi.Position + (DirectionToPlayer * Zombi.MovementSpeed * DeltaTime);
    Zombi.Position = NewPosition;

    // Rotar hacia el jugador
    if (!DirectionToPlayer.IsNearlyZero())
    {
        FRotator TargetRotation = DirectionToPlayer.Rotation();
        Zombi.Rotation = FMath::RInterpTo(Zombi.Rotation, TargetRotation, DeltaTime, ROTATION_SPEED);
    }
}

void UZombiUltraConsolidatedProcessor::ProcessAttackState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime)
{
    // Simular ataque
    Zombi.BehaviorTimer += DeltaTime;
    if (Zombi.BehaviorTimer > 1.0f) // Duración del ataque
    {
        Zombi.State = ZombiStates::CHASE;
        Zombi.SetAttacking(false);
        Zombi.BehaviorTimer = 0.0f;
    }
}

void UZombiUltraConsolidatedProcessor::ProcessBehavior(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime, const FVector &PlayerLocation)
{
    // Simular daño aleatorio (solo para zombis cercanos)
    if (Zombi.DistanceToPlayer < 200.0f && Zombi.CanTakeDamage())
    {
        static float DamageTimer = 0.0f;
        DamageTimer += DeltaTime;

        if (DamageTimer > 15.0f) // Daño cada 15 segundos
        {
            // Simular daño
            Zombi.Health = FMath::Max(0, Zombi.Health - 20);
            Zombi.SetDamaged(true);
            Zombi.DamageCooldown = 1.0f;

            // Verificar si murió
            if (Zombi.Health <= 0)
            {
                Zombi.SetDead();
                return;
            }

            DamageTimer = 0.0f;
        }
    }

    // Simular ataque aleatorio
    if (Zombi.DistanceToPlayer < 150.0f && Zombi.CanAttack())
    {
        static float AttackTimer = 0.0f;
        AttackTimer += DeltaTime;

        if (AttackTimer > 8.0f) // Ataque cada 8 segundos
        {
            Zombi.SetAttacking(true);
            Zombi.AttackCooldown = 2.0f;
            AttackTimer = 0.0f;
        }
    }
}

void UZombiUltraConsolidatedProcessor::UpdateCooldowns(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime)
{
    // Actualizar cooldowns
    Zombi.AttackCooldown = FMath::Max(0.0f, Zombi.AttackCooldown - DeltaTime);
    Zombi.DamageCooldown = FMath::Max(0.0f, Zombi.DamageCooldown - DeltaTime);
    Zombi.LastDamageTime += DeltaTime;

    // Limpiar flags de daño después de un tiempo
    if (Zombi.LastDamageTime > 0.5f)
    {
        Zombi.SetDamaged(false);
    }
}

void UZombiUltraConsolidatedProcessor::UpdateAnimation(FZombiUltraConsolidatedFragment &Zombi, uint8 PreviousState)
{
    // Solo actualizar animación si el estado cambió y la instancia visual es válida
    if (PreviousState == Zombi.State || !Zombi.IsMeshDataValid())
    {
        return;
    }

    // Obtener el asset de TurboSequence desde el spawner subsystem
    UZombiSpawnerSubsystem *SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();
    if (!SpawnerSubsystem || !SpawnerSubsystem->ZombiTurboSequenceAsset)
    {
        return;
    }

    UTurboSequence_MeshAsset_Lf *Asset = SpawnerSubsystem->ZombiTurboSequenceAsset;
    UAnimationAsset *AnimationToPlay = nullptr;

    // Seleccionar animación según el estado
    switch (Zombi.State)
    {
    case ZombiStates::IDLE:
        // Usar animación por defecto para idle
        AnimationToPlay = Asset->OverrideDefaultAnimation;
        break;

    case ZombiStates::WALK:
        // Buscar animación de walk en la biblioteca
        if (Asset->AnimationLibrary && Asset->AnimationLibrary->Animations.Num() > 0)
        {
            for (int32 i = 0; i < Asset->AnimationLibrary->Animations.Num(); ++i)
            {
                const FAnimationLibraryItem_Lf &AnimItem = Asset->AnimationLibrary->Animations[i];
                if (AnimItem.Animation && (AnimItem.Animation->GetName().Contains(TEXT("Walk"), ESearchCase::IgnoreCase) ||
                                           AnimItem.Animation->GetName().Contains(TEXT("MM_Walk"), ESearchCase::IgnoreCase)))
                {
                    AnimationToPlay = AnimItem.Animation;
                    break;
                }
            }
        }
        break;

    case ZombiStates::CHASE:
        // Buscar animación de run en la biblioteca
        if (Asset->AnimationLibrary && Asset->AnimationLibrary->Animations.Num() > 0)
        {
            for (int32 i = 0; i < Asset->AnimationLibrary->Animations.Num(); ++i)
            {
                const FAnimationLibraryItem_Lf &AnimItem = Asset->AnimationLibrary->Animations[i];
                if (AnimItem.Animation && (AnimItem.Animation->GetName().Contains(TEXT("Run"), ESearchCase::IgnoreCase) ||
                                           AnimItem.Animation->GetName().Contains(TEXT("MM_Run"), ESearchCase::IgnoreCase)))
                {
                    AnimationToPlay = AnimItem.Animation;
                    break;
                }
            }
        }
        break;

    case ZombiStates::ATTACK:
        // Buscar animación de attack en la biblioteca
        if (Asset->AnimationLibrary && Asset->AnimationLibrary->Animations.Num() > 0)
        {
            for (int32 i = 0; i < Asset->AnimationLibrary->Animations.Num(); ++i)
            {
                const FAnimationLibraryItem_Lf &AnimItem = Asset->AnimationLibrary->Animations[i];
                if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Attack"), ESearchCase::IgnoreCase))
                {
                    AnimationToPlay = AnimItem.Animation;
                    break;
                }
            }
        }
        break;
    }

    // Reproducir la animación si se encontró una
    if (AnimationToPlay)
    {
        FTurboSequence_AnimPlaySettings_Lf AnimSettings;
        // Los campos se configuran automáticamente por TurboSequence

        ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
            Zombi.MeshData,
            Cast<UAnimSequence>(AnimationToPlay),
            AnimSettings);

        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiUltraConsolidatedProcessor: Cambiando animación a %s para estado %d"),
               *AnimationToPlay->GetName(), Zombi.State);
    }
}
