// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTestActor.h"
#include "MyTurboSequenceAnimComponent.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiMassSubsystem.h"
#include "Engine/Engine.h"

// Constructor: crea el componente de animación
AZombiTestActor::AZombiTestActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crea el componente de animación TurboSequence
    AnimComponent = CreateDefaultSubobject<UMyTurboSequenceAnimComponent>(TEXT("AnimComponent"));

    // Inicializa variables
    StateTimer = 0.0f;
    CurrentStateIndex = 0;
    bRegisteredInMass = false;

    // Llena la lista de estados disponibles
    AllStates = {EZombiState::Idle, EZombiState::Walk, EZombiState::Chase,
                 EZombiState::Attack, EZombiState::Hit, EZombiState::Death};
}

// Se ejecuta cuando el actor aparece en el mundo
void AZombiTestActor::BeginPlay()
{
    Super::BeginPlay();

    // Inicializa el zombi con TurboSequence
    InitializeZombi();

    // Registra en el sistema Mass Entity
    RegisterInMassSystem();
}

// Se ejecuta cuando el actor se destruye
void AZombiTestActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Desregistra del sistema Mass si está registrado
    if (bRegisteredInMass)
    {
        if (UZombiMassSubsystem *MassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>())
        {
            MassSubsystem->UnregisterZombiEntity(this);
        }
        bRegisteredInMass = false;
    }

    Super::EndPlay(EndPlayReason);
}

// Se ejecuta cada frame
void AZombiTestActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Cambia estados automáticamente cada cierto tiempo
    StateTimer += DeltaTime;
    if (StateTimer >= StateChangeInterval)
    {
        ChangeToRandomState();
        StateTimer = 0.0f;
    }
}

// Inicializa el zombi con TurboSequence
void AZombiTestActor::InitializeZombi()
{
    if (!TSAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("TSAsset no asignado en ZombiTestActor"));
        return;
    }

    // Inicializa TurboSequence con el asset y la posición del actor
    AnimComponent->InitializeTurboSequence(TSAsset, GetActorTransform());

    // Establece el estado inicial
    AnimComponent->SetState(EZombiState::Idle);

    UE_LOG(LogTemp, Log, TEXT("Zombi inicializado con TurboSequence"));
}

// Cambia a un estado aleatorio
void AZombiTestActor::ChangeToRandomState()
{
    if (AllStates.Num() == 0)
        return;

    // Selecciona un estado aleatorio
    int32 RandomIndex = FMath::RandRange(0, AllStates.Num() - 1);
    EZombiState NewState = AllStates[RandomIndex];

    // Cambia el estado directamente en el componente
    AnimComponent->SetState(NewState);

    UE_LOG(LogTemp, Log, TEXT("Zombi cambió a estado: %d"), (int32)NewState);
}

// Registra este actor en el sistema Mass Entity
void AZombiTestActor::RegisterInMassSystem()
{
    if (bRegisteredInMass)
        return;

    UZombiMassSubsystem *MassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>();
    if (MassSubsystem)
    {
        MassSubsystem->RegisterZombiEntity(this);
        bRegisteredInMass = true;
        UE_LOG(LogTemp, Log, TEXT("Zombi registrado en Mass Entity System"));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("No se pudo obtener ZombiMassSubsystem"));
    }
}
