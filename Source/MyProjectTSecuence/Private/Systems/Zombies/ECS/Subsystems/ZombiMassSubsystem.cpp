// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiLODFragment.h"
// Procesadores individuales migrados al coordinador unificado
#include "MassExecutionContext.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"

// Constructor del subsystem
UZombiMassSubsystem::UZombiMassSubsystem()
{
    // Configuración básica del subsystem
}

// Inicialización: configuración básica
void UZombiMassSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    // NOTA: MassEntitySubsystem se inicializa en OnWorldBeginPlay cuando el mundo está listo
    UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem: Subsystem inicializado, esperando mundo listo..."));
}

// Se ejecuta cuando el mundo está listo
void UZombiMassSubsystem::OnWorldBeginPlay(UWorld &InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Inicializa MassEntitySubsystem cuando el mundo está listo
    MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();

    if (MassEntitySubsystem)
    {
        UE_LOG(LogTemp, Log, TEXT("✅ ZombiMassSubsystem: MassEntitySubsystem inicializado correctamente"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("❌ ZombiMassSubsystem: No se pudo obtener MassEntitySubsystem"));
    }

    // Inicializa el coordinador principal
    InitializeSystemCoordinator();
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
    FZombiCoreFragment CoreFragment(SpawnLocation, GenerateRandomRotation(), 500.0f);
    CoreFragment.MovementSpeed = FMath::RandRange(25.0f, 80.0f);
    CoreFragment.RotationSpeed = FMath::RandRange(60.0f, 120.0f);
    CoreFragment.DirectionChangeInterval = FMath::RandRange(2.0f, 5.0f);

    FZombiBehaviorFragment BehaviorFragment;
    BehaviorFragment.SetState(EZombiState::Idle);
    // Nota: Los métodos SetCondition y SetHordeBehavior fueron removidos en la simplificación
    // Solo mantenemos el estado básico para el enfoque híbrido

    FZombiTurboSequenceFragment TurboSequenceFragment;
    TurboSequenceFragment.TurboSequenceAsset = TurboSequenceAsset;

    // Crea la entidad con los fragmentos
    TArray<FInstancedStruct> FragmentList;

    FInstancedStruct CoreFragmentInstance;
    CoreFragmentInstance.InitializeAs<FZombiCoreFragment>();
    CoreFragmentInstance.GetMutable<FZombiCoreFragment>() = CoreFragment;
    FragmentList.Add(CoreFragmentInstance);

    FInstancedStruct BehaviorFragmentInstance;
    BehaviorFragmentInstance.InitializeAs<FZombiBehaviorFragment>();
    BehaviorFragmentInstance.GetMutable<FZombiBehaviorFragment>() = BehaviorFragment;
    FragmentList.Add(BehaviorFragmentInstance);

    FInstancedStruct TurboSequenceFragmentInstance;
    TurboSequenceFragmentInstance.InitializeAs<FZombiTurboSequenceFragment>();
    TurboSequenceFragmentInstance.GetMutable<FZombiTurboSequenceFragment>() = TurboSequenceFragment;
    FragmentList.Add(TurboSequenceFragmentInstance);

    // Instancia el fragmento de LOD (nuevo para optimización híbrida)
    FInstancedStruct LODFragmentInstance;
    LODFragmentInstance.InitializeAs<FZombiLODFragment>();
    LODFragmentInstance.GetMutable<FZombiLODFragment>() = FZombiLODFragment();
    FragmentList.Add(LODFragmentInstance);

    // Crea la entidad
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Agregar tags necesarios para que los queries optimizados funcionen
    EntityManager.AddTagToEntity(EntityHandle, FActiveTag::StaticStruct());

    // IMPORTANTE: NO agregar FDeadTag - el query del MovementProcessor requiere EMassFragmentPresence::None para DeadTag
    // Esto significa que las entidades NO deben tener el DeadTag para ser procesadas

    // Guarda la referencia para limpieza
    RegisteredEntities.Add(EntityHandle);

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
}

// OPTIMIZACIÓN: Genera una rotación aleatoria (inline para mejor rendimiento)
FORCEINLINE FRotator UZombiMassSubsystem::GenerateRandomRotation() const
{
    return FRotator(0.0f, FMath::RandRange(0.0f, 360.0f), 0.0f);
}

// Inicializa el coordinador principal
void UZombiMassSubsystem::InitializeSystemCoordinator()
{
    // Evita ejecutar esto múltiples veces
    if (bCoordinatorInitialized)
    {
        return;
    }

    // Obtener coordinador del Game Instance
    if (UGameInstance *GameInstance = GetWorld()->GetGameInstance())
    {
        SystemCoordinator = GameInstance->GetSubsystem<UZombiSystemCoordinator>();
        if (SystemCoordinator)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ ZombiMassSubsystem: Coordinador encontrado y iniciado"));
            SystemCoordinator->StartSystem();
            bCoordinatorInitialized = true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiMassSubsystem: Coordinador no disponible"));
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("ZombiMassSubsystem: No se pudo obtener MassEntitySubsystem"));
    }
}

// Verifica que el coordinador está inicializado correctamente
void UZombiMassSubsystem::VerifyCoordinatorInitialization()
{
    if (SystemCoordinator && SystemCoordinator->IsSystemActive())
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: Coordinador funcionando correctamente"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ ZombiMassSubsystem: Coordinador no activo"));
    }
}
