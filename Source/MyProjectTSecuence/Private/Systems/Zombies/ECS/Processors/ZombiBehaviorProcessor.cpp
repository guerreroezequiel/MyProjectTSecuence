// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Systems/Zombies/ECS/Fragments/ZombiConfigFragment.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "MyProjectTSecuence/MyProjectTSecuenceCharacter.h"

UZombiBehaviorProcessor::UZombiBehaviorProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    ExecutionOrder.ExecuteAfter.Add(TEXT("StimulusProcessor")); // Explícito después de stimulus
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Log solo en la primera instancia
    static bool bLoggedConstructor = false;
    if (!bLoggedConstructor)
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: Constructor llamado - autoregister=%s"),
               bAutoRegisterWithProcessingPhases ? TEXT("true") : TEXT("false"));
        bLoggedConstructor = true;
    }
}

void UZombiBehaviorProcessor::ConfigureQueries()
{
    UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: ConfigureQueries llamado"));

    // Query optimizada para comportamiento - solo fragmentos necesarios
    BehaviorQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    BehaviorQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    BehaviorQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadOnly);
    // TEMPORAL: Comentamos shared requirement para debug
    // BehaviorQuery.AddSharedRequirement<FZombiConfigFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);

    // Tags para filtrado rápido
    BehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    BehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: Query configurada correctamente"));
}

void UZombiBehaviorProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetWorld()->HasBegunPlay())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Debug: Verificar si el procesador se está ejecutando
    static int32 ExecuteCounter = 0;
    if (++ExecuteCounter % 300 == 0) // Cada 5 segundos aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 BehaviorProcessor: EJECUTÁNDOSE | Execute count: %d | DeltaTime: %.3f"), ExecuteCounter, DeltaTime);
    }

    // Procesa entidades activas para decisiones de IA
    BehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                     {
                                         TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
                                         TArrayView<const FZombiTransformFragment> TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
                                         TArrayView<FZombiMovementFragment> MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();
                                         TArrayView<const FZombiStimuliFragment> StimuliFragments = Context.GetFragmentView<FZombiStimuliFragment>();
                                         // TEMPORAL: Usar valores hardcoded mientras debuggeamos
                                         // const FZombiConfigFragment& Config = Context.GetSharedFragment<FZombiConfigFragment>();
                                         const float DeltaTime = Context.GetDeltaTimeSeconds();

                                         // Log por chunk para debug
                                         static int32 DebugCounter = 0;
                                         if (++DebugCounter % 300 == 0) // Cada 5 segundos aprox
                                         {
                                             UE_LOG(LogTemp, Log, TEXT("🧠 Behavior Chunk: %d entidades | HARDCODED values for debug"),
                                                    Context.GetNumEntities());
                                         }

                                         for (int32 i = 0; i < Context.GetNumEntities(); ++i)
                                         {
                                             FZombiStateFragment &StateFragment = StateFragments[i];
                                             const FZombiTransformFragment &TransformFragment = TransformFragments[i];
                                             FZombiMovementFragment &MovementFragment = MovementFragments[i];
                                             const FZombiStimuliFragment &StimuliFragment = StimuliFragments[i];

                                             // Debug individual por entidad (menos frecuente)
                                             static int32 EntityDebugCounter = 0;
                                             const bool bShouldDebugEntity = (++EntityDebugCounter % 600 == 0); // Cada 10 segundos para una entidad

                                             if (bShouldDebugEntity && i == 0)
                                             {
                                                 const float IdleTimer = StateFragment.GetStateTimerSeconds();
                                                 UE_LOG(LogTemp, Log, TEXT("🧠 Entity[%d]: Idle=%s Walk=%s Chase=%s Dead=%s | IdleTimer=%.2f Required=%.2f"),
                                                        i, StateFragment.IsIdle() ? TEXT("✓") : TEXT("✗"),
                                                        StateFragment.IsWalking() ? TEXT("✓") : TEXT("✗"),
                                                        StateFragment.IsChasing() ? TEXT("✓") : TEXT("✗"),
                                                        StateFragment.IsDead() ? TEXT("✓") : TEXT("✗"),
                                                        IdleTimer, 2.0f);
                                             }

                                             // Solo procesar si está vivo
                                             if (!StateFragment.IsDead())
                                             {
                                                 // FORZAR estado Idle si no tiene ningún estado válido
                                                 if (!StateFragment.IsIdle() && !StateFragment.IsWalking() && !StateFragment.IsChasing())
                                                 {
                                                     StateFragment.SetToIdle();
                                                     UE_LOG(LogTemp, Warning, TEXT("🧠 Entity[%d]: FORZADO a Idle - no tenía estado válido"), i);
                                                 }

                                                 // Actualizar timers de comportamiento
                                                 UpdateActionTimers(StateFragment, DeltaTime);

                                                 // Evaluar transiciones de estado (DOP-compatible)
                                                 EvaluateStateTransitions(StateFragment, TransformFragment, MovementFragment, StimuliFragment);

                                                 // Actualizar estado actual
                                                 UpdateCurrentState(StateFragment, TransformFragment, MovementFragment, StimuliFragment, DeltaTime);

                                                 // Si está Idle, promover a WalkAround tras intervalo hardcoded
                                                 if (StateFragment.IsIdle())
                                                 {
                                                     const float IdleTimer = StateFragment.GetStateTimerSeconds();
                                                     if (IdleTimer > 2.0f) // Hardcoded 2 segundos
                                                     {
                                                         StateFragment.SetToWalking();
                                                         const FVector RandomDirection = FVector(
                                                                                             FMath::RandRange(-1.0f, 1.0f),
                                                                                             FMath::RandRange(-1.0f, 1.0f),
                                                                                             0.0f)
                                                                                             .GetSafeNormal();
                                                         MovementFragment.StartMoving(RandomDirection, 50); // Hardcoded 50 speed
                                                         UE_LOG(LogTemp, Log, TEXT("🧠 Behavior: Idle → WalkAround | Dir=%s Speed=%d"), *RandomDirection.ToString(), (int32)MovementFragment.GetSpeed());
                                                     }
                                                 }
                                                 else if (StateFragment.IsWalking())
                                                 {
                                                     const float WalkTimer = StateFragment.GetStateTimerSeconds();
                                                     if (WalkTimer > 4.0f) // Hardcoded 4 segundos
                                                     {
                                                         StateFragment.SetToIdle();
                                                         MovementFragment.Stop();
                                                         UE_LOG(LogTemp, Log, TEXT("🧠 Behavior: WalkAround → Idle"));
                                                     }
                                                 }

                                                 // Actualizar comportamiento de horda
                                                 UpdateHordeBehavior(StateFragment, TransformFragment, DeltaTime);
                                             }
                                         } // Cerrar el for loop
                                     }); // Cerrar el ForEachEntityChunk lambda

    // GESTIÓN AUTOMÁTICA DE TAGS - Ejecutar después de procesar todas las entidades
    UpdateEntityTags(EntityManager, Context);
}

