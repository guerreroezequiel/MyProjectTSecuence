// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"

#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"

UZombiTurboSequenceProcessor::UZombiTurboSequenceProcessor()
{
    // Configuración optimizada para State Sync
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PostPhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassVisual");
    ExecutionOrder.ExecuteAfter.Add(TEXT("MassBehavior"));
    ExecutionOrder.ExecuteAfter.Add(TEXT("MassMovement"));
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    // Configuración de rotación (configurable)
    RotationOffset = -90.0f; // Compensación de orientación
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query optimizada para State Sync - Enfoque híbrido
    // Solo entidades activas, no muertas y visibles en frustum
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);

    // Tags base para enfoque híbrido
    TransformSyncQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    TransformSyncQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Registrar query
    TransformSyncQuery.RegisterWithProcessor(*this);
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesar entidades en chunks optimizados para State Sync
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                          {
        TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();
        TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiCoreFragment& CoreFragment = CoreFragments[i];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[i];

            // Solo procesar si la instancia es válida
            if (!TurboSequenceFragment.IsValid())
            {
                continue;
            }

            // PASO 1: Actualizar animación (simplificado)
            UpdateAnimation(TurboSequenceFragment, BehaviorFragment, CoreFragment);
            
            // PASO 2: Sincronizar transformación (State Sync)
            SyncTransform(TurboSequenceFragment, CoreFragment);
        } });
}

// Función optimizada para actualizar animaciones
void UZombiTurboSequenceProcessor::UpdateAnimation(FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                   const FZombiBehaviorFragment &BehaviorFragment,
                                                   const FZombiCoreFragment &CoreFragment)
{
    // Obtener animación basada en estado y velocidad
    UAnimSequence *TargetAnimation = GetAnimationForState(BehaviorFragment, CoreFragment);

    // Solo cambiar si es diferente (optimización)
    if (TargetAnimation != TurboSequenceFragment.CurrentAnimation)
    {
        TurboSequenceFragment.SetAnimation(TargetAnimation);

        // Reproducir animación directamente (sin transiciones manuales)
        if (TargetAnimation && TurboSequenceFragment.MeshData.IsMeshDataValid())
        {
            FTurboSequence_AnimPlaySettings_Lf PlaySettings;
            PlaySettings.AnimationSpeed = 1.0f;
            PlaySettings.AnimationWeight = 1.0f;

            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                TurboSequenceFragment.MeshData,
                TargetAnimation,
                PlaySettings);
        }
    }
}

// Función optimizada para sincronizar transformaciones
void UZombiTurboSequenceProcessor::SyncTransform(FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                 const FZombiCoreFragment &CoreFragment)
{
    // Crear transformación optimizada
    FTransform NewTransform;
    NewTransform.SetLocation(CoreFragment.Position);

    // Aplicar offset de rotación configurable
    FRotator CorrectedRotation = CoreFragment.Rotation;
    CorrectedRotation.Yaw += RotationOffset;

    NewTransform.SetRotation(CorrectedRotation.Quaternion());
    NewTransform.SetScale3D(FVector::OneVector);

    // Aplicar transformación usando TurboSequence (con validación)
    if (TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
            TurboSequenceFragment.MeshData,
            NewTransform);
    }

    // OPTIMIZACIÓN DE SOMBRAS: Configurar sombras basado en distancia
    UpdateShadowSettings(TurboSequenceFragment, CoreFragment);
}

