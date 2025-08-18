// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"

#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Animation/BlendSpace.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Math/Vector.h"
#include "TurboSequence_Manager_Lf.h"

UZombiTurboSequenceProcessor::UZombiTurboSequenceProcessor()
{
    // Rehabilitar procesador para sincronización de transformaciones
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PostPhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true; // Rehabilitar registro automático

    // Log solo en la primera instancia
    static bool bLoggedConstructor = false;
    if (!bLoggedConstructor)
    {

        bLoggedConstructor = true;
    }
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query para sincronizar transformaciones y animaciones (State Sync) - optimizado
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddSharedRequirement<FZombiVisualSharedFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Log eliminado para optimización de rendimiento
    // static float DebugTimer = 0.0f;
    // DebugTimer += DeltaTime;
    // if (DebugTimer >= 5.0f) // Log cada 5 segundos
    // {
    //     UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Ejecutándose - DeltaTime: %f"), DeltaTime);
    //     DebugTimer = 0.0f;
    // }

    // Sincronizar transformaciones y animaciones (State Sync) - optimizado
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                          {
        TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        TArrayView<const FZombiTransformFragment> TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
        TArrayView<const FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
        const FZombiVisualSharedFragment& VisualShared = Context.GetSharedFragment<FZombiVisualSharedFragment>();


        // Log eliminado para optimización de rendimiento
        // static float EntityDebugTimer = 0.0f;
        // EntityDebugTimer += DeltaTime;
        // if (EntityDebugTimer >= 5.0f && Context.GetNumEntities() > 0)
        // {
        //     UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Procesando %d entidades"), Context.GetNumEntities());
        //     EntityDebugTimer = 0.0f;
        // }

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiTransformFragment& TransformFragment = TransformFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];


            // Actualizar animación basada en estado PRIMERO
            UpdateAnimationBasedOnState(Context, i, TurboSequenceFragment, StateFragment, TransformFragment);
            
            // SINCRONIZAR TRANSFORMACIÓN con TurboSequence (con dirty flags y umbrales)
            if (TurboSequenceFragment.TurboSequenceAsset && TurboSequenceFragment.MeshData.IsMeshDataValid())
            {
                const FVector CurrentPos = TransformFragment.GetPosition();
                const float CurrentYaw = TransformFragment.GetYaw();

                // Usar thresholds compartidos si existen; fallback a per-entidad
                const float PosThresh = VisualShared.SharedMinPositionDeltaForSync > 0.f ? VisualShared.SharedMinPositionDeltaForSync : TurboSequenceFragment.MinPositionDeltaForSync;
                const float YawThresh = VisualShared.SharedMinYawDeltaForSync > 0.f ? VisualShared.SharedMinYawDeltaForSync : TurboSequenceFragment.MinYawDeltaForSync;
                const bool bPosDirty = FVector::DistSquared(CurrentPos, TurboSequenceFragment.LastSyncedPosition) >= FMath::Square(PosThresh);
                const bool bYawDirty = FMath::Abs(CurrentYaw - TurboSequenceFragment.LastSyncedYaw) >= YawThresh;
                TurboSequenceFragment.bTransformDirty = TurboSequenceFragment.bTransformDirty || bPosDirty || bYawDirty;

                if (TurboSequenceFragment.bTransformDirty)
                {
                    FTransform NewTransform;
                    NewTransform.SetLocation(CurrentPos);

                    // CORREGIR ROTACIÓN: Compensar la diferencia de 90 grados entre animación y transformación
                    FRotator CorrectedRotation = TransformFragment.GetRotation();
                    CorrectedRotation.Yaw -= 90.0f;

                    NewTransform.SetRotation(CorrectedRotation.Quaternion());
                    NewTransform.SetScale3D(FVector::OneVector);

                    ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                        TurboSequenceFragment.MeshData,
                        NewTransform);

                    TurboSequenceFragment.LastSyncedPosition = CurrentPos;
                    TurboSequenceFragment.LastSyncedYaw = CurrentYaw;
                    TurboSequenceFragment.bTransformDirty = false;
                }
            }
        } });
}