void UZombiBehaviorProcessor::UpdateEntityTags(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // OPTIMIZACIÓN: Recopilar cambios primero, aplicar después (evita modificar arrays durante iteración)
    struct FTagChange
    {
        FMassEntityHandle Entity;
        bool bIsIdle = false;
        bool bIsWalking = false;
        bool bIsChasing = false;
    };

    TArray<FTagChange> TagChanges;
    TagChanges.Reserve(64); // Pre-allocate para performance

    // PASO 1: Recopilar todos los cambios necesarios
    BehaviorQuery.ForEachEntityChunk(EntityManager, Context, [&TagChanges](FMassExecutionContext &Context)
                                     {
        TArrayView<const FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
        
        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            const FZombiStateFragment& StateFragment = StateFragments[i];
            FMassEntityHandle Entity = Context.GetEntity(i);
            
            FTagChange& Change = TagChanges.AddDefaulted_GetRef();
            Change.Entity = Entity;
            Change.bIsIdle = StateFragment.IsIdle();
            Change.bIsWalking = StateFragment.IsWalking();
            Change.bIsChasing = StateFragment.IsChasing();
        } });

    // PASO 2: Aplicar todos los cambios fuera de la iteración
    for (const FTagChange &Change : TagChanges)
    {
        // Remover todas las tags de estado primero
        EntityManager.RemoveTagFromEntity(Change.Entity, FIdleTag::StaticStruct());
        EntityManager.RemoveTagFromEntity(Change.Entity, FWalkingTag::StaticStruct());
        EntityManager.RemoveTagFromEntity(Change.Entity, FChasingTag::StaticStruct());

        // Agregar tag correspondiente al estado actual
        if (Change.bIsIdle)
        {
            EntityManager.AddTagToEntity(Change.Entity, FIdleTag::StaticStruct());
        }
        else if (Change.bIsWalking)
        {
            EntityManager.AddTagToEntity(Change.Entity, FWalkingTag::StaticStruct());
        }
        else if (Change.bIsChasing)
        {
            EntityManager.AddTagToEntity(Change.Entity, FChasingTag::StaticStruct());
        }
    }

    // Debug: Log cada 10 segundos el número de tags actualizadas
    static int32 TagUpdateCounter = 0;
    if (++TagUpdateCounter % 600 == 0 && TagChanges.Num() > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("🏷️ Tags actualizadas: %d entidades"), TagChanges.Num());
    }
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
            // DEBUG TEMPORAL: Log por qué cambia a Chase
            static int32 ChaseChangeCounter = 0;
            if (++ChaseChangeCounter % 100 == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("🧠 CAMBIANDO A CHASE: PlayerStimulus=%s AnyStimulus=%s Distance=%.1f"),
                       StimuliFragment.HasPlayerStimulus() ? TEXT("✓") : TEXT("✗"),
                       StimuliFragment.HasAnyStimulus() ? TEXT("✓") : TEXT("✗"),
                       StimuliFragment.StimulusDistance);
            }

            StateFragment.SetToChasing();
            MovementFragment.StartChasing(StimuliFragment.StimulusDirection, 150); // Velocidad alta para persecución

            // NOTA: Tags no necesarios con enfoque híbrido DOP

            // Guardar datos del estímulo en StateData
            StateFragment.SetStateData(static_cast<uint32>(StimuliFragment.StimulusDistance));
        }
        return;
    }

    // Sin logs en hot-path

    // Verificar si está persiguiendo y debe salir del estado
    if (bIsChasing)
    {
        // NUEVO: Verificar condiciones para salir del Chase
        float ChaseTimer = StateFragment.GetStateTimerSeconds();

        // Salir del Chase si:
        // 1. No hay estímulo del jugador
        // 2. El estímulo expiró
        // 3. Ha estado persiguiendo demasiado tiempo (5 segundos)
        if (!StimuliFragment.HasPlayerStimulus() ||
            StimuliFragment.IsStimulusExpired() ||
            ChaseTimer > 5.0f)
        {
            // Decidir entre Idle o WalkAround aleatoriamente
            float RandomChoice = FMath::RandRange(0.0f, 1.0f);
            if (RandomChoice < 0.6f) // 60% chance de WalkAround
            {
                StateFragment.SetToWalking();
                MovementFragment.StartMoving(MovementFragment.GetDirection(), 50);
            }
            else // 40% chance de Idle
            {
                StateFragment.SetToIdle();
                MovementFragment.Stop();
            }
        }
        return;
    }

    // Transiciones determinísticas entre Idle y WalkAround (compatibles con shared config)
    if (bIsIdle)
    {
        // NOTA: Esta lógica está duplicada intencionalmente con la del main loop
        // para garantizar transiciones en cualquier código path
        float IdleTimer = StateFragment.GetStateTimerSeconds();
        if (IdleTimer > 2.0f) // 2 segundos fijos para ser determinístico
        {
            StateFragment.SetToWalking();
            // Generar dirección aleatoria
            FVector RandomDirection = FVector(
                                          FMath::RandRange(-1.0f, 1.0f),
                                          FMath::RandRange(-1.0f, 1.0f),
                                          0.0f)
                                          .GetSafeNormal();
            MovementFragment.StartMoving(RandomDirection, 50);
            UE_LOG(LogTemp, Log, TEXT("🧠 Fallback: Idle → WalkAround (timer=%.2f)"), IdleTimer);
        }
    }
    else if (bIsWalking)
    {
        float WalkTimer = StateFragment.GetStateTimerSeconds();
        if (WalkTimer > 6.0f) // 6 segundos fijos para ser determinístico
        {
            StateFragment.SetToIdle();
            MovementFragment.Stop();
            UE_LOG(LogTemp, Log, TEXT("🧠 Fallback: WalkAround → Idle (timer=%.2f)"), WalkTimer);
        }
    }
    else
    {
        // Solo forzar Idle si no tiene ningún estado válido, pero SIN resetear timer
        if (!StateFragment.IsIdle() && !StateFragment.IsWalking() && !StateFragment.IsChasing())
        {
            StateFragment.SetToIdle();
            MovementFragment.Stop();
            UE_LOG(LogTemp, Log, TEXT("🧠 EvaluateStateTransitions: Forzando estado Idle por defecto"));
        }
        // Si ya está en Idle, NO tocarlo para que el timer pueda aumentar
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

    // Sin logs en hot-path
}

