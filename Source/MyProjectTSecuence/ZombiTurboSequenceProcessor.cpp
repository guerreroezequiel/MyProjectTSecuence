// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiStateFragment.h"
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
    TransformSyncQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);

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

    // Log de debugging para verificar ejecución
    static float DebugTimer = 0.0f;
    DebugTimer += DeltaTime;
    if (DebugTimer >= 5.0f) // Log cada 5 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Ejecutándose - DeltaTime: %f"), DeltaTime);
        DebugTimer = 0.0f;
    }

    // Sincronizar transformaciones y animaciones (State Sync)
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                          {
        TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        TArrayView<const FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
        TArrayView<const FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        // Log de debugging para verificar entidades procesadas
        static float EntityDebugTimer = 0.0f;
        EntityDebugTimer += DeltaTime;
        if (EntityDebugTimer >= 5.0f && Context.GetNumEntities() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Procesando %d entidades"), Context.GetNumEntities());
            EntityDebugTimer = 0.0f;
        }

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            // Actualizar animación basada en estado PRIMERO
            UpdateAnimationBasedOnState(Context, i, TurboSequenceFragment, StateFragment, MovementFragment);
            
            // Sincronizar transformación usando la rotación ya calculada en MovementProcessor DESPUÉS
            // Aplicar offset de -90° para corregir la orientación del asset de TurboSequence
            FRotator AdjustedRotation = MovementFragment.Rotation;
            AdjustedRotation.Yaw -= 90.0f; // Offset de -90° para corregir la orientación del asset
            FQuat RotationQuat = FQuat(AdjustedRotation);
            
            // Log de debugging para rotación (solo ocasionalmente)
            static float RotationDebugTimer = 0.0f;
            RotationDebugTimer += Context.GetDeltaTimeSeconds();
            if (RotationDebugTimer >= 15.0f && MovementFragment.MovementSpeed > 0.0f)
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 TurboSequence Rotación: Velocidad: %.2f, Dirección: %s, Rotación Original: %s, Rotación Ajustada: %s, Quat: %s, ForwardVector: %s"),
                       MovementFragment.MovementSpeed, 
                       *MovementFragment.MovementDirection.ToString(),
                       *MovementFragment.Rotation.ToString(),
                       *AdjustedRotation.ToString(),
                       *RotationQuat.ToString(),
                       *AdjustedRotation.Vector().ToString());
                RotationDebugTimer = 0.0f;
            }
            
            // Crear la transformación final
            FTransform FinalTransform = FTransform(RotationQuat, MovementFragment.Position, FVector::OneVector);
            
            // Aplicar la transformación a TurboSequence DESPUÉS de la animación
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                TurboSequenceFragment.MeshData,
                FinalTransform
            );
            
            // Log de debugging para verificar si la transformación se aplicó
            static float TransformAppliedDebugTimer = 0.0f;
            TransformAppliedDebugTimer += Context.GetDeltaTimeSeconds();
            if (TransformAppliedDebugTimer >= 25.0f && MovementFragment.MovementSpeed > 0.0f)
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 Transformación Aplicada: MeshData Válido: %s, Posición: %s, Rotación: %s"),
                       TurboSequenceFragment.MeshData.IsMeshDataValid() ? TEXT("SÍ") : TEXT("NO"),
                       *MovementFragment.Position.ToString(),
                       *MovementFragment.Rotation.ToString());
                TransformAppliedDebugTimer = 0.0f;
            }
            
            // Log de debugging para transformación (solo ocasionalmente)
            static float TransformDebugTimer = 0.0f;
            TransformDebugTimer += Context.GetDeltaTimeSeconds();
            if (TransformDebugTimer >= 20.0f && MovementFragment.MovementSpeed > 0.0f)
            {
                FVector ForwardVector = MovementFragment.Rotation.Vector();
                FVector MovementDirection = MovementFragment.MovementDirection;
                float DotProduct = FVector::DotProduct(ForwardVector, MovementDirection);
                
                UE_LOG(LogTemp, Log, TEXT("🎮 Transformación Final: Posición: %s, Rotación: %s, ForwardVector: %s, MovementDirection: %s, DotProduct: %.3f"),
                       *MovementFragment.Position.ToString(),
                       *MovementFragment.Rotation.ToString(),
                       *ForwardVector.ToString(),
                       *MovementDirection.ToString(),
                       DotProduct);
                TransformDebugTimer = 0.0f;
            }

            // Log de orden de ejecución
            static float ExecutionOrderDebugTimer = 0.0f;
            ExecutionOrderDebugTimer += Context.GetDeltaTimeSeconds();
            if (ExecutionOrderDebugTimer >= 30.0f && MovementFragment.MovementSpeed > 0.0f)
            {
                UE_LOG(LogTemp, Log, TEXT("🎮 Orden de Ejecución: 1. Animación actualizada, 2. Transformación aplicada, Velocidad: %.2f, Rotación: %s"),
                       MovementFragment.MovementSpeed, *MovementFragment.Rotation.ToString());
                ExecutionOrderDebugTimer = 0.0f;
            }
        } });
}

