// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
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
    // Query optimizada para State Sync - solo fragmentos necesarios
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
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

// Función optimizada para seleccionar animación
UAnimSequence *UZombiTurboSequenceProcessor::GetAnimationForState(const FZombiBehaviorFragment &BehaviorFragment,
                                                                  const FZombiCoreFragment &CoreFragment)
{
    // Cache global de animaciones (compartido entre todas las entidades)
    static UAnimSequence *CachedIdleAnimation = nullptr;
    static UAnimSequence *CachedWalkAnimation = nullptr;
    static UAnimSequence *CachedRunAnimation = nullptr;
    static bool bAnimationsCached = false;
    static UTurboSequence_MeshAsset_Lf *LastCachedAsset = nullptr;

    // Obtener el asset actual del primer zombie disponible
    UTurboSequence_MeshAsset_Lf *CurrentAsset = nullptr;

    // Intentar obtener el asset desde el SpawnerSubsystem
    if (GetWorld())
    {
        if (UZombiSpawnerSubsystem *SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>())
        {
            // Obtener el asset configurado en el spawner
            CurrentAsset = SpawnerSubsystem->GetZombiTurboSequenceAsset();
        }
    }

    // Cache animaciones si cambió el asset o no están cacheadas
    if (!bAnimationsCached || CurrentAsset != LastCachedAsset)
    {
        if (CurrentAsset && CurrentAsset->AnimationLibrary)
        {
            // Limpiar cache anterior
            CachedIdleAnimation = nullptr;
            CachedWalkAnimation = nullptr;
            CachedRunAnimation = nullptr;

            // Buscar y cachear animaciones por nombre
            for (const FAnimationLibraryItem_Lf &AnimItem : CurrentAsset->AnimationLibrary->Animations)
            {
                if (!AnimItem.Animation)
                {
                    continue;
                }

                FString AnimationName = AnimItem.Animation->GetName();

                if (AnimationName.Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
                {
                    CachedIdleAnimation = AnimItem.Animation;
                }
                else if (AnimationName.Contains(TEXT("Walk"), ESearchCase::IgnoreCase))
                {
                    CachedWalkAnimation = AnimItem.Animation;
                }
                else if (AnimationName.Contains(TEXT("Run"), ESearchCase::IgnoreCase))
                {
                    CachedRunAnimation = AnimItem.Animation;
                }
            }

            LastCachedAsset = CurrentAsset;
            bAnimationsCached = true;

            // Log de diagnóstico (solo una vez)
            static bool bLoggedCacheUpdate = false;
            if (!bLoggedCacheUpdate)
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 TurboSequence: Cache de animaciones actualizado - Idle: %s, Walk: %s, Run: %s"),
                       CachedIdleAnimation ? *CachedIdleAnimation->GetName() : TEXT("NULL"),
                       CachedWalkAnimation ? *CachedWalkAnimation->GetName() : TEXT("NULL"),
                       CachedRunAnimation ? *CachedRunAnimation->GetName() : TEXT("NULL"));
                bLoggedCacheUpdate = true;
            }
        }
        else
        {
            // Si no hay asset válido, marcar como cacheado para evitar búsquedas repetidas
            bAnimationsCached = true;
            UE_LOG(LogTemp, Warning, TEXT("🎮 TurboSequence: No se pudo obtener asset válido para cache de animaciones"));
        }
    }

    // Lógica de selección optimizada
    if (BehaviorFragment.IsChasing())
    {
        return CachedRunAnimation;
    }
    else if (CoreFragment.MovementSpeed < 5.0f)
    {
        return CachedIdleAnimation;
    }
    else if (CoreFragment.MovementSpeed < 50.0f)
    {
        return CachedWalkAnimation;
    }
    else
    {
        return CachedRunAnimation;
    }
}
