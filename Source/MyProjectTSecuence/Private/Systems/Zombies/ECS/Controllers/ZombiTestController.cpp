// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Controllers/ZombiTestController.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

AZombiTestController::AZombiTestController()
{
    PrimaryActorTick.bCanEverTick = true;

    // No llamamos SetActorLabel aquí para evitar problemas durante la creación del Blueprint
}

void AZombiTestController::BeginPlay()
{
    Super::BeginPlay();

    // Hace que este actor sea fácil de encontrar en el mundo (más seguro aquí)
    SetActorLabel(TEXT("ZombiTestController"));

    // Obtiene el subsystem de spawning
    SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

    if (SpawnerSubsystem)
    {
        // Configura el asset de TurboSequence si está asignado
        if (ZombiTurboSequenceAsset)
        {
            SpawnerSubsystem->SetZombiTurboSequenceAsset(ZombiTurboSequenceAsset);
            UE_LOG(LogTemp, Log, TEXT("ZombiTestController: TurboSequence Asset configurado: %s"), *ZombiTurboSequenceAsset->GetName());

            // Spawn automático con delay para asegurar que los subsystems estén listos
            GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AZombiTestController::DelayedSpawn, 1.0f, false);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: No hay TurboSequence Asset asignado"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
    }
}

void AZombiTestController::SpawnZombiBatch(int32 Count)
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    SpawnerSubsystem->SpawnZombiBatch(Count, SpawnLocation, SpawnRadius);
}

void AZombiTestController::SpawnSingleZombi()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    SpawnerSubsystem->SpawnSingleZombi(SpawnLocation);
}

void AZombiTestController::ClearAllZombis()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    SpawnerSubsystem->ClearAllZombis();
}

int32 AZombiTestController::GetActiveZombiCount()
{
    if (!SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: SpawnerSubsystem no disponible"));
        return 0;
    }

    int32 Count = SpawnerSubsystem->GetActiveZombiCount();
    return Count;
}

void AZombiTestController::SetTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset)
{
    ZombiTurboSequenceAsset = Asset;

    if (SpawnerSubsystem && Asset)
    {
        SpawnerSubsystem->SetZombiTurboSequenceAsset(Asset);
        UE_LOG(LogTemp, Log, TEXT("ZombiTestController: TurboSequence Asset configurado: %s"), *Asset->GetName());
    }
}

void AZombiTestController::DelayedSpawn()
{
    SpawnZombiBatch(ZombisPerBatch);
}

void AZombiTestController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    // Inicialización del sistema
    if (!bSystemInitialized)
    {
        // CRÍTICO: Verificar que existe TurboSequence Manager en el mundo
        ATurboSequence_Manager_Lf *TSManager = nullptr;

        // Buscar directamente en el mundo por si Instance no está inicializada
        for (TActorIterator<ATurboSequence_Manager_Lf> ActorItr(GetWorld()); ActorItr; ++ActorItr)
        {
            TSManager = *ActorItr;
            break;
        }

        if (!TSManager && !ATurboSequence_Manager_Lf::Instance)
        {
            UE_LOG(LogTemp, Error, TEXT("❌ ZombiTestController: TurboSequence Manager no encontrado en el mundo. Debe agregar ATurboSequence_Manager_Lf al nivel."));
            return;
        }

        if (TSManager)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ ZombiTestController: TurboSequence Manager encontrado en el mundo: %s"), *TSManager->GetName());
        }

        bSystemInitialized = true;
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiTestController: Sistema inicializado correctamente"));
    }

    // Control centralizado del sistema
    UpdateSystemControl(DeltaTime);

    // Procesa instancias visuales pendientes
    if (SpawnerSubsystem)
    {
        SpawnerSubsystem->ProcessPendingVisualInstances(DeltaTime);
    }

    // CORRECCIÓN CRÍTICA: SolveMeshes_GameThread debe llamarse UNA VEZ por grupo, UNA VEZ por frame
    // Según documentación oficial de TurboSequence
    static int32 CurrentUpdateGroup = 0;
    static float AccumulatedDeltaTime = 0.0f;

    // Acumular DeltaTime para grupos que no se actualizan este frame
    AccumulatedDeltaTime += DeltaTime;

    // Solo procesar un grupo por frame para distribución de carga
    const int32 MaxUpdateGroups = 4;
    if (CurrentUpdateGroup < MaxUpdateGroups)
    {
        FTurboSequence_UpdateContext_Lf UpdateContext;
        UpdateContext.GroupIndex = CurrentUpdateGroup;

        // Usar DeltaTime acumulado para este grupo
        float GroupDeltaTime = (CurrentUpdateGroup == 0) ? DeltaTime : AccumulatedDeltaTime;

        try
        {
            // PATRÓN CORRECTO: Una llamada por grupo, una vez por frame
            ATurboSequence_Manager_Lf::SolveMeshes_GameThread(GroupDeltaTime, GetWorld(), UpdateContext);

            // Reset acumulador para este grupo
            if (CurrentUpdateGroup > 0)
            {
                AccumulatedDeltaTime = 0.0f;
            }
        }
        catch (...)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ ZombiTestController: Error en SolveMeshes_GameThread para grupo %d"), CurrentUpdateGroup);
        }
    }

    // Rotar al siguiente grupo
    CurrentUpdateGroup = (CurrentUpdateGroup + 1) % MaxUpdateGroups;
}

// Control centralizado del sistema
void AZombiTestController::UpdateSystemControl(float DeltaTime)
{
    // Actualizar timers de logs
    SystemLogTimer += DeltaTime;
    PerformanceLogTimer += DeltaTime;

    // Logs eliminados para optimización de rendimiento
}

// Logs centralizados del estado del sistema
void AZombiTestController::LogSystemStatus()
{
    if (!SpawnerSubsystem)
    {
        return;
    }

    int32 CurrentEntityCount = SpawnerSubsystem->GetActiveZombiCount();

    if (CurrentEntityCount != LastEntityCount)
    {

        LastEntityCount = CurrentEntityCount;
    }
}

// Logs centralizados de métricas de rendimiento
void AZombiTestController::LogPerformanceMetrics()
{
    if (!SpawnerSubsystem)
    {
        return;
    }

    int32 CurrentEntityCount = SpawnerSubsystem->GetActiveZombiCount();
}
