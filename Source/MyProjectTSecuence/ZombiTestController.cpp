// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTestController.h"
#include "ZombiSpawnerSubsystem.h"
#include "ZombiMassSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
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

    // Log temporal para verificar que Tick se ejecuta
    static float TickLogTimer = 0.0f;
    TickLogTimer += DeltaTime;
    if (TickLogTimer >= 1.0f)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiTestController: Tick ejecutándose - DeltaTime: %f"), DeltaTime);
        TickLogTimer = 0.0f;
    }

    // Procesar reintentos de instancias visuales pendientes
    if (SpawnerSubsystem)
    {
        SpawnerSubsystem->ProcessPendingVisualInstances(DeltaTime);
    }

    // Ejecuta los procesadores manualmente cada frame
    if (UZombiMassSubsystem *ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>())
    {
        ZombiMassSubsystem->ExecuteProcessorsManually(DeltaTime);
    }
}
