// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUltraConsolidatedFragment.h"
// Helpers eliminados - lógica movida al procesador ultra-consolidado
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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTurboSequenceProcessor: Procesador optimizado inicializado"));
        bLoggedConstructor = true;
    }
}

void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query para sincronizar transformaciones y animaciones (State Sync) - optimizado
    TransformSyncQuery.AddRequirement<FZombiUltraConsolidatedFragment>(EMassFragmentAccess::ReadWrite);
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
        TArrayView<FZombiUltraConsolidatedFragment> UltraFragments = Context.GetMutableFragmentView<FZombiUltraConsolidatedFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            FZombiUltraConsolidatedFragment& UltraFragment = UltraFragments[i];

            // SINCRONIZAR TRANSFORMACIÓN con TurboSequence
            if (UltraFragment.IsMeshDataValid())
            {
                // Crear transformación desde los datos del fragmento ultra-consolidado
                FTransform NewTransform;
                NewTransform.SetLocation(UltraFragment.Position);
                
                // CORREGIR ROTACIÓN: Compensar la diferencia de 90 grados entre animación y transformación
                FRotator CorrectedRotation = UltraFragment.Rotation;
                CorrectedRotation.Yaw -= 90.0f; // Compensar la diferencia de orientación
                
                NewTransform.SetRotation(CorrectedRotation.Quaternion());
                NewTransform.SetScale3D(FVector::OneVector);

                // Aplicar transformación usando el manager de TurboSequence
                ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
                    UltraFragment.MeshData,
                    NewTransform);
            }
        } });
}

// Método simplificado para el fragmento ultra-consolidado
void UZombiTurboSequenceProcessor::UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
                                                               FZombiUltraConsolidatedFragment &UltraFragment)
{
    // Con el fragmento ultra-consolidado, las animaciones se manejan automáticamente por TurboSequence
    // Solo necesitamos sincronizar transformaciones
}
