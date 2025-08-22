// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
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

    // Configurar el asset
    TurboSequenceFragment.TurboSequenceAsset = ZombiTurboSequenceAsset;

    // Crear transform de spawn
    FRotator SpawnRotation = GenerateRandomRotation();
    FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);

    // Verificaciones optimizadas
    if (!TurboSequenceFragment.TurboSequenceAsset || !IsValid(TurboSequenceFragment.TurboSequenceAsset) || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiSpawnerSubsystem: Asset o World inválido para entidad %d"), EntityHandle.Index);
        return;
    }

    // Crear la instancia visual optimizada
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;

    // OPTIMIZACIÓN: Configuración básica sin sombras personalizadas
    // Las sombras se manejarán dinámicamente en el procesador

    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    // Verificar instancia creada
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ZombiSpawnerSubsystem: Instancia visual no válida para entidad %d"), EntityHandle.Index);
        return;
    }

    // Asignar a grupo de update (distribución automática)
    int32 UpdateGroupIndex = FMath::RandRange(0, 3);
    ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
        UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // NUEVA IMPLEMENTACIÓN: Inicializar animación por defecto siguiendo patrón del plugin
    if (TurboSequenceFragment.TurboSequenceAsset &&
        TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary &&
        TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() > 0)
    {
        // Buscar animación Idle por defecto
        UAnimSequence *DefaultAnimation = nullptr;
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
            {
                DefaultAnimation = AnimItem.Animation;
                break;
            }
        }

        // Si no hay Idle, usar la primera animación disponible
        if (!DefaultAnimation && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation)
        {
            DefaultAnimation = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation;
        }

        // Reproducir animación inicial (patrón correcto del plugin)
        if (DefaultAnimation)
        {
            TurboSequenceFragment.SetAnimation(DefaultAnimation);

            FTurboSequence_AnimPlaySettings_Lf PlaySettings;
            PlaySettings.AnimationSpeed = 1.0f;
            PlaySettings.AnimationWeight = 1.0f;

            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                TurboSequenceFragment.MeshData,
                DefaultAnimation,
                PlaySettings);
        }
    }

    // Log de diagnóstico (solo primera entidad)
    static bool bLoggedDiagnostic = false;
    if (!bLoggedDiagnostic)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiSpawnerSubsystem: Instancia creada exitosamente - Grupo: %d"), UpdateGroupIndex);
        bLoggedDiagnostic = true;
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
            FZombiCoreFragment &CoreFragment = MassEntitySubsystem->GetEntityManager()
                                                   .GetFragmentDataChecked<FZombiCoreFragment>(EntityHandle);

            CreateTurboSequenceVisualInstance(EntityHandle, CoreFragment.Position);
        }
        else
        {
            StillPending.Add(EntityHandle);
        }
    }

    // Actualizar lista de pendientes
    PendingVisualInstances = StillPending;
}

// Función eliminada - configuración de animaciones manejada por el procesador optimizado