// Implementación de animaciones basadas en estado con Blend Space
void UZombiTurboSequenceProcessor::UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
                                                               FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                               const FZombiStateFragment &StateFragment,
                                                               const FZombiTransformFragment &TransformFragment)
{
    // Verificar que tenemos todo lo necesario
    if (!TurboSequenceFragment.TurboSequenceAsset || !TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Log eliminado para optimización de rendimiento
    // static float AnimationDebugTimer = 0.0f;
    // AnimationDebugTimer += Context.GetDeltaTimeSeconds();
    // if (AnimationDebugTimer >= 10.0f) // Log cada 10 segundos
    // {
    //     UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Actualizando animación - Estado: %d, Velocidad: %.2f"),
    //            (int32)StateFragment.State, MovementFragment.MovementSpeed);
    //     AnimationDebugTimer = 0.0f;
    // }

    // Obtener la librería de animaciones del asset
    if (!TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
    {
        return;
    }

    // USAR BLEND SPACE DIRECTAMENTE - Enfoque correcto según documentación
    float CurrentSpeed = 0.0f; // Obtener velocidad del MovementFragment si es necesario

    // Ajustar velocidad basada en estado de persecución usando flags
    if (StateFragment.IsChasing())
    {
        // Durante persecución, usar velocidad de persecución
        CurrentSpeed = FMath::Max(CurrentSpeed, 80.0f); // Mínimo 80 para persecución
    }

    // Normalizar velocidad al rango del Blend Space (0-100)
    float NormalizedSpeed = FMath::Clamp(CurrentSpeed, 0.0f, 100.0f);

    // Configuración para Blend Space
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    // Log de debugging para Blend Space
    // if (AnimationDebugTimer >= 10.0f)
    // {
    //     UE_LOG(LogTemp, Log, TEXT("🎮 Blend Space: Velocidad: %.2f, Normalizada: %.2f"), CurrentSpeed, NormalizedSpeed);
    // }

    // Actualizar timer de animación
    TurboSequenceFragment.AnimationUpdateTimer += Context.GetDeltaTimeSeconds();

    // OPTIMIZACIÓN: Usa caché por-asset si está disponible
    FCachedAnims *Cached = AnimCacheByAsset.Find(TurboSequenceFragment.TurboSequenceAsset);
    if (!Cached || !Cached->bReady)
    {
        // Rellenar caché por-asset una vez
        FCachedAnims NewCached;
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (!AnimItem.Animation)
                continue;
            const FString Name = AnimItem.Animation->GetName();
            if (!NewCached.Idle && Name.Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
                NewCached.Idle = AnimItem.Animation;
            else if (!NewCached.Walk && Name.Contains(TEXT("Walk"), ESearchCase::IgnoreCase))
                NewCached.Walk = AnimItem.Animation;
            else if (!NewCached.Run && Name.Contains(TEXT("Run"), ESearchCase::IgnoreCase))
                NewCached.Run = AnimItem.Animation;
        }
        NewCached.bReady = true;
        AnimCacheByAsset.Add(TurboSequenceFragment.TurboSequenceAsset, NewCached);
        Cached = AnimCacheByAsset.Find(TurboSequenceFragment.TurboSequenceAsset);
    }

    // OPTIMIZACIÓN: Selección directa de animación sin cálculos innecesarios
    UAnimSequence *TargetAnimation = nullptr;

    // Priorizar estado de persecución sobre velocidad
    if (StateFragment.IsChasing())
    {
        // Durante persecución, usar animación de correr
        TargetAnimation = Cached && Cached->Run ? Cached->Run : static_cast<UAnimSequence *>(TurboSequenceFragment.CachedRunAnimation);
    }
    else if (NormalizedSpeed < 5.0f)
    {
        TargetAnimation = Cached && Cached->Idle ? Cached->Idle : static_cast<UAnimSequence *>(TurboSequenceFragment.CachedIdleAnimation);
    }
    else if (NormalizedSpeed < 50.0f)
    {
        TargetAnimation = Cached && Cached->Walk ? Cached->Walk : static_cast<UAnimSequence *>(TurboSequenceFragment.CachedWalkAnimation);
    }
    else
    {
        TargetAnimation = Cached && Cached->Run ? Cached->Run : static_cast<UAnimSequence *>(TurboSequenceFragment.CachedRunAnimation);
    }

    // Log de debugging para selección de animación (reducido para mejor rendimiento)
    // static float AnimationSelectionDebugTimer = 0.0f;
    // AnimationSelectionDebugTimer += Context.GetDeltaTimeSeconds();
    // if (AnimationSelectionDebugTimer >= 15.0f) // Log cada 15 segundos en lugar de 5
    // {
    //     UE_LOG(LogTemp, Log, TEXT("🎮 Selección de Animación: Velocidad: %.2f, Target: %s"),
    //            NormalizedSpeed,
    //            TargetAnimation ? *TargetAnimation->GetName() : TEXT("NULL"));
    //     AnimationSelectionDebugTimer = 0.0f;
    // }

    // Verificar si necesitamos cambiar de animación
    bool bShouldChangeAnimation = (TargetAnimation != TurboSequenceFragment.CurrentAnimation) &&
                                  (TargetAnimation != nullptr);

    // OPTIMIZACIÓN: Solo procesar si hay cambios reales
    if (!TurboSequenceFragment.bAnimationInitialized || bShouldChangeAnimation)
    {
        if (TurboSequenceFragment.TurboSequenceAsset && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
        {
            // Cache de animaciones si no está hecho
            if (!TurboSequenceFragment.bAnimationsCached)
            {
                CacheAnimations(TurboSequenceFragment);
            }

            // Si no encontramos animación específica, usar la primera disponible
            if (!TargetAnimation && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() > 0)
            {
                TargetAnimation = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation;
            }

            // Manejar transición de animación
            if (TargetAnimation && TargetAnimation != TurboSequenceFragment.CurrentAnimation)
            {
                // Iniciar transición
                if (!TurboSequenceFragment.bIsTransitioning)
                {
                    TurboSequenceFragment.TargetAnimation = TargetAnimation;
                    TurboSequenceFragment.TransitionProgress = 0.0f;
                    TurboSequenceFragment.bIsTransitioning = true;

                    // Log eliminado para optimización de rendimiento
                }
            }

            // Actualizar transición en progreso
            if (TurboSequenceFragment.bIsTransitioning)
            {
                TurboSequenceFragment.TransitionProgress += Context.GetDeltaTimeSeconds() / TurboSequenceFragment.TransitionDuration;

                if (TurboSequenceFragment.TransitionProgress >= 1.0f)
                {
                    // Transición completada
                    TurboSequenceFragment.CurrentAnimation = TurboSequenceFragment.TargetAnimation;
                    TurboSequenceFragment.bIsTransitioning = false;
                    TurboSequenceFragment.TransitionProgress = 0.0f;

                    // Reproducir la nueva animación
                    ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                        TurboSequenceFragment.MeshData,
                        TurboSequenceFragment.CurrentAnimation,
                        PlaySettings);

                    // Log eliminado para optimización de rendimiento
                }
            }
            else if (TurboSequenceFragment.CurrentAnimation)
            {
                // No hay transición, solo reproducir la animación actual
                ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                    TurboSequenceFragment.MeshData,
                    TurboSequenceFragment.CurrentAnimation,
                    PlaySettings);
            }

            // Actualizar estado
            TurboSequenceFragment.LastSpeed = NormalizedSpeed;
            TurboSequenceFragment.AnimationUpdateTimer = 0.0f;
            TurboSequenceFragment.bAnimationInitialized = true;
            TurboSequenceFragment.LastAnimationUpdateTime = Context.GetDeltaTimeSeconds();
        }
    }
}

// Implementación de cache de animaciones para optimizar búsquedas
void UZombiTurboSequenceProcessor::CacheAnimations(FZombiTurboSequenceFragment &TurboSequenceFragment)
{
    if (!TurboSequenceFragment.TurboSequenceAsset || !TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
    {
        return;
    }

    // Buscar y cachear animaciones por nombre
    for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
    {
        if (!AnimItem.Animation)
        {
            continue;
        }

        FString AnimationName = AnimItem.Animation->GetName();

        if (AnimationName.Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
        {
            TurboSequenceFragment.CachedIdleAnimation = AnimItem.Animation;
        }
        else if (AnimationName.Contains(TEXT("Walk"), ESearchCase::IgnoreCase))
        {
            TurboSequenceFragment.CachedWalkAnimation = AnimItem.Animation;
        }
        else if (AnimationName.Contains(TEXT("Run"), ESearchCase::IgnoreCase))
        {
            TurboSequenceFragment.CachedRunAnimation = AnimItem.Animation;
        }
    }

    // Marcar como cacheado
    TurboSequenceFragment.bAnimationsCached = true;

    // Log eliminado para optimización de rendimiento
}
