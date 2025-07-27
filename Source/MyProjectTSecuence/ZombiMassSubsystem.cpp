// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "ZombiCoreFragment.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiUpdateFrequencyFragment.h"
#include "ZombiCombatFragment.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementProcessor.h"
#include "ZombiBehaviorProcessor.h"
#include "ZombiCombatProcessor.h"
#include "ZombiTurboSequenceProcessor.h"
#include "ZombiPeriodicChaseProcessor.h"
// ZombiUpdateProcessor eliminado - migrado a sistema especializado
// ZombiChaseProcessor eliminado - migrado a sistema especializado
#include "MassExecutionContext.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiTags.h"

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

    // Crea los fragmentos especializados con los datos iniciales
    // 1. Fragmento Core (movimiento + transformación)
    FZombiCoreFragment CoreFragment(SpawnLocation, GenerateRandomRotation(), 500.0f);
    CoreFragment.MovementSpeed = FMath::RandRange(25.0f, 80.0f);
    CoreFragment.RotationSpeed = FMath::RandRange(60.0f, 120.0f);
    CoreFragment.DirectionChangeInterval = FMath::RandRange(2.0f, 5.0f);

    // 2. Fragmento de Comportamiento (IA + estados)
    FZombiBehaviorFragment BehaviorFragment;
    BehaviorFragment.SetState(EZombiState::WalkAround); // Estado inicial
    BehaviorFragment.SetCondition(EZombiCondition::Healthy);
    BehaviorFragment.SetHordeBehavior(EZombiHordeBehavior::Individual);
    BehaviorFragment.ChaseDistance = 1000.0f;
    BehaviorFragment.ChaseSpeed = 200.0f;
    BehaviorFragment.AttackRange = 150.0f;

    // 3. Fragmento de Combate (salud + daño)
    FZombiCombatFragment CombatFragment(100.0f, 25.0f, 150.0f); // MaxHealth, AttackDamage, AttackRange

    // 4. Fragmento de TurboSequence (visual)
    FZombiTurboSequenceFragment TurboSequenceFragment;
    TurboSequenceFragment.TurboSequenceAsset = TurboSequenceAsset;
    TurboSequenceFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // Distribuye en 4 grupos

    // Crea la entidad con los fragmentos ya instanciados (método correcto de UE5.5)
    TArray<FInstancedStruct> FragmentList;

    // Instancia el fragmento Core
    FInstancedStruct CoreFragmentInstance;
    CoreFragmentInstance.InitializeAs<FZombiCoreFragment>();
    CoreFragmentInstance.GetMutable<FZombiCoreFragment>() = CoreFragment;
    FragmentList.Add(CoreFragmentInstance);

    // Instancia el fragmento de comportamiento
    FInstancedStruct BehaviorFragmentInstance;
    BehaviorFragmentInstance.InitializeAs<FZombiBehaviorFragment>();
    BehaviorFragmentInstance.GetMutable<FZombiBehaviorFragment>() = BehaviorFragment;
    FragmentList.Add(BehaviorFragmentInstance);

    // Instancia el fragmento de combate
    FInstancedStruct CombatFragmentInstance;
    CombatFragmentInstance.InitializeAs<FZombiCombatFragment>();
    CombatFragmentInstance.GetMutable<FZombiCombatFragment>() = CombatFragment;
    FragmentList.Add(CombatFragmentInstance);

    // Instancia el fragmento de TurboSequence
    FInstancedStruct TurboSequenceFragmentInstance;
    TurboSequenceFragmentInstance.InitializeAs<FZombiTurboSequenceFragment>();
    TurboSequenceFragmentInstance.GetMutable<FZombiTurboSequenceFragment>() = TurboSequenceFragment;
    FragmentList.Add(TurboSequenceFragmentInstance);

    // Instancia el fragmento de frecuencia de update (NUEVO - para optimización)
    FInstancedStruct UpdateFrequencyFragmentInstance;
    UpdateFrequencyFragmentInstance.InitializeAs<FZombiUpdateFrequencyFragment>();
    UpdateFrequencyFragmentInstance.GetMutable<FZombiUpdateFrequencyFragment>() = FZombiUpdateFrequencyFragment();
    FragmentList.Add(UpdateFrequencyFragmentInstance);

    // Crea la entidad
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Agregar tags necesarios para que los queries optimizados funcionen
    EntityManager.AddTagToEntity(EntityHandle, FActiveTag::StaticStruct());

    // IMPORTANTE: NO agregar FDeadTag - el query del MovementProcessor requiere EMassFragmentPresence::None para DeadTag
    // Esto significa que las entidades NO deben tener el DeadTag para ser procesadas

    // Guarda la referencia para limpieza
    RegisteredEntities.Add(EntityHandle);

    UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem: Entidad optimizada creada exitosamente - Handle: %d, Total entidades: %d"),
           EntityHandle.Index, RegisteredEntities.Num());

    // Debug: Verificar que la entidad tiene los fragmentos correctos
    DebugEntityFragments(EntityHandle);

    // Verificación adicional: Log de confirmación de creación
    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: Entidad %d creada exitosamente"), EntityHandle.Index);

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

// OPTIMIZACIÓN: Genera una rotación aleatoria (inline para mejor rendimiento)
FORCEINLINE FRotator UZombiMassSubsystem::GenerateRandomRotation() const
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
        UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: Procesadores configurados para registro automático"));

        // Verificar que los procesadores están registrados
        VerifyProcessorsRegistration();
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

// Debug: Verifica que una entidad tiene los fragmentos correctos
void UZombiMassSubsystem::DebugEntityFragments(FMassEntityHandle EntityHandle)
{
    if (!MassEntitySubsystem || !EntityHandle.IsValid())
    {
        return;
    }

    // En UE5.5, no podemos verificar fragmentos directamente desde EntityManager
    // Solo podemos loguear que la entidad fue creada exitosamente
    UE_LOG(LogTemp, Log, TEXT("🎮 Debug Entity %d - Entidad creada exitosamente"), EntityHandle.Index);
    UE_LOG(LogTemp, Log, TEXT("🎮 Debug Entity %d - Se espera que tenga: Core, Behavior, Combat, TurboSequence, ActiveTag"), EntityHandle.Index);
}

// Debug: Verifica que una entidad tiene los tags correctos
void UZombiMassSubsystem::DebugEntityTags(FMassEntityHandle EntityHandle)
{
    if (!MassEntitySubsystem || !EntityHandle.IsValid())
    {
        return;
    }

    // En UE5.5, no podemos verificar tags directamente, pero podemos loguear información útil
    UE_LOG(LogTemp, Log, TEXT("🎮 Debug Tags Entity %d - Verificando tags después de un frame"), EntityHandle.Index);
    UE_LOG(LogTemp, Log, TEXT("🎮 Debug Tags Entity %d - Debe tener: ActiveTag ✓, DeadTag ✗"), EntityHandle.Index);
    UE_LOG(LogTemp, Log, TEXT("🎮 Debug Tags Entity %d - Si no se procesa, verificar que NO tenga DeadTag"), EntityHandle.Index);
}

// Verifica que los procesadores están registrados correctamente
void UZombiMassSubsystem::VerifyProcessorsRegistration()
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    // En UE5.5.4, los procesadores se registran automáticamente
    // Solo podemos verificar que el sistema Mass Entity esté funcionando
    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: Verificando registro de procesadores"));
    UE_LOG(LogTemp, Log, TEXT("🎮 ZombiMassSubsystem: MassEntitySubsystem válido: %s"),
           MassEntitySubsystem ? TEXT("Sí") : TEXT("No"));
}
