// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiUpdateProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

UZombiUpdateProcessor::UZombiUpdateProcessor()
{
    // Configuración correcta para procesadores Mass Entity en UE5.5.4
    ExecutionFlags = (int32)(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    UE_LOG(LogTemp, Log, TEXT("ZombiUpdateProcessor: Constructor llamado - Procesador creado"));
}

void UZombiUpdateProcessor::ConfigureQueries()
{
    // Este procesador no necesita queries específicos
}

void UZombiUpdateProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();
    for (int32 GroupIndex = 0; GroupIndex < NUM_UPDATE_GROUPS; ++GroupIndex)
    {
        SolveUpdateGroup(GroupIndex, DeltaTime);
    }
}

void UZombiUpdateProcessor::SolveUpdateGroup(int32 GroupIndex, float DeltaTime)
{
    // Configura el contexto de actualización para este grupo
    UpdateContext.GroupIndex = GroupIndex;

    // Resuelve las animaciones para este grupo
    // Esta es la llamada esencial para que TurboSequence funcione correctamente
    // Por ahora, comentamos esta llamada hasta que resolvamos el problema de contexto
    // ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), UpdateContext);

    UE_LOG(LogTemp, Log, TEXT("ZombiUpdateProcessor: SolveUpdateGroup llamado para grupo %d"), GroupIndex);
}
