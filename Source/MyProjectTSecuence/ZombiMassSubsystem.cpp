// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiMassSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "ZombiStateFragment.h"
#include "FMassActorFragment.h"
#include "MyTurboSequenceAnimComponent.h"

// Constructor del subsystem
UZombiMassSubsystem::UZombiMassSubsystem()
{
}

// Inicialización: obtiene referencia al Mass Entity Subsystem
void UZombiMassSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
    MassEntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>();
}

// Limpieza al destruir el subsystem
void UZombiMassSubsystem::Deinitialize()
{
    // Desregistra todas las entidades antes de destruir
    for (const auto &Pair : RegisteredEntities)
    {
        if (Pair.Key && MassEntitySubsystem)
        {
            FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
            EntityManager.DestroyEntity(Pair.Value);
        }
    }
    RegisteredEntities.Empty();

    Super::Deinitialize();
}

// Registra un actor zombi en el sistema Mass Entity
void UZombiMassSubsystem::RegisterZombiEntity(AActor *ZombiActor)
{
    if (!MassEntitySubsystem || !ZombiActor)
    {
        UE_LOG(LogTemp, Warning, TEXT("No se puede registrar entidad: MassEntitySubsystem o ZombiActor es null"));
        return;
    }

    // Verifica que el actor tenga el componente de animación
    UMyTurboSequenceAnimComponent *AnimComp = ZombiActor->FindComponentByClass<UMyTurboSequenceAnimComponent>();
    if (!AnimComp)
    {
        UE_LOG(LogTemp, Warning, TEXT("Actor %s no tiene componente MyTurboSequenceAnimComponent"), *ZombiActor->GetName());
        return;
    }

    // Crea la entidad Mass usando el método más simple de UE5.5
    FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();

    // Crea los fragmentos con los datos iniciales
    FZombiStateFragment StateFragment;
    StateFragment.State = AnimComp->CurrentState;

    FZombiActorFragment ActorFragment(ZombiActor);

    // Crea la entidad con los fragmentos ya instanciados (método correcto de UE5.5)
    TArray<FInstancedStruct> FragmentList;

    // Instancia el fragmento de estado
    FInstancedStruct StateFragmentInstance;
    StateFragmentInstance.InitializeAs<FZombiStateFragment>();
    StateFragmentInstance.GetMutable<FZombiStateFragment>() = StateFragment;
    FragmentList.Add(StateFragmentInstance);

    // Instancia el fragmento de actor
    FInstancedStruct ActorFragmentInstance;
    ActorFragmentInstance.InitializeAs<FZombiActorFragment>();
    ActorFragmentInstance.GetMutable<FZombiActorFragment>() = ActorFragment;
    FragmentList.Add(ActorFragmentInstance);

    // Crea la entidad con los fragmentos
    FMassEntityHandle EntityHandle = EntityManager.CreateEntity(FragmentList);

    // Guarda la referencia para poder desregistrar después
    RegisteredEntities.Add(ZombiActor, EntityHandle);

    UE_LOG(LogTemp, Log, TEXT("Entidad zombi registrada: %s"), *ZombiActor->GetName());
}

// Desregistra un actor zombi del sistema Mass Entity
void UZombiMassSubsystem::UnregisterZombiEntity(AActor *ZombiActor)
{
    if (!MassEntitySubsystem || !ZombiActor)
    {
        return;
    }

    // Busca la entidad en el mapa
    FMassEntityHandle *EntityHandle = RegisteredEntities.Find(ZombiActor);
    if (EntityHandle)
    {
        // Destruye la entidad
        FMassEntityManager &EntityManager = MassEntitySubsystem->GetMutableEntityManager();
        EntityManager.DestroyEntity(*EntityHandle);

        // Remueve del mapa
        RegisteredEntities.Remove(ZombiActor);

        UE_LOG(LogTemp, Log, TEXT("Entidad zombi desregistrada: %s"), *ZombiActor->GetName());
    }
}
