// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiUpdateProcessor.h"
#include "MassProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

UZombiUpdateProcessor::UZombiUpdateProcessor()
{
    // Rehabilitar procesador pero con cuidado
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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiUpdateProcessor: Procesador inicializado"));
        bLoggedConstructor = true;
    }
}

void UZombiUpdateProcessor::ConfigureQueries()
{
    // Query para procesar grupos de actualización de TurboSequence
    UpdateGroupQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
}

void UZombiUpdateProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Log cada 60 segundos (aproximadamente una vez por minuto)
    static float LogTimer = 0.0f;
    LogTimer += DeltaTime;

    if (LogTimer >= 60.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiUpdateProcessor: Ejecutándose - DeltaTime: %f"), DeltaTime);
        LogTimer = 0.0f;
    }

    // TEMPORAL: Comentar SolveMeshes_GameThread porque causa crash
    // ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), UpdateContext);

    if (LogTimer == 0.0f) // Solo loggear cuando se resetea el timer
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiUpdateProcessor: SolveMeshes_GameThread temporalmente deshabilitado (causa crash)"));
    }
}
