// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

UZombiTurboSequenceProcessor::UZombiTurboSequenceProcessor()
{
    // Rehabilitar procesador para sincronización de transformaciones
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true; // Rehabilitar registro automático
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query para sincronizar transformaciones
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);

    // Query para controlar animaciones con Blend Space (temporalmente deshabilitada)
    // VisualInstanceQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    // VisualInstanceQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Sincronizar transformaciones
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                          {
        const TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
        
        for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[EntityIndex];
            const FZombiMovementFragment& MovementFragment = MovementFragments[EntityIndex];
            
            if (TurboSequenceFragment.MeshData.IsMeshDataValid())
            {
                // Sincronizar transformación con TurboSequence
                FTransform MeshTransform;
                MeshTransform.SetLocation(MovementFragment.Position);
                MeshTransform.SetRotation(FQuat(MovementFragment.Rotation));
                MeshTransform.SetScale3D(FVector::OneVector);
                
                ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                    TurboSequenceFragment.MeshData,
                    MeshTransform);
            }
        } });

    // TEMPORAL: Animaciones deshabilitadas porque SolveMeshes causa crash
    /*
    // Controlar animaciones con Blend Space
    VisualInstanceQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
    {
        const TArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetMutableFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
        {
            FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[EntityIndex];
            const FZombiStateFragment& StateFragment = StateFragments[EntityIndex];

            UpdateBlendSpaceAnimation(TurboSequenceFragment, StateFragment);
        }
    });
    */
}

void UZombiTurboSequenceProcessor::UpdateBlendSpaceAnimation(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                             const FZombiStateFragment &StateFragment)
{
    // TEMPORAL: Deshabilitar Blend Space para estabilidad
    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: UpdateBlendSpaceAnimation temporalmente deshabilitado"));
    return;

    /*
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // TEMPORAL: Deshabilitar Blend Space para estabilidad
    // Una vez que el sistema básico esté estable, implementaremos el Blend Space completo
    static float LogTimer = 0.0f;
    static float LastTime = 0.0f;
    float CurrentTime = FPlatformTime::Seconds();
    LogTimer += (CurrentTime - LastTime);
    LastTime = CurrentTime;

    if (LogTimer >= 15.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Blend Space temporalmente deshabilitado para estabilidad (Estado: %d)"),
               (int32)StateFragment.State);
        LogTimer = 0.0f;
    }

    // Código comentado temporalmente para evitar crashes
    // Determinar la posición del Blend Space según el estado
    FVector3f BlendSpacePosition = FVector3f::ZeroVector;

    switch (StateFragment.State)
    {
    case EZombiState::Idle:
        BlendSpacePosition = FVector3f(0.0f, 0.0f, 0.0f); // Posición central (Idle)
        break;
    case EZombiState::Walk:
        BlendSpacePosition = FVector3f(0.5f, 0.0f, 0.0f); // Posición media (Walk)
        break;
    case EZombiState::Chase:
        BlendSpacePosition = FVector3f(1.0f, 0.0f, 0.0f); // Posición máxima (Run)
        break;
    case EZombiState::Attack:
        BlendSpacePosition = FVector3f(0.0f, 1.0f, 0.0f); // Posición Y (Attack)
        break;
    case EZombiState::Hit:
        BlendSpacePosition = FVector3f(0.0f, 0.5f, 0.0f); // Posición Y media (Hit)
        break;
    case EZombiState::Death:
        BlendSpacePosition = FVector3f(0.0f, 0.0f, 1.0f); // Posición Z (Death)
        break;
    default:
        BlendSpacePosition = FVector3f(0.0f, 0.0f, 0.0f);
        break;
    }

    // Si tenemos un Blend Space configurado, ajustar su posición
    if (TurboSequenceFragment.BlendSpaceData.IsAnimCollectionValid())
    {
        ATurboSequence_Manager_Lf::TweakBlendSpace_Concurrent(
            TurboSequenceFragment.BlendSpaceData,
            BlendSpacePosition);

        static float LogTimer = 0.0f;
        static float LastTime = 0.0f;
        float CurrentTime = FPlatformTime::Seconds();
        LogTimer += (CurrentTime - LastTime);
        LastTime = CurrentTime;

        if (LogTimer >= 10.0f)
        {
            UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Ajustando Blend Space a posición (%.1f, %.1f, %.1f) para estado %d"),
                   BlendSpacePosition.X, BlendSpacePosition.Y, BlendSpacePosition.Z, (int32)StateFragment.State);
            LogTimer = 0.0f;
        }
    }
    else
    {
        // Si no hay Blend Space configurado, usar animación por defecto
        static float LogTimer = 0.0f;
        static float LastTime = 0.0f;
        float CurrentTime = FPlatformTime::Seconds();
        LogTimer += (CurrentTime - LastTime);
        LastTime = CurrentTime;

        if (LogTimer >= 15.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiTurboSequenceProcessor: No hay Blend Space configurado para entidad"));
            LogTimer = 0.0f;
        }
    }
    */
}
