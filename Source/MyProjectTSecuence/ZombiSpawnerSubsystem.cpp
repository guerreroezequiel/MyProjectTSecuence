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

    // Crear transform de spawn
    FTransform SpawnTransform(GenerateRandomRotation(), SpawnLocation, FVector::OneVector);

    // Crear instancia visual de TurboSequence
    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Intentando crear instancia visual para entidad %d"), EntityHandle.Index);

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

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Asset válido, creando instancia con transform: %s"), *SpawnTransform.ToString());

    // Crear la instancia visual
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: SpawnData.RootMotionMesh.Mesh válido: %s"),
           SpawnData.RootMotionMesh.Mesh ? TEXT("SÍ") : TEXT("NO"));

    if (SpawnData.RootMotionMesh.Mesh)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Asset Name: %s"), *SpawnData.RootMotionMesh.Mesh->GetName());
    }

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: World válido: %s"),
           GetWorld() ? TEXT("SÍ") : TEXT("NO"));

    if (GetWorld())
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: World Name: %s"), *GetWorld()->GetName());
    }

    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Instancia creada, MeshData válida: %s"),
           TurboSequenceFragment.MeshData.IsMeshDataValid() ? TEXT("SÍ") : TEXT("NO"));

    if (TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        // TODO: Implementar obtención de MeshID cuando esté disponible en la API
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Instancia válida creada"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: Instancia visual no válida para entidad %d"), EntityHandle.Index);
        return; // No continuar si la instancia no es válida
    }

    // Agregar al grupo de actualización
    ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
        TurboSequenceFragment.UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // Configurar Blend Space para animaciones
    ConfigureBlendSpaceForEntity(TurboSequenceFragment);

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Instancia visual creada exitosamente para entidad %d en grupo %d"),
           EntityHandle.Index, TurboSequenceFragment.UpdateGroupIndex);
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

    // DIAGNÓSTICO: Verificar qué tiene el asset
    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: === DIAGNÓSTICO DEL ASSET ==="));
    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Asset Name: %s"), *TurboSequenceFragment.TurboSequenceAsset->GetName());

    // Verificar OverrideDefaultAnimation
    if (TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: OverrideDefaultAnimation: %s"),
               *TurboSequenceFragment.TurboSequenceAsset->OverrideDefaultAnimation->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: OverrideDefaultAnimation es null"));
    }

    // Verificar AnimationLibrary
    if (TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: AnimationLibrary encontrada: %s"),
               *TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->GetName());

        // Verificar si tiene animaciones
        if (TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animaciones en librería: %d"),
                   TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num());

            // Listar las primeras 5 animaciones
            for (int32 i = 0; i < FMath::Min(5, TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations.Num()); ++i)
            {
                const FAnimationLibraryItem_Lf &AnimItem = TurboSequenceFragment.TurboSequenceAsset->AnimationLibrary->Animations[i];
                if (AnimItem.Animation)
                {
                    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animación %d: %s"),
                           i, *AnimItem.Animation->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: Animación %d: NULL"), i);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: AnimationLibrary no tiene animaciones"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: AnimationLibrary es null"));
    }

    // Verificar Skeleton
    TObjectPtr<USkeleton> Skeleton = TurboSequenceFragment.TurboSequenceAsset->GetSkeleton();
    if (Skeleton)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Skeleton: %s"),
               *Skeleton->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ZombiSpawnerSubsystem: Skeleton es null"));
    }

    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: === FIN DIAGNÓSTICO ==="));

    // TEMPORAL: Comentar completamente animaciones para compilación estable
    // TODO: Implementar Blend Space en lugar de animaciones individuales
    UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem: Animaciones comentadas - preparando para Blend Space"));

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
