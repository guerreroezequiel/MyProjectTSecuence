// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTestController.h"
#include "ZombiSpawnerSubsystem.h"
#include "ZombiMassSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

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
        bSystemInitialized = true;
    }

    // Control centralizado del sistema
    UpdateSystemControl(DeltaTime);

    // Procesa instancias visuales pendientes
    if (SpawnerSubsystem)
    {
        SpawnerSubsystem->ProcessPendingVisualInstances(DeltaTime);
    }

    // Ejecutar SolveMeshes_GameThread para todos los grupos de actualización
    // Esto debe llamarse una vez por frame como en el ejemplo de TurboSequence
    static float SolveMeshesLogTimer = 0.0f;
    SolveMeshesLogTimer += DeltaTime;

    for (int32 GroupIndex = 0; GroupIndex < 4; ++GroupIndex) // 4 grupos como configurado en el spawner
    {
        FTurboSequence_UpdateContext_Lf UpdateContext;
        UpdateContext.GroupIndex = GroupIndex;

        try
        {
            ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), UpdateContext);

            // Log eliminado para optimización de rendimiento
        }
        catch (...)
        {
            UE_LOG(LogTemp, Warning, TEXT("❌ ZombiTestController: Error en SolveMeshes_GameThread para grupo %d"), GroupIndex);
        }
    }

    // Timer reset eliminado para optimización
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
        UE_LOG(LogTemp, Warning, TEXT("🎮 ZombiTestController: SpawnerSubsystem no disponible"));
        return;
    }

    int32 CurrentEntityCount = SpawnerSubsystem->GetActiveZombiCount();

    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTestController: Estado del Sistema:"));
    UE_LOG(LogTemp, Log, TEXT("  📊 Entidades activas: %d"), CurrentEntityCount);
    UE_LOG(LogTemp, Log, TEXT("  🎯 Sistema funcionando: %s"), CurrentEntityCount > 0 ? TEXT("✅") : TEXT("❌"));

    if (CurrentEntityCount != LastEntityCount)
    {
        UE_LOG(LogTemp, Log, TEXT("  📈 Cambio en entidades: %d → %d"), LastEntityCount, CurrentEntityCount);
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

    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiTestController: Métricas de Rendimiento:"));
    UE_LOG(LogTemp, Log, TEXT("  🚀 Entidades procesadas: %d"), CurrentEntityCount);
    UE_LOG(LogTemp, Log, TEXT("  ⚡ Sistema estable: %s"), CurrentEntityCount > 0 ? TEXT("✅") : TEXT("❌"));
}
