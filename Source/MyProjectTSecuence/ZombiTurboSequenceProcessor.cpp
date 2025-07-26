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
    // Query para sincronizar transformaciones (State Sync)
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    TransformSyncQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);

    // TODO: Query para Blend Space (futuro)
    // BlendSpaceQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    // BlendSpaceQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    // BlendSpaceQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);

    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: ConfigureQueries completado - preparado para Blend Space"));
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Sincronizar transformaciones (State Sync)
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                          {
        const TConstArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            const FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiMovementFragment& MovementFragment = MovementFragments[i];

            // Sincronizar transformación
            ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                TurboSequenceFragment.MeshData,
                FTransform(FQuat(MovementFragment.Rotation), MovementFragment.Position, FVector::OneVector)
            );
        } });

    // TODO: Implementar Blend Space Query aquí
    // BlendSpaceQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext& Context)
    // {
    //     // Actualizar Blend Space basado en estado del zombi
    //     // Esto será implementado cuando resolvamos el problema de animaciones
    // });
}

// Implementación de Blend Space Animation (preparada para futuro uso)
void UZombiTurboSequenceProcessor::UpdateBlendSpaceAnimation(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                             const FZombiStateFragment &StateFragment,
                                                             const FZombiMovementFragment &MovementFragment)
{
    // Esta función será implementada cuando resolvamos el problema de animaciones
    // Enfoque: Blend Space basado en velocidad y estado del zombi

    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        return;
    }

    // TODO: Implementar lógica de Blend Space
    // - Idle cuando velocidad = 0
    // - Walk cuando velocidad < threshold
    // - Run cuando velocidad > threshold

    // Por ahora, solo un placeholder para evitar la advertencia de compilación
    UE_LOG(LogTemp, Verbose, TEXT("ZombiTurboSequenceProcessor: UpdateBlendSpaceAnimation llamada (placeholder)"));
}
