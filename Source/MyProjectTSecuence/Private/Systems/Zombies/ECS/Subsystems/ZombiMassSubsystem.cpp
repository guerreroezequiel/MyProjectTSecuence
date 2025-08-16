// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUltraConsolidatedFragment.h"
#include "Systems/Zombies/ECS/Processors/ZombiUltraConsolidatedProcessor.h"
#include "Systems/Zombies/ECS/Processors/ZombiTurboSequenceProcessor.h"
// ZombiUpdateProcessor eliminado - migrado a sistema especializado
// ZombiChaseProcessor eliminado - migrado a sistema especializado
#include "MassExecutionContext.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
// Helpers eliminados - lógica movida al procesador ultra-consolidado

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

// Registra una entidad zombi en el sistema Mass Entity usando fragmento ultra-consolidado
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

    // Crea el fragmento ultra-consolidado con todos los datos necesarios
    FZombiUltraConsolidatedFragment UltraFragment;
    
    // Datos de transformación
    UltraFragment.Position = SpawnLocation;
    UltraFragment.Rotation = GenerateRandomRotation();
    UltraFragment.MovementSpeed = FMath::RandRange(80.0f, 150.0f);
    
    // Datos de estado
    UltraFragment.State = ZombiStates::WALK; // Estado inicial
    UltraFragment.Health = 100;
    UltraFragment.Flags = 0;
    UltraFragment.UpdatePriority = ZombiPriorities::NORMAL;
    
    // Datos de movimiento
    UltraFragment.MovementDirection = FVector::ForwardVector;
    UltraFragment.BehaviorTimer = 0.0f;
    UltraFragment.DirectionChangeInterval = FMath::RandRange(1.0f, 3.0f);
    
    // Datos de combate
    UltraFragment.AttackCooldown = 0.0f;
    UltraFragment.DamageCooldown = 0.0f;
    UltraFragment.LastDamageTime = 0.0f;
    
    // Datos de optimización
    UltraFragment.DistanceToPlayer = 0.0f;
    UltraFragment.LastUpdateTime = 0.0f;
    UltraFragment.UpdateInterval = 1.0f / 60.0f;
    
    // Datos visuales
    UltraFragment.UpdateGroupIndex = FMath::RandRange(0, 3); // Distribuye en 4 grupos

    // Crea la entidad con el fragmento ultra-consolidado
    TArray<FInstancedStruct> FragmentList;

    // Instancia el fragmento ultra-consolidado
    FInstancedStruct UltraFragmentInstance;
    UltraFragmentInstance.InitializeAs<FZombiUltraConsolidatedFragment>();
    UltraFragmentInstance.GetMutable<FZombiUltraConsolidatedFragment>() = UltraFragment;
    FragmentList.Add(UltraFragmentInstance);

    // Crea la entidad
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Agregar tags necesarios para que los queries optimizados funcionen
    EntityManager.AddTagToEntity(EntityHandle, FActiveTag::StaticStruct());

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

    // Log de desregistro eliminado para optimización de rendimiento
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

    // Logs de debug eliminados para optimización de rendimiento
}

// Debug: Verifica que una entidad tiene los tags correctos
void UZombiMassSubsystem::DebugEntityTags(FMassEntityHandle EntityHandle)
{
    if (!MassEntitySubsystem || !EntityHandle.IsValid())
    {
        return;
    }

    // Logs de debug eliminados para optimización de rendimiento
}

// Verifica que los procesadores están registrados correctamente
void UZombiMassSubsystem::VerifyProcessorsRegistration()
{
    if (!MassEntitySubsystem)
    {
        return;
    }

    // Logs de verificación eliminados para optimización de rendimiento
}