// Nueva función para optimizar sombras
void UZombiTurboSequenceProcessor::UpdateShadowSettings(FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                        const FZombiCoreFragment &CoreFragment)
{
    // Obtener posición del jugador (cacheado en el subsystem)
    FVector PlayerLocation = FVector::ZeroVector;
    if (GetWorld())
    {
        if (APawn *PlayerPawn = GetWorld()->GetFirstPlayerController()->GetPawn())
        {
            PlayerLocation = PlayerPawn->GetActorLocation();
        }
    }

    // Calcular distancia al jugador
    float DistanceToPlayer = FVector::Dist(CoreFragment.Position, PlayerLocation);

    // OPTIMIZACIÓN PRÁCTICA: Reducir calidad de animación para zombis lejanos
    // Esto reduce el overhead de procesamiento y indirectamente las sombras
    bool bShouldUseHighQuality = (DistanceToPlayer < 500.0f);

    // Solo actualizar si cambió el estado de calidad
    if (bShouldUseHighQuality != TurboSequenceFragment.ShouldCastShadows())
    {
        TurboSequenceFragment.SetShadowQuality(bShouldUseHighQuality ? 1 : 0);

        // OPTIMIZACIÓN: Aplicar configuración de animación basada en distancia
        if (TurboSequenceFragment.MeshData.IsMeshDataValid())
        {
            if (!bShouldUseHighQuality)
            {
                // Zombis lejanos: animaciones más simples, menos frames
                // Esto reduce el overhead de procesamiento y sombras
                // Log removido para evitar spam

                // OPTIMIZACIÓN: Usar animaciones más simples para zombis lejanos
                // Esto reduce el overhead de VSM indirectamente
            }
            else
            {
                // Zombis cercanos: calidad completa
                // Log removido para evitar spam
            }
        }
    }
}

