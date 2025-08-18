// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUpdateFrequencyFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiConfigFragment.h"
#include "Systems/Zombies/ECS/Processors/ZombiMovementProcessorOptimized.h"
// LEGACY: Procesadores viejos comentados para testing del nuevo enfoque
// #include "Systems/Zombies/ECS/Processors/ZombiBehaviorProcessor.h"
// #include "Systems/Zombies/ECS/Processors/ZombiBasicBehaviorProcessor.h"
#include "Systems/Zombies/ECS/Processors/ZombiUnifiedBehaviorProcessor.h"
#include "Systems/Zombies/ECS/Processors/ZombiTransformProcessor.h"

#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"

#include "Systems/Zombies/ECS/Processors/ZombiStimulusProcessor.h"
#include "Systems/Zombies/ECS/Processors/ZombiChaseProcessor.h"
// ZombiWalkAroundProcessor y ZombiIdleProcessor eliminados - migrados a ZombiBasicBehaviorProcessor
// ZombiUpdateProcessor eliminado - migrado a sistema especializado
#include "MassExecutionContext.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"

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

        // Asegurar Shared Fragments globales
        FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
        EntityManager.GetOrCreateSharedFragment<FZombiConfigFragment>(FZombiConfigFragment());
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

    // Crea los fragmentos optimizados con los datos iniciales
    // 1. Fragmento de Transformación (16 bytes)
    FZombiTransformFragment TransformFragment(SpawnLocation, GenerateRandomRotation());

    // 2. Fragmento de Movimiento (16 bytes)
    FZombiMovementFragment MovementFragment;
    MovementFragment.SetSpeed(static_cast<uint8>(FMath::RandRange(25, 80))); // Usar uint8
    MovementFragment.SetDirection(TransformFragment.GetForwardVector());
    MovementFragment.SetBatchGroup(FMath::RandRange(0, 3)); // Distribuir en grupos de batch

    // 3. Fragmento de Estado (16 bytes) - usando flags en lugar de enums
    FZombiStateFragment StateFragment;
    StateFragment.SetToIdle(); // Estado inicial Idle para transiciones automáticas
    StateFragment.SetHealthy(true);
    StateFragment.SetIndividual(true);

    // DEBUG: Log spawn con posición
    UE_LOG(LogTemp, Warning, TEXT("🧱 SPAWN: Zombie creado en Pos=%s | Estado=IDLE forzado"),
           *SpawnLocation.ToString());

    // IMPORTANTE: Asegurar que timer empiece en 0 para transiciones
    StateFragment.SetStateTimerSeconds(0.0f);

    // 4. Fragmento de TurboSequence (visual)
    FZombiTurboSequenceFragment TurboSequenceFragment;
    TurboSequenceFragment.TurboSequenceAsset = TurboSequenceAsset;
    TurboSequenceFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // Distribuye en 4 grupos

    // Crea la entidad con los fragmentos ya instanciados (método correcto de UE5.5)
    TArray<FInstancedStruct> FragmentList;

    // Instancia el fragmento de transformación
    FInstancedStruct TransformFragmentInstance;
    TransformFragmentInstance.InitializeAs<FZombiTransformFragment>();
    TransformFragmentInstance.GetMutable<FZombiTransformFragment>() = TransformFragment;
    FragmentList.Add(TransformFragmentInstance);

    // Instancia el fragmento de movimiento
    FInstancedStruct MovementFragmentInstance;
    MovementFragmentInstance.InitializeAs<FZombiMovementFragment>();
    MovementFragmentInstance.GetMutable<FZombiMovementFragment>() = MovementFragment;
    FragmentList.Add(MovementFragmentInstance);

    // Instancia el fragmento de estado
    FInstancedStruct StateFragmentInstance;
    StateFragmentInstance.InitializeAs<FZombiStateFragment>();
    StateFragmentInstance.GetMutable<FZombiStateFragment>() = StateFragment;
    FragmentList.Add(StateFragmentInstance);

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

    // Instancia el fragmento de estímulos (NUEVO - para sistema de estímulos)
    FInstancedStruct StimuliFragmentInstance;
    StimuliFragmentInstance.InitializeAs<FZombiStimuliFragment>();
    StimuliFragmentInstance.GetMutable<FZombiStimuliFragment>() = FZombiStimuliFragment();
    FragmentList.Add(StimuliFragmentInstance);

    // Crea la entidad
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Agregar tags necesarios para que los queries optimizados funcionen
    EntityManager.AddTagToEntity(EntityHandle, FActiveTag::StaticStruct());

    // IMPORTANTE: NO agregar FDeadTag - el query del MovementProcessor requiere EMassFragmentPresence::None para DeadTag
    // Esto significa que las entidades NO deben tener el DeadTag para ser procesadas

    // Guarda la referencia para limpieza
    RegisteredEntities.Add(EntityHandle);

    // Debug: Verificar que la entidad tiene los fragmentos correctos
    DebugEntityFragments(EntityHandle);

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
    // Solo loguear la primera entidad para confirmar que funciona
    static bool bFirstEntityLogged = false;
    if (!bFirstEntityLogged)
    {
        UE_LOG(LogTemp, Log, TEXT("🎮 Debug Entity %d - Primera entidad creada exitosamente"), EntityHandle.Index);
        bFirstEntityLogged = true;
    }
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
