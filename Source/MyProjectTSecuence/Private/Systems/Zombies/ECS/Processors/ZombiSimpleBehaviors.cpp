#include "Systems/Zombies/ECS/Processors/ZombiSimpleBehaviors.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUpdateFrequencyFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

// Función helper para obtener buckets por LOD (estática)
FORCEINLINE uint8 GetBucketsForLOD(EZombiUpdatePriority Priority)
{
    switch (Priority)
    {
    case EZombiUpdatePriority::Critical:
        return 1; // 60 FPS
    case EZombiUpdatePriority::High:
        return 2; // 30 FPS
    case EZombiUpdatePriority::Normal:
        return 4; // 15 FPS
    case EZombiUpdatePriority::Low:
        return 12; // 5 FPS
    default:
        return 4;
    }
}

UZombiSimpleBehaviors::UZombiSimpleBehaviors()
{
    // Configuración del processor unificado
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("ZombiSimpleBehaviors");
    bAutoRegisterWithProcessingPhases = true;
}

void UZombiSimpleBehaviors::ConfigureQueries()
{
    // Query ultra-simple para debugging - solo los fragmentos más básicos
    SimpleBehaviorsQuery.RegisterWithProcessor(*this);
    SimpleBehaviorsQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    SimpleBehaviorsQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    SimpleBehaviorsQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);

    // Sin tags ni fragmentos opcionales por ahora - debugging puro
    UE_LOG(LogTemp, Warning, TEXT("🎮 ZombiSimpleBehaviors: Query configurada con solo 3 fragmentos básicos"));
}

