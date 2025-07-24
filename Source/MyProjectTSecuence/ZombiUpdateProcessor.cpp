// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiUpdateProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

UZombiUpdateProcessor::UZombiUpdateProcessor()
{
    // Se ejecuta al final del frame para resolver todas las animaciones
    ExecutionFlags = (int32)(EProcessorExecutionFlags::All);
    ExecutionOrder.ExecuteInGroup = TEXT("BehaviorBeginFrame");
}

void UZombiUpdateProcessor::ConfigureQueries()
{
    // Este procesador no necesita queries específicos
    // Solo se ejecuta una vez por frame para resolver Update Groups
}

void UZombiUpdateProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();
    UWorld *World = GetWorld();

    // Resuelve todos los grupos de actualización
    // Esto distribuye la carga de procesamiento de animaciones
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
    ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), UpdateContext);
}
