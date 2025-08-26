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
#include "EngineUtils.h"

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

// Crea la instancia visual de TurboSequence para una entidad siguiendo el patrón oficial
void UZombiSpawnerSubsystem::CreateTurboSequenceVisualInstance(FMassEntityHandle EntityHandle, const FVector &SpawnLocation)
{
    // PASO 1: Verificar que MassEntitySubsystem esté disponible
    if (!MassEntitySubsystem)
    {
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: MassEntitySubsystem no disponible, agregando a lista de reintentos"));
            PendingVisualInstances.Add(EntityHandle);
            return;
        }
    }

    // PASO 2: Verificar que TurboSequence Manager esté en el mundo
    ATurboSequence_Manager_Lf *TSManager = nullptr;

    // Buscar directamente en el mundo por si Instance no está inicializada
    for (TActorIterator<ATurboSequence_Manager_Lf> ActorItr(GetWorld()); ActorItr; ++ActorItr)
    {
        TSManager = *ActorItr;
        break;
    }

    if (!TSManager && !ATurboSequence_Manager_Lf::Instance)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ZombiSpawnerSubsystem: TurboSequence Manager no encontrado. Debe agregar ATurboSequence_Manager_Lf al nivel."));
        PendingVisualInstances.Add(EntityHandle);
        return;
    }

    // PASO 3: Obtener el fragmento de TurboSequence
    FZombiTurboSequenceFragment &TurboSequenceFragment = MassEntitySubsystem->GetEntityManager()
                                                             .GetFragmentDataChecked<FZombiTurboSequenceFragment>(EntityHandle);

    // PASO 4: Configurar el asset
    TurboSequenceFragment.TurboSequenceAsset = ZombiTurboSequenceAsset;

    // PASO 5: Crear transform de spawn
    FRotator SpawnRotation = GenerateRandomRotation();
    FTransform SpawnTransform(SpawnRotation, SpawnLocation, FVector::OneVector);

    // PASO 6: Verificaciones de validez
    if (!TurboSequenceFragment.TurboSequenceAsset || !IsValid(TurboSequenceFragment.TurboSequenceAsset) || !GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ZombiSpawnerSubsystem: Asset o World inválido para entidad %d"), EntityHandle.Index);
        return;
    }

    // PASO 7: PATRÓN OFICIAL - Configurar SpawnData correctamente
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;
    // NOTA: FootprintAsset no está disponible en esta versión de TurboSequence
    // Para híbrido mode se configuraría por separado si fuera necesario

    // PASO 8: PATRÓN OFICIAL - AddSkinnedMeshInstance_GameThread
    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    // PASO 9: Verificar que la instancia se creó correctamente
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ ZombiSpawnerSubsystem: Instancia visual no válida para entidad %d"), EntityHandle.Index);
        return;
    }

    // PASO 10: PATRÓN OFICIAL - AddInstanceToUpdateGroup_Concurrent
    // Distribución inteligente de grupos basada en posición para mejor LOD
    int32 UpdateGroupIndex = FMath::Abs(static_cast<int32>(SpawnLocation.X + SpawnLocation.Y)) % 4;
    ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
        UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // PASO 11: PATRÓN OFICIAL - PlayAnimation_Concurrent con animación inicial
    if (TurboSequenceFragment.TurboSequenceAsset &&
        TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary &&
        TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() > 0)
    {
        // Buscar animación Idle por defecto (patrón robusto)
        UAnimSequence *DefaultAnimation = nullptr;

        // Primero buscar Idle
        for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Idle"), ESearchCase::IgnoreCase))
            {
                DefaultAnimation = AnimItem.Animation;
                break;
            }
        }

        // Si no hay Idle, buscar Walk
        if (!DefaultAnimation)
        {
            for (const FAnimationLibraryItem_Lf &AnimItem : TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations)
            {
                if (AnimItem.Animation && AnimItem.Animation->GetName().Contains(TEXT("Walk"), ESearchCase::IgnoreCase))
                {
                    DefaultAnimation = AnimItem.Animation;
                    break;
                }
            }
        }

        // Como último recurso, usar la primera animación disponible
        if (!DefaultAnimation && TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation)
        {
            DefaultAnimation = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[0].Animation;
        }

        // PATRÓN OFICIAL - PlayAnimation_Concurrent
        if (DefaultAnimation)
        {
            TurboSequenceFragment.SetAnimation(DefaultAnimation);

            FTurboSequence_AnimPlaySettings_Lf PlaySettings;
            PlaySettings.AnimationSpeed = 1.0f;
            PlaySettings.AnimationWeight = 1.0f;
            // PlaySettings.bLooping = true; // Si existe esta opción

            // Según documentación: PlayAnimation_Concurrent devuelve FTurboSequence_AnimMinimalCollection_Lf
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                TurboSequenceFragment.MeshData,
                DefaultAnimation,
                PlaySettings);
        }
    }

    // PASO 12: Log de diagnóstico (solo primera entidad para evitar spam)
    static bool bLoggedFirstInstance = false;
    if (!bLoggedFirstInstance)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiSpawnerSubsystem: Primera instancia creada exitosamente - Grupo: %d, Animaciones disponibles: %d"),
               UpdateGroupIndex,
               TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary ? TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() : 0);
        bLoggedFirstInstance = true;
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