void UZombiSimpleBehaviors::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetWorld()->HasBegunPlay())
    {
        return;
    }

    // Obtener delta time y tiempo actual
    const float DeltaTime = Context.GetDeltaTimeSeconds();
    const float CurrentTime = GetWorld()->GetTimeSeconds();

    // Debug: Verificar si el procesador se está ejecutando
    static int32 ExecuteCounter = 0;
    if (++ExecuteCounter % 300 == 0) // Cada 5 segundos aprox
    {
        UE_LOG(LogTemp, Warning, TEXT("🧠 ZombiSimpleBehaviors: EJECUTÁNDOSE | Execute count: %d"), ExecuteCounter);
    }

    // Actualizar cache de estímulos
    UpdateStimulusCache(DeltaTime);

    // Obtener posición del jugador para LOD
    FVector PlayerLocation = FVector::ZeroVector;
    if (this->bHasCachedPlayerStimulus)
    {
        PlayerLocation = this->CachedLatestPlayerStimulus.Position;
    }
    else
    {
        // Fallback: obtener directamente del jugador
        if (APawn *PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0))
        {
            PlayerLocation = PlayerPawn->GetActorLocation();
        }
    }

    // Procesar todas las entidades en un solo loop unificado
    SimpleBehaviorsQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime, CurrentTime, PlayerLocation](FMassExecutionContext &ChunkContext)
                                            {
        // Debug del chunk
        static int32 ChunkDebugCounter = 0;
        if (++ChunkDebugCounter % 300 == 0) // Cada 5 segundos aprox
        {
            UE_LOG(LogTemp, Warning, TEXT("🧠 ZombiSimpleBehaviors Chunk: %d entidades procesadas"), ChunkContext.GetNumEntities());
        }

        // Obtener arrays de fragmentos - solo los 3 básicos para debugging
        TArrayView<FZombiTransformFragment> TransformFragments = ChunkContext.GetMutableFragmentView<FZombiTransformFragment>();
        TArrayView<FZombiStateFragment> StateFragments = ChunkContext.GetMutableFragmentView<FZombiStateFragment>();
        TArrayView<FZombiMovementFragment> MovementFragments = ChunkContext.GetMutableFragmentView<FZombiMovementFragment>();
        
        // Procesar cada zombie en el chunk - MODO SIMPLE PARA DEBUGGING
        for (int32 EntityIndex = 0; EntityIndex < ChunkContext.GetNumEntities(); ++EntityIndex)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[EntityIndex];
            FZombiStateFragment& StateFragment = StateFragments[EntityIndex];
            FZombiMovementFragment& MovementFragment = MovementFragments[EntityIndex];
            
            // PIPELINE ULTRA-SIMPLE: Solo comportamiento básico
            
            // 1. ACTUALIZAR TIMER DEL ESTADO (AHORA QUE FUNCIONA)
            float CurrentTimer = StateFragment.GetStateTimerSeconds();
            float NewTimer = CurrentTimer + DeltaTime;
            StateFragment.SetStateTimerSeconds(NewTimer);
            
            // DEBUG SIMPLIFICADO: Solo verificar entity[0]
            if (EntityIndex == 0)
            {
                static int32 TimerDebugCounter = 0;
                if (++TimerDebugCounter % 300 == 0) // Cada 5 segundos
                {
                    UE_LOG(LogTemp, Warning, TEXT("✅ TIMER ARREGLADO: Entity[0] = %.3f segundos"),
                           StateFragment.GetStateTimerSeconds());
                }
            }
            
            // 2. PROCESAR COMPORTAMIENTO (IA) - sin estímulos por ahora
            FZombiStimuliFragment DummyStimuliFragment; // Fragmento vacío para compatibilidad
            
            // Debug: Solo cada 5 segundos
            if (EntityIndex == 0 && StateFragment.IsIdle())
            {
                static int32 TransitionDebugCounter = 0;
                if (++TransitionDebugCounter % 300 == 0) // Cada 5 segundos
                {
                    bool bShouldTransition = this->ShouldTransitionFromIdle(StateFragment);
                    UE_LOG(LogTemp, Warning, TEXT("🧠 PROBLEMA: Timer FIJO en %.3f | Required=%.1f | ShouldTransition=%s"),
                           CurrentTimer, IDLE_TO_WALK_TIME, bShouldTransition ? TEXT("SÍ") : TEXT("NO"));
                }
            }
            
            // HABILITADO: Procesar comportamiento ahora que el timer funciona
            this->ProcessBehaviorForZombie(StateFragment, MovementFragment, DummyStimuliFragment, DeltaTime);
            
            // 3. PROCESAR MOVIMIENTO (FÍSICA)
            this->ProcessMovementForZombie(TransformFragment, MovementFragment, StateFragment, DeltaTime);
            
            // 4. SINCRONIZAR VISUAL (TurboSequence)
            this->SyncVisualState(StateFragment, MovementFragment, EntityIndex);
            
            // Debug individual reducido
            static int32 EntityDebugCounter = 0;
            if (++EntityDebugCounter % 300 == 0 && EntityIndex == 0) // Cada 5 segundos
            {
                UE_LOG(LogTemp, Warning, TEXT("🧠 Entity[0]: %s | Timer=%.3f | DeltaTime=%.3f"),
                       StateFragment.IsIdle() ? TEXT("Idle") : 
                       StateFragment.IsWalking() ? TEXT("Walk") : 
                       StateFragment.IsChasing() ? TEXT("Chase") : TEXT("Unknown"),
                       CurrentTimer,
                       DeltaTime);
            }
        } });
}

void UZombiSimpleBehaviors::UpdateStimulusCache(float DeltaTime)
{
    // Actualizar cache periódicamente
    CacheUpdateTimer += DeltaTime;
    if (CacheUpdateTimer >= StimulusUpdateInterval)
    {
        // Obtener estímulos activos del StimulusSubsystem
        if (StimulusSubsystem && StimulusSubsystem->IsValidLowLevel())
        {
            CachedActiveStimuli = StimulusSubsystem->GetEnvironmentStimuli();

            // Vía rápida: cachear último estímulo del jugador
            FStimulusData LatestPlayerStimulus;
            bHasCachedPlayerStimulus = StimulusSubsystem->TryGetLatestPlayerStimulus(LatestPlayerStimulus);
            if (bHasCachedPlayerStimulus)
            {
                CachedLatestPlayerStimulus = LatestPlayerStimulus;
            }
        }
        else
        {
            // Intentar obtener referencia al StimulusSubsystem
            if (UWorld *World = GetWorld())
            {
                StimulusSubsystem = World->GetSubsystem<UStimulusSubsystem>();
                if (StimulusSubsystem)
                {
                    CachedActiveStimuli = StimulusSubsystem->GetEnvironmentStimuli();

                    // Vía rápida: cachear último estímulo del jugador
                    FStimulusData LatestPlayerStimulus;
                    bHasCachedPlayerStimulus = StimulusSubsystem->TryGetLatestPlayerStimulus(LatestPlayerStimulus);
                    if (bHasCachedPlayerStimulus)
                    {
                        CachedLatestPlayerStimulus = LatestPlayerStimulus;
                    }
                }
            }
        }

        CacheUpdateTimer = 0.0f;
    }
}