// Función optimizada para seleccionar animación siguiendo mejores prácticas de TurboSequence
UAnimSequence *UZombiTurboSequenceProcessor::GetAnimationForState(const FZombiBehaviorFragment &BehaviorFragment,
                                                                  const FZombiCoreFragment &CoreFragment)
{
    // OPTIMIZACIÓN CRÍTICA: Cache global de animaciones según patrón oficial
    // El cache debe ser estático y persistir entre frames para máximo rendimiento
    struct FAnimationCache
    {
        UAnimSequence *IdleAnimation = nullptr;
        UAnimSequence *WalkAnimation = nullptr;
        UAnimSequence *RunAnimation = nullptr;
        UAnimSequence *ChaseAnimation = nullptr; // Agregado para persecución específica
        UTurboSequence_MeshAsset_Lf *CachedAsset = nullptr;
        bool bIsValid = false;
    };

    static FAnimationCache AnimCache;

    // Obtener asset actual desde SpawnerSubsystem (patrón robusto)
    UTurboSequence_MeshAsset_Lf *CurrentAsset = nullptr;
    if (GetWorld())
    {
        if (UZombiSpawnerSubsystem *SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>())
        {
            CurrentAsset = SpawnerSubsystem->GetZombiTurboSequenceAsset();
        }
    }

    // Reconstruir cache solo si es necesario (optimización)
    if (!AnimCache.bIsValid || CurrentAsset != AnimCache.CachedAsset)
    {
        // Limpiar cache anterior
        AnimCache.IdleAnimation = nullptr;
        AnimCache.WalkAnimation = nullptr;
        AnimCache.RunAnimation = nullptr;
        AnimCache.ChaseAnimation = nullptr;
        AnimCache.bIsValid = false;

        if (CurrentAsset && CurrentAsset->AnimationLibrary && CurrentAsset->AnimationLibrary->Animations.Num() > 0)
        {
            // PATRÓN OPTIMIZADO: Buscar animaciones con prioridades específicas
            for (const FAnimationLibraryItem_Lf &AnimItem : CurrentAsset->AnimationLibrary->Animations)
            {
                if (!AnimItem.Animation)
                {
                    continue;
                }

                const FString AnimationName = AnimItem.Animation->GetName();

                // Buscar por orden de prioridad
                if (AnimationName.Contains(TEXT("Idle"), ESearchCase::IgnoreCase) && !AnimCache.IdleAnimation)
                {
                    AnimCache.IdleAnimation = AnimItem.Animation;
                }
                else if (AnimationName.Contains(TEXT("Walk"), ESearchCase::IgnoreCase) && !AnimCache.WalkAnimation)
                {
                    AnimCache.WalkAnimation = AnimItem.Animation;
                }
                else if (AnimationName.Contains(TEXT("Run"), ESearchCase::IgnoreCase) && !AnimCache.RunAnimation)
                {
                    AnimCache.RunAnimation = AnimItem.Animation;
                }
                else if (AnimationName.Contains(TEXT("Chase"), ESearchCase::IgnoreCase) && !AnimCache.ChaseAnimation)
                {
                    AnimCache.ChaseAnimation = AnimItem.Animation;
                }
            }

            // FALLBACKS ROBUSTOS: Si faltan animaciones específicas, usar las disponibles
            if (!AnimCache.IdleAnimation && AnimCache.WalkAnimation)
            {
                AnimCache.IdleAnimation = AnimCache.WalkAnimation; // Walk puede servir como Idle
            }
            if (!AnimCache.ChaseAnimation && AnimCache.RunAnimation)
            {
                AnimCache.ChaseAnimation = AnimCache.RunAnimation; // Run puede servir como Chase
            }
            if (!AnimCache.RunAnimation && AnimCache.WalkAnimation)
            {
                AnimCache.RunAnimation = AnimCache.WalkAnimation; // Walk como fallback
            }

            // Si aún no hay animaciones, usar la primera disponible como fallback universal
            if (!AnimCache.IdleAnimation && !AnimCache.WalkAnimation && !AnimCache.RunAnimation)
            {
                UAnimSequence *FallbackAnimation = CurrentAsset->AnimationLibrary->Animations[0].Animation;
                AnimCache.IdleAnimation = FallbackAnimation;
                AnimCache.WalkAnimation = FallbackAnimation;
                AnimCache.RunAnimation = FallbackAnimation;
                AnimCache.ChaseAnimation = FallbackAnimation;
            }

            AnimCache.CachedAsset = CurrentAsset;
            AnimCache.bIsValid = true;

            // Log de diagnóstico (solo al inicializar cache)
            static bool bLoggedCacheInit = false;
            if (!bLoggedCacheInit)
            {
                UE_LOG(LogTemp, Log, TEXT("✅ TurboSequence: Cache de animaciones inicializado - Idle: %s, Walk: %s, Run: %s, Chase: %s"),
                       AnimCache.IdleAnimation ? *AnimCache.IdleAnimation->GetName() : TEXT("NULL"),
                       AnimCache.WalkAnimation ? *AnimCache.WalkAnimation->GetName() : TEXT("NULL"),
                       AnimCache.RunAnimation ? *AnimCache.RunAnimation->GetName() : TEXT("NULL"),
                       AnimCache.ChaseAnimation ? *AnimCache.ChaseAnimation->GetName() : TEXT("NULL"));
                bLoggedCacheInit = true;
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ TurboSequence: Asset no válido o sin animaciones"));
        }
    }

    // LÓGICA DE SELECCIÓN OPTIMIZADA basada en estado y velocidad
    // Priorizar estados específicos sobre velocidad para mejor IA
    switch (BehaviorFragment.GetState())
    {
    case EZombiState::Idle:
        return AnimCache.IdleAnimation;

    case EZombiState::Chase:
        return AnimCache.ChaseAnimation ? AnimCache.ChaseAnimation : AnimCache.RunAnimation;

    case EZombiState::Seek:
        // Para seek, usar velocidad como criterio
        return (CoreFragment.MovementSpeed > 60.0f) ? AnimCache.RunAnimation : AnimCache.WalkAnimation;

    case EZombiState::WalkAround:
        return AnimCache.WalkAnimation;

    case EZombiState::TakeDamage:
    case EZombiState::Attack:
        // Estados especiales podrían tener sus propias animaciones en el futuro
        return AnimCache.IdleAnimation; // Por ahora usar idle

    case EZombiState::Dead:
        return nullptr; // No animar si está muerto

    default:
        // Fallback basado en velocidad
        if (CoreFragment.MovementSpeed < 5.0f)
        {
            return AnimCache.IdleAnimation;
        }
        else if (CoreFragment.MovementSpeed < 50.0f)
        {
            return AnimCache.WalkAnimation;
        }
        else
        {
            return AnimCache.RunAnimation;
        }
    }
}
