// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "MyTurboSequenceAnimComponent.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

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
        const TConstArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
        const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();

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
            const FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            // Sincronizar transformación
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                TurboSequenceFragment.MeshData,
                FTransform(FQuat(MovementFragment.Rotation), MovementFragment.Position, FVector::OneVector)
            );

            // Actualizar animación basada en estado
            UpdateAnimationBasedOnState(TurboSequenceFragment, StateFragment, MovementFragment);
        } });
}

// Implementación de animaciones basadas en estado
void UZombiTurboSequenceProcessor::UpdateAnimationBasedOnState(const FZombiTurboSequenceFragment &TurboSequenceFragment,
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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Actualizando animación - Estado: %d"), (int32)StateFragment.State);
        AnimationDebugTimer = 0.0f;
    }

    // Obtener la librería de animaciones del asset
    if (!TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
    {
        return;
    }

    // Configurar settings de animación (usando solo campos confirmados)
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    // Seleccionar animación basada en estado
    UAnimSequence *SelectedAnimation = nullptr;

    switch (StateFragment.State)
    {
    case EZombiState::Idle:
        // Buscar animación "MM_Idle" en la librería
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("MM_Idle")))
            {
                SelectedAnimation = AnimItem.Animation;
                break;
            }
        }
        break;

    case EZombiState::Walk:
        // Buscar animación "MM_Walk" en la librería
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("MM_Walk")))
            {
                SelectedAnimation = AnimItem.Animation;
                break;
            }
        }
        break;

    case EZombiState::Chase:
        // Buscar animación "MM_Run" en la librería
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("MM_Run")))
            {
                SelectedAnimation = AnimItem.Animation;
                break;
            }
        }
        break;

    default:
        // Para otros estados, usar Idle como fallback
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("MM_Idle")))
            {
                SelectedAnimation = AnimItem.Animation;
                break;
            }
        }
        break;
    }

    // Reproducir la animación seleccionada
    if (SelectedAnimation)
    {
        ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
            TurboSequenceFragment.MeshData,
            SelectedAnimation,
            PlaySettings);

        // Log de debugging para confirmar reproducción
        static float PlayDebugTimer = 0.0f;
        PlayDebugTimer += LastDeltaTime;
        if (PlayDebugTimer >= 15.0f) // Log cada 15 segundos
        {
            UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Reproduciendo animación - %s"), *SelectedAnimation->GetName());
            PlayDebugTimer = 0.0f;
        }
    }
}