void UZombiSimpleBehaviors::ProcessStimuliForZombie(const FVector &ZombiePosition, FZombiStimuliFragment &StimuliFragment)
{
    // Limpiar estímulos expirados
    if (StimuliFragment.IsStimulusExpired())
    {
        StimuliFragment.ClearStimuli();
        return;
    }

    // Vía rápida: estímulo del jugador cacheado
    if (this->bHasCachedPlayerStimulus && this->IsStimulusInRange(ZombiePosition, this->CachedLatestPlayerStimulus))
    {
        StimuliFragment.UpdateStimulus(this->CachedLatestPlayerStimulus, ZombiePosition);
    }

    // Procesar estímulos ambientales usando grid espacial optimizado
    if (this->StimulusSubsystem && this->StimulusSubsystem->IsValidLowLevel())
    {
        TArray<FStimulusData> StimuliInRange = this->StimulusSubsystem->GetStimuliInRange(ZombiePosition, this->StimulusDetectionRange);

        for (const FStimulusData &Stimulus : StimuliInRange)
        {
            if (this->IsStimulusInRange(ZombiePosition, Stimulus))
            {
                StimuliFragment.UpdateStimulus(Stimulus, ZombiePosition);
            }
        }
    }
}

void UZombiSimpleBehaviors::ProcessBehaviorForZombie(FZombiStateFragment &StateFragment,
                                                     FZombiMovementFragment &MovementFragment,
                                                     const FZombiStimuliFragment &StimuliFragment,
                                                     float DeltaTime)
{
    // PIPELINE DE COMPORTAMIENTO:

    // FASE 1: ¿Hay estímulo que override el comportamiento automático?
    if (this->ShouldOverrideWithStimulus(StimuliFragment))
    {
        // STIMULUS OVERRIDE: Player detected → Chase
        if (StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus())
        {
            if (!StateFragment.IsChasing())
            {
                StateFragment.SetToChasing();
                MovementFragment.StartChasing(StimuliFragment.StimulusDirection, CHASE_SPEED);

                static int32 ChaseCounter = 0;
                if (++ChaseCounter % 60 == 0)
                {
                    // UE_LOG(LogTemp, Warning, TEXT("🧠 STIMULUS OVERRIDE: → Chase | Distance=%.1f"),
                    //        StimuliFragment.StimulusDistance);
                }
            }
        }
    }
    else
    {
        // FASE 2: Comportamiento automático (Idle ↔ WalkAround)
        if (StateFragment.IsIdle())
        {
            // Idle → WalkAround después de un tiempo
            if (this->ShouldTransitionFromIdle(StateFragment))
            {
                StateFragment.SetToWalking();
                StateFragment.SetStateTimerSeconds(0.0f);  // Resetear timer para el nuevo estado
                StateFragment.SetActionTimerSeconds(0.0f); // Resetear timer de cambio de dirección
                FVector RandomDirection = this->GenerateRandomDirection();
                MovementFragment.StartMoving(RandomDirection, DEFAULT_WALK_SPEED);

                UE_LOG(LogTemp, Warning, TEXT("🧠 TRANSICIÓN: Idle → WalkAround | Nuevo timer iniciado"));
            }
        }
        else if (StateFragment.IsWalking())
        {
            // WalkAround → Idle después de un tiempo
            if (this->ShouldTransitionFromWalk(StateFragment))
            {
                StateFragment.SetToIdle();
                StateFragment.SetStateTimerSeconds(0.0f); // Resetear timer para el nuevo estado
                MovementFragment.Stop();

                UE_LOG(LogTemp, Warning, TEXT("🧠 TRANSICIÓN: WalkAround → Idle | Nuevo timer iniciado"));

                // UE_LOG(LogTemp, Warning, TEXT("🧠 AUTO: WalkAround → Idle | Timer=%.1f"),
                //        StateFragment.GetStateTimerSeconds());
            }
            else
            {
                // Cambiar dirección ocasionalmente (usando ActionTimer separado)
                float TimeSinceDirectionChange = StateFragment.GetActionTimerSeconds();
                if (TimeSinceDirectionChange > DIRECTION_CHANGE_TIME)
                {
                    FVector NewDirection = this->GenerateRandomDirection();
                    MovementFragment.SetDirection(NewDirection);
                    // Resetear SOLO el ActionTimer para cambios de dirección
                    StateFragment.SetActionTimerSeconds(0.0f);

                    UE_LOG(LogTemp, Warning, TEXT("🧠 WalkAround: Nueva dirección | StateTimer=%.1f ActionTimer=%.1f"),
                           StateFragment.GetStateTimerSeconds(), TimeSinceDirectionChange);
                }
                else
                {
                    // Actualizar ActionTimer para cambios de dirección
                    StateFragment.SetActionTimerSeconds(TimeSinceDirectionChange + DeltaTime);
                }
            }
        }
        else if (StateFragment.IsChasing())
        {
            // Chase → volver a comportamiento automático si se pierde el estímulo
            StateFragment.SetToIdle();
            MovementFragment.Stop();
            // UE_LOG(LogTemp, Log, TEXT("🧠 Chase perdido → Idle"));
        }
    }
}

