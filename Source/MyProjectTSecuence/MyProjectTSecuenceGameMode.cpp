// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectTSecuenceGameMode.h"
#include "MyProjectTSecuenceCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "MassEntitySubsystem.h"
#include "ZombiSpawnerSubsystem.h"

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
}
