// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiSpawnerSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "ZombiStateFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMassSubsystem.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "Engine/Engine.h"

// Constructor del subsystem
UZombiSpawnerSubsystem::UZombiSpawnerSubsystem()
{
}

// Inicialización: obtiene referencia al Mass Entity Subsystem
void UZombiSpawnerSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Obtiene el MassEntitySubsystem
    MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
}

// Limpieza al destruir el subsystem
void UZombiSpawnerSubsystem::Deinitialize()
{
    // Limpia todas las entidades antes de destruir
    ClearAllZombis();

    Super::Deinitialize();
}

// Spawna un lote de zombis de manera optimizada
void UZombiSpawnerSubsystem::SpawnZombiBatch(int32 Count, const FVector &CenterLocation, float SpawnRadius)
{
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("TurboSequence Asset no asignado. Usa SetZombiTurboSequenceAsset primero"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Iniciando spawn de %d zombis en %s (radio: %.1f)"),
           Count, *CenterLocation.ToString(), SpawnRadius);

    // Spawna los zombis en lotes para optimizar rendimiento
    const int32 BatchSize = 100; // Procesa en lotes de 100 para evitar bloqueos
    int32 RemainingCount = Count;
    int32 SuccessfullySpawned = 0;

    while (RemainingCount > 0)
    {
        int32 CurrentBatchSize = FMath::Min(BatchSize, RemainingCount);

        for (int32 i = 0; i < CurrentBatchSize; ++i)
        {
            FVector SpawnLocation = GenerateRandomSpawnLocation(CenterLocation, SpawnRadius);
            FMassEntityHandle EntityHandle = CreateZombiMassEntity(SpawnLocation);

            if (EntityHandle.IsValid())
            {
                SpawnedEntities.Add(EntityHandle);
                ActiveZombiCount++;
                SuccessfullySpawned++;
            }
        }

        RemainingCount -= CurrentBatchSize;
    }

    // UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Spawn completado - %d zombis creados exitosamente, Total activos: %d"),
    //        SuccessfullySpawned, ActiveZombiCount);
}

// Spawna un zombi individual
void UZombiSpawnerSubsystem::SpawnSingleZombi(const FVector &Location)
{
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("TurboSequence Asset no asignado para spawning individual"));
        return;
    }

    FMassEntityHandle EntityHandle = CreateZombiMassEntity(Location);
    if (EntityHandle.IsValid())
    {
        SpawnedEntities.Add(EntityHandle);
        ActiveZombiCount++;
    }
}

// Configura el asset de TurboSequence para los zombis
void UZombiSpawnerSubsystem::SetZombiTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset)
{
    ZombiTurboSequenceAsset = Asset;
    UE_LOG(LogTemp, Log, TEXT("TurboSequence Asset configurado: %s"), Asset ? *Asset->GetName() : TEXT("NULL"));
}

// Limpia todos los zombis
void UZombiSpawnerSubsystem::ClearAllZombis()
{
    // Usa el nuevo ZombiMassSubsystem para limpiar entidades
    if (UZombiMassSubsystem *ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>())
    {
        ZombiMassSubsystem->ClearAllEntities();
        SpawnedEntities.Empty();
        ActiveZombiCount = 0;
    }
}

// Crea una entidad Mass con todos los fragmentos necesarios
FMassEntityHandle UZombiSpawnerSubsystem::CreateZombiMassEntity(const FVector &SpawnLocation)
{
    // Usa el nuevo ZombiMassSubsystem para crear entidades puras
    if (UZombiMassSubsystem *ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>())
    {
        FMassEntityHandle EntityHandle = ZombiMassSubsystem->RegisterZombiEntity(SpawnLocation, ZombiTurboSequenceAsset);

        // Crea la instancia visual inmediatamente después de crear la entidad lógica
        if (EntityHandle.IsValid() && ZombiTurboSequenceAsset)
        {
            CreateTurboSequenceVisualInstance(EntityHandle, SpawnLocation);
        }

        return EntityHandle;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No se pudo obtener ZombiMassSubsystem"));
        return FMassEntityHandle();
    }
}

// Genera una posición aleatoria dentro del radio especificado
FVector UZombiSpawnerSubsystem::GenerateRandomSpawnLocation(const FVector &Center, float Radius) const
{
    // Genera un ángulo aleatorio
    float RandomAngle = FMath::RandRange(0.0f, 360.0f);
    float RandomRadius = FMath::RandRange(0.0f, Radius);

    // Convierte a coordenadas cartesianas
    float X = Center.X + RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle));
    float Y = Center.Y + RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle));
    float Z = Center.Z; // Mantiene la altura del centro

    return FVector(X, Y, Z);
}

// Genera una rotación aleatoria
FRotator UZombiSpawnerSubsystem::GenerateRandomRotation() const
{
    return FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
}

// Crea la instancia visual de TurboSequence para una entidad
void UZombiSpawnerSubsystem::CreateTurboSequenceVisualInstance(FMassEntityHandle EntityHandle, const FVector &SpawnLocation)
{
    if (!ZombiTurboSequenceAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("TurboSequence Asset no asignado para crear instancia visual"));
        return;
    }

    // Crea la transformación de spawn
    FTransform SpawnTransform(GenerateRandomRotation(), SpawnLocation, FVector::OneVector);

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Creando instancia visual para entidad %d en posición %s"),
           EntityHandle.Index, *SpawnLocation.ToString());

    // Crea la instancia visual usando las mejores prácticas de TurboSequence
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = ZombiTurboSequenceAsset;

    FTurboSequence_MinimalMeshData_Lf MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    // Agrega a un grupo de actualización para optimización
    if (MeshData.IsMeshDataValid())
    {
        ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
            FMath::RandRange(0, 3), // Distribuye en 4 grupos
            MeshData);

        // Guarda la referencia de la instancia visual localmente
        VisualInstances.Add(EntityHandle, MeshData);

        // Actualiza el fragmento visual de la entidad Mass
        if (UZombiMassSubsystem *ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>())
        {
            if (UMassEntitySubsystem *LocalMassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>())
            {
                FMassEntityManager &EntityManager = LocalMassEntitySubsystem->GetMutableEntityManager();

                // Actualiza el fragmento visual de la entidad usando la API correcta de UE5.5.4
                if (EntityManager.IsEntityValid(EntityHandle))
                {
                    // Usar MassEntityUtils para acceder al fragmento
                    if (FZombiTurboSequenceFragment *TurboSequenceFragment = EntityManager.GetFragmentDataPtr<FZombiTurboSequenceFragment>(EntityHandle))
                    {
                        TurboSequenceFragment->MeshData = MeshData;
                        TurboSequenceFragment->bIsVisualInstanceValid = true;
                        TurboSequenceFragment->UpdateGroupIndex = FMath::RandRange(0, 3);

                        // UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Fragmento visual actualizado para entidad %d"), EntityHandle.Index);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: Entidad %d no tiene fragmento visual"), EntityHandle.Index);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: Entidad %d no es válida"), EntityHandle.Index);
                }
            }
        }

        // UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Instancia visual creada exitosamente - Instancias totales: %d"),
        //        VisualInstances.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: Error al crear instancia visual para entidad %d"), EntityHandle.Index);
    }
}