void UZombiSimpleBehaviors::ProcessMovementForZombie(FZombiTransformFragment &TransformFragment,
                                                     FZombiMovementFragment &MovementFragment,
                                                     const FZombiStateFragment &StateFragment,
                                                     float DeltaTime)
{
    // Aplicar movimiento físico según el estado
    if (StateFragment.IsChasing() || StateFragment.IsWalking())
    {
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
        }
    }
    else if (StateFragment.IsIdle())
    {
        // Asegurar que esté detenido
        if (MovementFragment.GetSpeed() > 0)
        {
            MovementFragment.Stop();
        }
    }
}

bool UZombiSimpleBehaviors::IsStimulusInRange(const FVector &ZombiePosition, const FStimulusData &Stimulus) const
{
    // Calcular distancia al estímulo
    float Distance = FVector::Dist(ZombiePosition, Stimulus.Position);

    // Verificar si está en rango de detección y en radio del estímulo
    return Distance <= StimulusDetectionRange && Distance <= Stimulus.Radius;
}

void UZombiSimpleBehaviors::UpdateResponseTimers(FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Actualizar timer de respuesta
    if (StimuliFragment.HasAnyStimulus())
    {
        StimuliFragment.ResponseTimer += static_cast<uint16>(DeltaTime * 100.0f);
    }
}

FVector UZombiSimpleBehaviors::GenerateRandomDirection()
{
    float RandomAngle = FMath::RandRange(0.0f, 2.0f * PI);
    return FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f).GetSafeNormal();
}

bool UZombiSimpleBehaviors::ShouldTransitionFromIdle(const FZombiStateFragment &StateFragment)
{
    return StateFragment.GetStateTimerSeconds() >= IDLE_TO_WALK_TIME;
}

bool UZombiSimpleBehaviors::ShouldTransitionFromWalk(const FZombiStateFragment &StateFragment)
{
    return StateFragment.GetStateTimerSeconds() >= WALK_TO_IDLE_TIME;
}

bool UZombiSimpleBehaviors::ShouldOverrideWithStimulus(const FZombiStimuliFragment &StimuliFragment)
{
    // Por ahora solo Player stimulus, pero se puede extender
    return StimuliFragment.HasPlayerStimulus() && StimuliFragment.HasAnyStimulus();
}

void UZombiSimpleBehaviors::SyncVisualState(const FZombiStateFragment &StateFragment,
                                            const FZombiMovementFragment &MovementFragment,
                                            int32 EntityIndex)
{
    // SYNC VISUAL: Indicar a TurboSequence qué animación usar basada en el estado lógico

    // Debug solo para Entity[0] cada cierto tiempo
    if (EntityIndex == 0)
    {
        static int32 VisualDebugCounter = 0;
        if (++VisualDebugCounter % 300 == 0) // Cada 5 segundos
        {
            FString VisualState = "Unknown";
            if (StateFragment.IsIdle())
                VisualState = "Idle";
            else if (StateFragment.IsWalking())
                VisualState = "Walking";
            else if (StateFragment.IsChasing())
                VisualState = "Chasing";

            UE_LOG(LogTemp, Warning, TEXT("🎬 VISUAL SYNC: Estado=%s | Speed=%.1f"),
                   *VisualState, MovementFragment.GetEffectiveSpeed());
        }
    }

    // TODO: Implementar sincronización real con TurboSequence
    // Por ahora solo registramos el estado para debugging
}
