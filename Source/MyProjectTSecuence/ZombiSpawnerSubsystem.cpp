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

    // Log eliminado para optimización de rendimiento

    // Spawna los zombis en lotes optimizados para máximo rendimiento
    const int32 BatchSize = 1000; // Aumentado a 1000 para mejor rendimiento
    int32 RemainingCount = Count;
    int32 SuccessfullySpawned = 0;

    // Pre-allocar memoria para evitar reallocaciones
    SpawnedEntities.Reserve(SpawnedEntities.Num() + Count);

    while (RemainingCount > 0)
    {
        int32 CurrentBatchSize = FMath::Min(BatchSize, RemainingCount);

        // Crear entidades en lote para mejor rendimiento
        TArray<FMassEntityHandle> BatchEntities;
        BatchEntities.Reserve(CurrentBatchSize);

        for (int32 i = 0; i < CurrentBatchSize; ++i)
        {
            FVector SpawnLocation = GenerateRandomSpawnLocation(CenterLocation, SpawnRadius);
            FMassEntityHandle EntityHandle = CreateZombiMassEntity(SpawnLocation);

            if (EntityHandle.IsValid())
            {
                BatchEntities.Add(EntityHandle);
                SuccessfullySpawned++;
            }
        }

        // Agregar lote completo de una vez
        SpawnedEntities.Append(BatchEntities);
        ActiveZombiCount += BatchEntities.Num();

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
    // Verificar que MassEntitySubsystem esté disponible
    if (!MassEntitySubsystem)
    {
        // Reintentar obtener el subsystem
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: MassEntitySubsystem no disponible, agregando a lista de reintentos"));
            PendingVisualInstances.Add(EntityHandle);
            return;
        }
    }

    // Obtener el fragmento de TurboSequence
    FZombiTurboSequenceFragment &TurboSequenceFragment = MassEntitySubsystem->GetEntityManager()
                                                             .GetFragmentDataChecked<FZombiTurboSequenceFragment>(EntityHandle);

    // Configurar el asset y grupo de actualización
    TurboSequenceFragment.TurboSequenceAsset = ZombiTurboSequenceAsset;
    TurboSequenceFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // 4 grupos de actualización

    // Crear transform de spawn sin offset - el asset ya está orientado correctamente
    FRotator SpawnRotation = GenerateRandomRotation();
    FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);

    // Crear instancia visual de TurboSequence
    // Log solo para las primeras 3 entidades
    static int32 LoggedEntities = 0;
    if (LoggedEntities < 3)
    {
        // Log eliminado para optimización de rendimiento
        LoggedEntities++;
    }

    // Verificar que el asset sea válido antes de crear la instancia
    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: TurboSequenceAsset es null para entidad %d"), EntityHandle.Index);
        return;
    }

    if (!IsValid(TurboSequenceFragment.TurboSequenceAsset))
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: TurboSequenceAsset no es válido para entidad %d"), EntityHandle.Index);
        return;
    }

    // Verificar que el World esté disponible
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: World no disponible"));
        return;
    }

    // Log solo para las primeras 3 entidades
    static int32 LoggedTransforms = 0;
    if (LoggedTransforms < 3)
    {
        // Log eliminado para optimización de rendimiento
        LoggedTransforms++;
    }

    // Crear la instancia visual
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;

    // Logs de verificación solo para la primera entidad
    static bool bLoggedVerification = false;
    if (!bLoggedVerification)
    {
        // Log eliminado para optimización de rendimiento
        bLoggedVerification = true;
    }

    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    // Verificar instancia creada
    if (TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        // Log solo para las primeras 3 entidades
        static int32 LoggedInstances = 0;
        if (LoggedInstances < 3)
        {
            // Log eliminado para optimización de rendimiento
            LoggedInstances++;
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ZombiSpawnerSubsystem: Instancia visual no válida para entidad %d"), EntityHandle.Index);
        return; // No continuar si la instancia no es válida
    }

    // Agregar al grupo de actualización
    ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
        TurboSequenceFragment.UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // Configurar Blend Space para animaciones
    ConfigureBlendSpaceForEntity(TurboSequenceFragment);

    // Log final solo para las primeras 3 entidades
    static int32 LoggedFinal = 0;
    if (LoggedFinal < 3)
    {
        // Log eliminado para optimización de rendimiento
        LoggedFinal++;
    }
}

// Procesa reintentos de instancias visuales pendientes
void UZombiSpawnerSubsystem::ProcessPendingVisualInstances(float DeltaTime)
{
    if (PendingVisualInstances.Num() == 0)
    {
        return;
    }

    RetryTimer += DeltaTime;
    if (RetryTimer < RetryInterval)
    {
        return;
    }

    RetryTimer = 0.0f;

    // Verificar si MassEntitySubsystem está disponible ahora
    if (!MassEntitySubsystem)
    {
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: MassEntitySubsystem aún no disponible, %d instancias pendientes"),
                   PendingVisualInstances.Num());
            return;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Procesando %d instancias visuales pendientes"),
           PendingVisualInstances.Num());

    // Procesar instancias pendientes
    TArray<FMassEntityHandle> StillPending;

    for (FMassEntityHandle EntityHandle : PendingVisualInstances)
    {
        if (EntityHandle.IsValid())
        {
            // Obtener la posición de la entidad para recrear la instancia visual
            FZombiMovementFragment &MovementFragment = MassEntitySubsystem->GetEntityManager()
                                                           .GetFragmentDataChecked<FZombiMovementFragment>(EntityHandle);

            CreateTurboSequenceVisualInstance(EntityHandle, MovementFragment.Position);
        }
        else
        {
            StillPending.Add(EntityHandle);
        }
    }

    // Actualizar lista de pendientes
    PendingVisualInstances = StillPending;
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

    // Diagnóstico del asset solo para la primera entidad
    static bool bLoggedAssetDiagnostic = false;
    if (!bLoggedAssetDiagnostic)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiSpawnerSubsystem: Asset configurado - %d animaciones disponibles"),
               TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary ? TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() : 0);
        bLoggedAssetDiagnostic = true;
    }

    /*
    // Configurar animación por defecto
    if (TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animación por defecto disponible: %s (reproducción temporalmente deshabilitada)"),
               *TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation->GetName());

        // TEMPORAL: Comentar reproducción de animaciones para evitar crash
        // Configurar animación por defecto - usar configuración mínima
        // FTurboSequence_AnimPlaySettings_Lf AnimSettings;
        // Los campos específicos se configurarán según la estructura real

        // Reproducir animación por defecto
        // ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
        //     TurboSequenceFragment.MeshData,
        //     TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation,
        //     AnimSettings
        // );
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: No hay animación por defecto configurada"));
    }
    */
}
