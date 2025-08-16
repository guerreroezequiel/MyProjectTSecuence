// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectTSecuenceGameMode.h"
#include "MyProjectTSecuenceCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "MassEntitySubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Systems/StimulusSubsystem/PlayerSignalSubsystem.h"

AMyProjectTSecuenceGameMode::AMyProjectTSecuenceGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AMyProjectTSecuenceGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Obtiene el ZombiMassSubsystem
	ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>();

	if (ZombiMassSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem inicializado correctamente"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener ZombiMassSubsystem"));
	}

	// Obtiene el ZombiSpawnerSubsystem
	ZombiSpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

	if (ZombiSpawnerSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem inicializado correctamente"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener ZombiSpawnerSubsystem"));
	}

	// Obtiene el StimulusSubsystem
	StimulusSubsystem = GetWorld()->GetSubsystem<UStimulusSubsystem>();

	if (StimulusSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("StimulusSubsystem inicializado correctamente"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener StimulusSubsystem"));
	}

	// Obtiene el PlayerSignalSubsystem
	PlayerSignalSubsystem = GetWorld()->GetSubsystem<UPlayerSignalSubsystem>();

	if (PlayerSignalSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("PlayerSignalSubsystem inicializado correctamente"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener PlayerSignalSubsystem"));
	}
}
