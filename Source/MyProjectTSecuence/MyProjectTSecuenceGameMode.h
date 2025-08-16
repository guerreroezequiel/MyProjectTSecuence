// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiMassSubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Systems/StimulusSubsystem/PlayerSignalSubsystem.h"
#include "MyProjectTSecuenceGameMode.generated.h"

UCLASS(minimalapi)
class AMyProjectTSecuenceGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMyProjectTSecuenceGameMode();

protected:
	// Referencia al ZombiMassSubsystem
	UPROPERTY()
	UZombiMassSubsystem *ZombiMassSubsystem;

	// Referencia al ZombiSpawnerSubsystem
	UPROPERTY()
	UZombiSpawnerSubsystem *ZombiSpawnerSubsystem;

	// Referencia al StimulusSubsystem
	UPROPERTY()
	UStimulusSubsystem *StimulusSubsystem;

	// Referencia al PlayerSignalSubsystem
	UPROPERTY()
	UPlayerSignalSubsystem *PlayerSignalSubsystem;

	// Se llama cuando el GameMode inicia
	virtual void BeginPlay() override;
};
