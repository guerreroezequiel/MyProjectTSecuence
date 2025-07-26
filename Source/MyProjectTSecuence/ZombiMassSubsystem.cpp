// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "ZombiStateFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementProcessor.h"
#include "ZombiTurboSequenceProcessor.h"
#include "ZombiUpdateProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiUpdateProcessor.h"
#include "ZombiMovementProcessor.h" // Para los tags FActiveTag, FMovingTag, FDeadTag

// Constructor del subsystem
UZombiMassSubsystem::UZombiMassSubsystem()
{
    // Configuración básica del subsystem
}

// Inicialización: obtiene referencia al Mass Entity Subsystem
void UZombiMassSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // Obtiene el MassEntitySubsystem
    MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();

    if (MassEntitySubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem: Sistema Mass Entity inicializado correctamente"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiMassSubsystem: No se pudo obtener MassEntitySubsystem"));
    }
}

// Se ejecuta cuando el mundo está listo
void UZombiMassSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Registra los procesadores de Mass Entity
    RegisterMassProcessors();
}

// Limpieza al destruir el subsystem
void UZombiMassSubsystem::Deinitialize()
{
    // Desregistra todas las entidades antes de destruir
    ClearAllEntities();

    Super::Deinitialize();
}

// Registra una entidad zombi en el sistema Mass Entity (nuevo método sin Actors)
FMassEntityHandle UZombiMassSubsystem::RegisterZombiEntity(const FVector &SpawnLocation,
                                                           UTurboSequence_MeshAsset_Lf *TurboSequenceAsset)
{
    // Intenta obtener el MassEntitySubsystem si no lo tenemos
    if (!MassEntitySubsystem)
    {
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        if (!MassEntitySubsystem)
        {
            UE_LOG(LogTemp, Warning, TEXT("No se puede registrar entidad: MassEntitySubsystem no disponible"));
            return FMassEntityHandle();
        }
    }

    // Crea la entidad Mass usando el método más simple de UE5.5
    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    // Crea los fragmentos con los datos iniciales
    FZombiStateFragment StateFragment;
    StateFragment.State = EZombiState::Idle; // Estado inicial

    // Log eliminado para optimización de rendimiento

    FZombiMovementFragment MovementFragment;
    MovementFragment.Position = SpawnLocation;
    MovementFragment.Rotation = GenerateRandomRotation();
    MovementFragment.MovementCenter = SpawnLocation;
    MovementFragment.MovementRadius = 500.0f;
    MovementFragment.MovementSpeed = FMath::RandRange(80.0f, 120.0f);
    MovementFragment.RotationSpeed = FMath::RandRange(60.0f, 120.0f);
    MovementFragment.DirectionChangeInterval = FMath::RandRange(2.0f, 5.0f);

    FZombiTurboSequenceFragment TurboSequenceFragment;
    TurboSequenceFragment.TurboSequenceAsset = TurboSequenceAsset;
    TurboSequenceFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // Distribuye en 4 grupos

    // Crea la entidad con los fragmentos ya instanciados (método correcto de UE5.5)
    TArray<FInstancedStruct> FragmentList;

    // Instancia el fragmento de estado
    FInstancedStruct StateFragmentInstance;
    StateFragmentInstance.InitializeAs<FZombiStateFragment>();
    StateFragmentInstance.GetMutable<FZombiStateFragment>() = StateFragment;
    FragmentList.Add(StateFragmentInstance);

    // Instancia el fragmento de movimiento
    FInstancedStruct MovementFragmentInstance;
    MovementFragmentInstance.InitializeAs<FZombiMovementFragment>();
    MovementFragmentInstance.GetMutable<FZombiMovementFragment>() = MovementFragment;
    FragmentList.Add(MovementFragmentInstance);

    // Instancia el fragmento de TurboSequence
    FInstancedStruct TurboSequenceFragmentInstance;
    TurboSequenceFragmentInstance.InitializeAs<FZombiTurboSequenceFragment>();
    TurboSequenceFragmentInstance.GetMutable<FZombiTurboSequenceFragment>() = TurboSequenceFragment;
    FragmentList.Add(TurboSequenceFragmentInstance);

    // Crea la entidad
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Agregar tags necesarios para que los queries optimizados funcionen
    EntityManager.AddTagToEntity(EntityHandle, FActiveTag::StaticStruct());
    EntityManager.AddTagToEntity(EntityHandle, FMovingTag::StaticStruct());

    // Guarda la referencia para limpieza
    RegisteredEntities.Add(EntityHandle);

    // UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem: Entidad creada exitosamente - Handle: %d, Total entidades: %d"),
    //        EntityHandle.Index, RegisteredEntities.Num());

    return EntityHandle;
}

// Desregistra una entidad zombi
void UZombiMassSubsystem::UnregisterZombiEntity(FMassEntityHandle EntityHandle)
{
    if (!EntityHandle.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("No se puede desregistrar entidad: Handle inválido"));
        return;
    }

    if (!MassEntitySubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("No se puede desregistrar entidad: MassEntitySubsystem no disponible"));
        return;
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
    EntityManager.DestroyEntity(EntityHandle);

    // Remueve de la lista de entidades registradas
    RegisteredEntities.Remove(EntityHandle);

    UE_LOG(LogTemp, Log, TEXT("Entidad zombi desregistrada del Mass Entity System - Handle: %d"), EntityHandle.Index);
}

// Limpia todas las entidades registradas
void UZombiMassSubsystem::ClearAllEntities()
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    for (const FMassEntityHandle &EntityHandle : RegisteredEntities)
    {
        if (EntityHandle.IsValid())
        {
            EntityManager.DestroyEntity(EntityHandle);
        }
    }

    RegisteredEntities.Empty();

    UE_LOG(LogTemp, Log, TEXT("Todas las entidades zombi eliminadas del Mass Entity System"));
}

// Genera una rotación aleatoria
FRotator UZombiMassSubsystem::GenerateRandomRotation() const
{
    return FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
}

// Registra los procesadores de Mass Entity
void UZombiMassSubsystem::RegisterMassProcessors()
{
    // Evita ejecutar esto múltiples veces
    if (bProcessorsRegistered)
    {
        return;
    }

    if (!MassEntitySubsystem)
    {
        MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
    }

    if (MassEntitySubsystem)
    {
        // En UE5.5.4, los procesadores Mass Entity se registran automáticamente cuando están en el proyecto
        // Solo necesitamos verificar que el sistema Mass Entity esté funcionando
        UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem: Sistema Mass Entity inicializado correctamente"));

        // Los procesadores se registran automáticamente en UE5.5.4

        // Los procesadores se registran automáticamente en UE5.5.4
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: Procesadores configurados para registro automático"));
        bProcessorsRegistered = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiMassSubsystem: No se pudo obtener MassEntitySubsystem"));
    }
}

// Ejecuta los procesadores manualmente cada frame (DEPRECATED - Ahora se ejecutan automáticamente)
void UZombiMassSubsystem::ExecuteProcessorsManually(float DeltaTime)
{
    // Los procesadores ahora se ejecutan automáticamente por el sistema Mass Entity
    // Este método se mantiene por compatibilidad pero no hace nada
    // TODO: Eliminar este método cuando se confirme que todo funciona correctamente
}