void UZombiBehaviorProcessor::UpdateWalkAroundState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime)
{
    // Lógica de caminar aleatoriamente
    float Timer = StateFragment.GetStateTimerSeconds();

    // Cambiar dirección cada 2-4 segundos para más variedad
    float DirectionChangeInterval = FMath::RandRange(2.0f, 4.0f);
    if (Timer > DirectionChangeInterval)
    {
        // Resetear timer para el próximo cambio de dirección
        StateFragment.SetStateTimerSeconds(0.0f);

        // Generar nueva dirección aleatoria
        FVector RandomDirection = FVector(
                                      FMath::RandRange(-1.0f, 1.0f),
                                      FMath::RandRange(-1.0f, 1.0f),
                                      0.0f)
                                      .GetSafeNormal();

        MovementFragment.SetDirection(RandomDirection);
        MovementFragment.SetSpeed(static_cast<uint8>(FMath::RandRange(30, 70))); // Velocidad variable
    }
}

void UZombiBehaviorProcessor::UpdateIdleState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, float DeltaTime)
{
    // En estado Idle, el zombie está inmóvil
    // Las transiciones se manejan en EvaluateStateTransitions()

    // Asegurarse de que está realmente parado
    MovementFragment.Stop();

    // Comportamiento ocasional: girar la cabeza/cuerpo ligeramente
    float Timer = StateFragment.GetStateTimerSeconds();
    if (Timer > FMath::RandRange(1.0f, 3.0f))
    {
        // Pequeño ajuste de rotación para simular "mirar alrededor"
        // (Este procesamiento se maneja en TransformProcessor)
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