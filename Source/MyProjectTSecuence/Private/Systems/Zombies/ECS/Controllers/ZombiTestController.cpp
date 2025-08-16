// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Controllers/ZombiTestController.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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

    // Verificar que estamos en el mundo correcto (solo servidor para spawning)
    if (GetNetMode() == NM_Client)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: Ejecutándose en cliente - spawning deshabilitado"));
        return;
    }

    // Verificar que tenemos el asset de TurboSequence asignado
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: No hay TurboSequence Asset asignado. Asigna uno en el Blueprint antes de ejecutar."));
        return;
    }

    // TurboSequence maneja automáticamente la optimización de sombras
    // No necesitamos optimización manual

    // Inicialización robusta del subsystem con reintentos
    InitializeSpawnerSubsystem();

    // Configurar timer de retry para asegurar que el subsystem esté disponible
    GetWorld()->GetTimerManager().SetTimer(RetryTimerHandle, this, &AZombiTestController::RetryInitializeSpawner, 0.5f, true);
}

void AZombiTestController::SpawnZombiBatch(int32 Count)
{
    // Verificar que tenemos el asset configurado
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: No hay TurboSequence Asset asignado. Asigna uno en el Blueprint antes de spawnear."));
        return;
    }

    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: SpawnerSubsystem no disponible - reintentando en el próximo frame"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    Spawner->SpawnZombiBatch(Count, SpawnLocation, SpawnRadius);
}

void AZombiTestController::SpawnSingleZombi()
{
    // Verificar que tenemos el asset configurado
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiTestController: No hay TurboSequence Asset asignado. Asigna uno en el Blueprint antes de spawnear."));
        return;
    }

    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: SpawnerSubsystem no disponible - reintentando en el próximo frame"));
        return;
    }

    // Usa el centro de spawning configurado o la posición del actor
    FVector SpawnLocation = SpawnCenter;
    if (SpawnLocation.IsZero())
    {
        SpawnLocation = GetActorLocation();
    }

    Spawner->SpawnSingleZombi(SpawnLocation);
}

void AZombiTestController::ClearAllZombis()
{
    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: SpawnerSubsystem no disponible - reintentando en el próximo frame"));
        return;
    }

    Spawner->ClearAllZombis();
}

int32 AZombiTestController::GetActiveZombiCount()
{
    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: SpawnerSubsystem no disponible - reintentando en el próximo frame"));
        return 0;
    }

    int32 Count = Spawner->GetActiveZombiCount();
    return Count;
}

void AZombiTestController::SetTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset)
{
    ZombiTurboSequenceAsset = Asset;

    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (Spawner && Asset)
    {
        Spawner->SetZombiTurboSequenceAsset(Asset);
        UE_LOG(LogTemp, Log, TEXT("ZombiTestController: TurboSequence Asset configurado: %s"), *Asset->GetName());
    }
}

// TurboSequence maneja automáticamente la optimización de sombras
// No necesitamos optimización manual

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
    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (Spawner)
    {
        Spawner->ProcessPendingVisualInstances(DeltaTime);
    }

    // Ejecutar SolveMeshes_GameThread para todos los grupos de actualización
    for (int32 GroupIndex = 0; GroupIndex < 4; ++GroupIndex) // 4 grupos como configurado en el spawner
    {
        FTurboSequence_UpdateContext_Lf UpdateContext;
        UpdateContext.GroupIndex = GroupIndex;

        try
        {
            ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, GetWorld(), UpdateContext);
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
    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        return;
    }

    int32 CurrentEntityCount = Spawner->GetActiveZombiCount();

    // Log de cambio de entidades eliminado para optimización de rendimiento
    LastEntityCount = CurrentEntityCount;
}

// Logs centralizados de métricas de rendimiento
void AZombiTestController::LogPerformanceMetrics()
{
    UZombiSpawnerSubsystem *Spawner = GetSpawnerSubsystem();
    if (!Spawner)
    {
        return;
    }

    int32 CurrentEntityCount = Spawner->GetActiveZombiCount();

    // Logs de métricas eliminados para optimización de rendimiento
}

// Inicialización robusta del SpawnerSubsystem con manejo de dependencias
void AZombiTestController::InitializeSpawnerSubsystem()
{
    // Intentar obtener el subsystem
    SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

    if (SpawnerSubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiTestController: SpawnerSubsystem inicializado exitosamente"));

        // Configurar el asset de TurboSequence si está asignado
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

        // Detener el timer de retry ya que el subsystem está disponible
        GetWorld()->GetTimerManager().ClearTimer(RetryTimerHandle);
        bSpawnerInitialized = true;
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiTestController: SpawnerSubsystem no disponible - reintentando..."));
        bSpawnerInitialized = false;
    }
}

// Método de retry para inicialización del SpawnerSubsystem
void AZombiTestController::RetryInitializeSpawner()
{
    // Solo reintentar si aún no está inicializado
    if (!bSpawnerInitialized)
    {
        InitializeSpawnerSubsystem();
    }
}

// Método seguro para obtener el SpawnerSubsystem con reintento
UZombiSpawnerSubsystem *AZombiTestController::GetSpawnerSubsystem()
{
    if (!SpawnerSubsystem)
    {
        // Intentar obtener el subsystem una vez más
        SpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

        if (SpawnerSubsystem)
        {
            UE_LOG(LogTemp, Log, TEXT("ZombiTestController: SpawnerSubsystem obtenido en retry"));
            bSpawnerInitialized = true;
        }
    }

    return SpawnerSubsystem;
}
