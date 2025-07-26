// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiStateFragment.h"
#include "ZombiChaseFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "MyTurboSequenceAnimComponent.h"
#include "Animation/BlendSpace.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"
#include "Math/Vector.h"

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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Procesador inicializado"));
        bLoggedConstructor = true;
    }
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query para sincronizar transformaciones y animaciones (State Sync)
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiVelocityFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiChaseFragment>(EMassFragmentAccess::ReadOnly);

    // TODO: Query para Blend Space (futuro)
    // BlendSpaceQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    // BlendSpaceQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    // BlendSpaceQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);

    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: ConfigureQueries completado - animaciones habilitadas"));
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

    // Sincronizar transformaciones y animaciones (State Sync)
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                          {
        TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        TArrayView<const FZombiTransformFragment> TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
        TArrayView<const FZombiVelocityFragment> VelocityFragments = Context.GetFragmentView<FZombiVelocityFragment>();
        TArrayView<const FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
        TArrayView<const FZombiChaseFragment> ChaseFragments = Context.GetFragmentView<FZombiChaseFragment>();

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
            const FZombiVelocityFragment& VelocityFragment = VelocityFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];
            const FZombiChaseFragment& ChaseFragment = ChaseFragments[i];

            // Actualizar animación basada en estado PRIMERO
            UpdateAnimationBasedOnState(Context, i, TurboSequenceFragment, StateFragment, VelocityFragment, ChaseFragment);
            
            // Sincronizar transformación usando la rotación ya calculada en MovementProcessor DESPUÉS
            // Aplicar offset de -90° para corregir la orientación del asset de TurboSequence
            FRotator AdjustedRotation = TransformFragment.Rotation;
            AdjustedRotation.Yaw -= 90.0f; // Offset de -90° para corregir la orientación del asset
            FQuat RotationQuat = FQuat(AdjustedRotation);
            
            // Log eliminado para optimización de rendimiento
            // static float RotationDebugTimer = 0.0f;
            // RotationDebugTimer += Context.GetDeltaTimeSeconds();
            // if (RotationDebugTimer >= 15.0f && VelocityFragment.MovementSpeed > 0.0f)
            // {
            //     UE_LOG(LogTemp, Log, TEXT("🎮 TurboSequence Rotación: Velocidad: %.2f, Dirección: %s, Rotación Original: %s, Rotación Ajustada: %s, Quat: %s, ForwardVector: %s"),
            //            VelocityFragment.MovementSpeed, 
            //            *VelocityFragment.MovementDirection.ToString(),
            //            *TransformFragment.Rotation.ToString(),
            //            *AdjustedRotation.ToString(),
            //            *RotationQuat.ToString(),
            //            *AdjustedRotation.Vector().ToString());
            //     RotationDebugTimer = 0.0f;
            // }
            
            // Crear la transformación final
            FTransform FinalTransform = FTransform(RotationQuat, TransformFragment.Position, FVector::OneVector);
            
            // Aplicar la transformación a TurboSequence DESPUÉS de la animación
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                TurboSequenceFragment.MeshData,
                FinalTransform
            );
            
            // Logs eliminados para optimización de rendimiento
            // static float TransformAppliedDebugTimer = 0.0f;
            // TransformAppliedDebugTimer += Context.GetDeltaTimeSeconds();
            // if (TransformAppliedDebugTimer >= 25.0f && MovementFragment.MovementSpeed > 0.0f)
            // {
            //     UE_LOG(LogTemp, Log, TEXT("🎮 Transformación Aplicada: MeshData Válido: %s, Posición: %s, Rotación: %s"),
            //            TurboSequenceFragment.MeshData.IsMeshDataValid() ? TEXT("SÍ") : TEXT("NO"),
            //            *MovementFragment.Position.ToString(),
            //            *MovementFragment.Rotation.ToString());
            //     TransformAppliedDebugTimer = 0.0f;
            // }
            
            // static float TransformDebugTimer = 0.0f;
            // TransformDebugTimer += Context.GetDeltaTimeSeconds();
            // if (TransformDebugTimer >= 20.0f && MovementFragment.MovementSpeed > 0.0f)
            // {
            //     FVector ForwardVector = MovementFragment.Rotation.Vector();
            //     FVector MovementDirection = MovementFragment.MovementDirection;
            //     float DotProduct = FVector::DotProduct(ForwardVector, MovementDirection);
            //     
            //     UE_LOG(LogTemp, Log, TEXT("🎮 Transformación Final: Posición: %s, Rotación: %s, ForwardVector: %s, MovementDirection: %s, DotProduct: %.3f"),
            //            *MovementFragment.Position.ToString(),
            //            *MovementFragment.Rotation.ToString(),
            //            *ForwardVector.ToString(),
            //            *MovementDirection.ToString(),
            //            DotProduct);
            //     TransformDebugTimer = 0.0f;
            // }

            // static float ExecutionOrderDebugTimer = 0.0f;
            // ExecutionOrderDebugTimer += Context.GetDeltaTimeSeconds();
            // if (ExecutionOrderDebugTimer >= 30.0f && MovementFragment.MovementSpeed > 0.0f)
            // {
            //     UE_LOG(LogTemp, Log, TEXT("🎮 Orden de Ejecución: 1. Animación actualizada, 2. Transformación aplicada, Velocidad: %.2f, Rotación: %s"),
            //            MovementFragment.MovementSpeed, *MovementFragment.Rotation.ToString());
            //     ExecutionOrderDebugTimer = 0.0f;
            // }
        } });
}

// Implementación de animaciones basadas en estado con Blend Space
void UZombiTurboSequenceProcessor::UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
                                                               FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                               const FZombiStateFragment &StateFragment,
                                                               const FZombiVelocityFragment &VelocityFragment,
                                                               const FZombiChaseFragment &ChaseFragment)
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
    float CurrentSpeed = VelocityFragment.MovementSpeed;

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

    // OPTIMIZACIÓN: Selección directa de animación sin cálculos innecesarios
    UAnimSequence *TargetAnimation = nullptr;

    // Priorizar estado de persecución sobre velocidad
    if (ChaseFragment.bIsChasing)
    {
        // Durante persecución, usar animación de correr
        TargetAnimation = TurboSequenceFragment.CachedRunAnimation;
    }
    else if (NormalizedSpeed < 5.0f)
    {
        TargetAnimation = TurboSequenceFragment.CachedIdleAnimation;
    }
    else if (NormalizedSpeed < 50.0f)
    {
        TargetAnimation = TurboSequenceFragment.CachedWalkAnimation;
    }
    else
    {
        TargetAnimation = TurboSequenceFragment.CachedRunAnimation;
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
