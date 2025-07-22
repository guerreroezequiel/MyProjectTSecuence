// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZombiMassSubsystem.h"
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

	// Se llama cuando el GameMode inicia
	virtual void BeginPlay() override;
};
