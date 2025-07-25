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

    // Verificar y obtener MassEntitySubsystem
    if (!MassEntitySubsystem)
    {
        // Reintentar obtener el subsystem
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: MassEntitySubsystem no disponible - reintentando en el siguiente frame"));
            return;
        }
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    FZombiTurboSequenceFragment &TurboSequenceFragment = EntityManager.GetFragmentDataChecked<FZombiTurboSequenceFragment>(EntityHandle);
    TurboSequenceFragment.TurboSequenceAsset = ZombiTurboSequenceAsset;
    TurboSequenceFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // 4 grupos de actualización
    TurboSequenceFragment.bIsVisualInstanceValid = false;

    // Crear instancia visual de TurboSequence
    if (ZombiTurboSequenceAsset)
    {
        // Crear datos de spawn
        FTurboSequence_MeshSpawnData_Lf SpawnData;
        SpawnData.RootMotionMesh.Mesh = ZombiTurboSequenceAsset;

        // Crear instancia
        TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
            SpawnData,
            SpawnTransform,
            GetWorld());

        if (TurboSequenceFragment.MeshData.IsMeshDataValid())
        {
            // Agregar al grupo de actualización
            ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
                TurboSequenceFragment.UpdateGroupIndex,
                TurboSequenceFragment.MeshData);

            // Configurar Blend Space para animaciones
            ConfigureBlendSpaceForEntity(TurboSequenceFragment);

            TurboSequenceFragment.bIsVisualInstanceValid = true;

            UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Instancia visual creada exitosamente para entidad %d en grupo %d"),
                   EntityHandle.Index, TurboSequenceFragment.UpdateGroupIndex);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: Error al crear instancia visual para entidad %d"), EntityHandle.Index);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: No hay asset de TurboSequence configurado"));
    }
}

void UZombiSpawnerSubsystem::ConfigureBlendSpaceForEntity(FZombiTurboSequenceFragment &TurboSequenceFragment)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: MeshData no válido"));
        return;
    }

    // Verificación estricta del asset antes de usarlo
    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: TurboSequenceAsset es null"));
        return;
    }

    if (!IsValid(TurboSequenceFragment.TurboSequenceAsset))
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: TurboSequenceAsset no es válido"));
        return;
    }

    // Verificación estricta de la animación por defecto
    if (!TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation)
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: OverrideDefaultAnimation es null"));
        return;
    }

    if (!IsValid(TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation))
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: OverrideDefaultAnimation no es válido"));
        return;
    }

    // Configuración de animación para Blend Space
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;
    PlaySettings.AnimationPlayTimeInSeconds = 0.0f;
    PlaySettings.StartTransitionTimeInSeconds = 0.1f;
    PlaySettings.EndTransitionTimeInSeconds = 0.1f;
    PlaySettings.RootMotionMode = ETurboSequence_RootMotionMode_Lf::None;
    PlaySettings.AnimationManagementMode = ETurboSequence_ManagementMode_Lf::Auto;

    // TEMPORAL: Comentar la reproducción de animaciones para evitar crash
    // Una vez que el sistema esté estable, implementaremos el Blend Space
    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animación configurada (reproducción temporalmente deshabilitada para estabilidad)"));

    /*
    // Código comentado temporalmente para evitar crashes
    try
    {
        FTurboSequence_AnimMinimalCollection_Lf AnimationCollection =
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                TurboSequenceFragment.MeshData,
                TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation,
                PlaySettings);

        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animación reproducida exitosamente"));
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: Error al reproducir animación"));
    }
    */
}