// Implementación de animaciones basadas en estado con Blend Space
void UZombiTurboSequenceProcessor::UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
                                                               FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                               const FZombiStateFragment &StateFragment,
                                                               const FZombiMovementFragment &MovementFragment)
{
    // Verificar que tenemos todo lo necesario
    if (!TurboSequenceFragment.TurboSequenceAsset || !TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Log de debugging para verificar ejecución de animaciones
    static float AnimationDebugTimer = 0.0f;
    AnimationDebugTimer += Context.GetDeltaTimeSeconds();
    if (AnimationDebugTimer >= 10.0f) // Log cada 10 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Actualizando animación - Estado: %d, Velocidad: %.2f"),
               (int32)StateFragment.State, MovementFragment.MovementSpeed);
        AnimationDebugTimer = 0.0f;
    }

    // Obtener la librería de animaciones del asset
    if (!TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
    {
        return;
    }

    // USAR BLEND SPACE DIRECTAMENTE - Enfoque correcto según documentación
    float CurrentSpeed = MovementFragment.MovementSpeed;

    // Normalizar velocidad al rango del Blend Space (0-100)
    float NormalizedSpeed = FMath::Clamp(CurrentSpeed, 0.0f, 100.0f);

    // Configuración para Blend Space
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    // Log de debugging para Blend Space
    if (AnimationDebugTimer >= 10.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 Blend Space: Velocidad: %.2f, Normalizada: %.2f"), CurrentSpeed, NormalizedSpeed);
    }

    // USAR BLEND SPACE CON TURBOSEQUENCE - Enfoque correcto según documentación
    TurboSequenceFragment.AnimationUpdateTimer += Context.GetDeltaTimeSeconds();

    // Solo actualizar cuando cambie significativamente la velocidad o cada 3 segundos
    bool bShouldUpdateAnimation = (FMath::Abs(TurboSequenceFragment.LastSpeed - NormalizedSpeed) > 10.0f) ||
                                  (TurboSequenceFragment.AnimationUpdateTimer >= 3.0f);

    if (!TurboSequenceFragment.bAnimationInitialized || bShouldUpdateAnimation)
    {
        // USAR BLEND SPACE CON TURBOSEQUENCE - Enfoque correcto según documentación
        if (TurboSequenceFragment.TurboSequenceAsset && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
        {
            // Buscar Blend Space en la librería
            UBlendSpace *SelectedBlendSpace = nullptr;
            if (TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->BlendSpaces.Num() > 0)
            {
                SelectedBlendSpace = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->BlendSpaces[0];
            }

            if (SelectedBlendSpace)
            {
                // Usar PlayBlendSpace_Concurrent para Blend Space
                TurboSequenceFragment.BlendSpaceData = ATurboSequence_Manager_Lf::PlayBlendSpace_Concurrent(
                    TurboSequenceFragment.MeshData,
                    SelectedBlendSpace,
                    PlaySettings);

                // Usar TweakBlendSpace_Concurrent para ajustar la posición en el Blend Space
                FVector3f BlendSpacePosition(NormalizedSpeed, 0.0f, 0.0f); // Velocidad en X, dirección en Y
                bool bBlendSpaceTweaked = ATurboSequence_Manager_Lf::TweakBlendSpace_Concurrent(
                    TurboSequenceFragment.BlendSpaceData,
                    BlendSpacePosition);

                TurboSequenceFragment.LastSpeed = NormalizedSpeed;
                TurboSequenceFragment.AnimationUpdateTimer = 0.0f;
                TurboSequenceFragment.bAnimationInitialized = true;
                TurboSequenceFragment.LastAnimationUpdateTime = Context.GetDeltaTimeSeconds();

                // Log de debugging para Blend Space
                UE_LOG(LogTemp, Log, TEXT("🎮 Blend Space Aplicado: Velocidad: %.2f, Posición: %s, Tweaked: %s"),
                       NormalizedSpeed, *BlendSpacePosition.ToString(), bBlendSpaceTweaked ? TEXT("SÍ") : TEXT("NO"));
            }
            else
            {
                // Fallback a animaciones individuales si no hay Blend Space
                UAnimSequence *SelectedAnimation = nullptr;

                // Seleccionar animación basada en velocidad
                if (NormalizedSpeed < 5.0f)
                {
                    // IDLE - Buscar animación de idle
                    for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
                    {
                        if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
                        {
                            SelectedAnimation = AnimItem.Animation;
                            break;
                        }
                    }
                }
                else if (NormalizedSpeed < 50.0f)
                {
                    // WALK - Buscar animación de caminar
                    for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
                    {
                        if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Walk"), ESearchCase::IgnoreCase))
                        {
                            SelectedAnimation = AnimItem.Animation;
                            break;
                        }
                    }
                }
                else
                {
                    // RUN - Buscar animación de correr
                    for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
                    {
                        if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Run"), ESearchCase::IgnoreCase))
                        {
                            SelectedAnimation = AnimItem.Animation;
                            break;
                        }
                    }
                }

                // Si no encontramos animación específica, usar la primera disponible
                if (!SelectedAnimation && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() > 0)
                {
                    SelectedAnimation = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation;
                }

                // Reproducir la animación seleccionada
                if (SelectedAnimation)
                {
                    ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                        TurboSequenceFragment.MeshData,
                        SelectedAnimation,
                        PlaySettings);

                    TurboSequenceFragment.LastSpeed = NormalizedSpeed;
                    TurboSequenceFragment.AnimationUpdateTimer = 0.0f;
                    TurboSequenceFragment.bAnimationInitialized = true;
                    TurboSequenceFragment.LastAnimationUpdateTime = Context.GetDeltaTimeSeconds();

                    // Log de debugging para animación individual
                    UE_LOG(LogTemp, Log, TEXT("🎮 Animación Individual: %s, Velocidad: %.2f"),
                           *SelectedAnimation->GetName(), NormalizedSpeed);
                }
            }
        }
    }

    // Log de debugging para confirmar reproducción (solo para la primera entidad)
    static float PlayDebugTimer = 0.0f;
    PlayDebugTimer += Context.GetDeltaTimeSeconds();
    if (PlayDebugTimer >= 15.0f) // Log cada 15 segundos
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Animación - Velocidad: %.2f, Normalizada: %.2f, Inicializada: %s, Entidades: %d"),
               CurrentSpeed, NormalizedSpeed, TurboSequenceFragment.bAnimationInitialized ? TEXT("Sí") : TEXT("No"), Context.GetNumEntities());
        PlayDebugTimer = 0.0f;
    }
}
