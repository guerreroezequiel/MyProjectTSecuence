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

            // Sincronizar transformación con rotación hacia la dirección de movimiento
            FQuat RotationQuat = FQuat(MovementFragment.Rotation);
            
            // Si hay movimiento, calcular la rotación hacia la dirección
            if (!MovementFragment.MovementDirection.IsNearlyZero() && MovementFragment.MovementSpeed > 0.0f)
            {
                // Calcular rotación hacia la dirección de movimiento
                FRotator DirectionRotation = MovementFragment.MovementDirection.Rotation();
                
                // Interpolar suavemente la rotación para evitar giros bruscos
                FRotator CurrentRotation = MovementFragment.Rotation;
                FRotator TargetRotation = DirectionRotation;
                
                // Interpolación suave con velocidad de rotación
                float RotationSpeed = 5.0f; // Velocidad de rotación en radianes por segundo
                RotationQuat = FQuat(FMath::RInterpTo(CurrentRotation, TargetRotation, Context.GetDeltaTimeSeconds(), RotationSpeed));
            }
            
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                TurboSequenceFragment.MeshData,
                FTransform(RotationQuat, MovementFragment.Position, FVector::OneVector)
            );

            // Actualizar animación basada en estado
            UpdateAnimationBasedOnState(Context, i, TurboSequenceFragment, StateFragment, MovementFragment);
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
    static float LastDeltaTime = 0.0f;
    AnimationDebugTimer += LastDeltaTime;
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

    // Usar TweakAnimation_Concurrent para Blend Space
    // Esto es más eficiente que PlayAnimation_Concurrent para transiciones suaves
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    // Log de debugging para Blend Space
    if (AnimationDebugTimer >= 10.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 Blend Space: Velocidad: %.2f, Normalizada: %.2f"), CurrentSpeed, NormalizedSpeed);
    }

    // USAR ANIMACIONES CON TRANSICIONES SUAVES - POR ENTIDAD
    // Cada entidad tiene su propio estado de animación
    TurboSequenceFragment.AnimationUpdateTimer += Context.GetDeltaTimeSeconds();

    // Solo actualizar cuando cambie significativamente la velocidad o cada 3 segundos
    bool bShouldUpdateAnimation = (FMath::Abs(TurboSequenceFragment.LastSpeed - NormalizedSpeed) > 10.0f) ||
                                  (TurboSequenceFragment.AnimationUpdateTimer >= 3.0f);

    if (!TurboSequenceFragment.bAnimationInitialized || bShouldUpdateAnimation)
    {
        // Buscar la animación más apropiada basada en velocidad
        if (TurboSequenceFragment.TurboSequenceAsset && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
        {
            UAnimSequence *SelectedAnimation = nullptr;

            // Seleccionar animación basada en velocidad con umbrales más claros
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
