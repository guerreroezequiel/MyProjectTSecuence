// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntitySubsystem.h"
#include "ZombiMassSubsystem.generated.h"

// Subsystem que maneja el registro de entidades zombi en el sistema Mass Entity
UCLASS()
class MYPROJECTTSECUENCE_API UZombiMassSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UZombiMassSubsystem();

	// Inicialización del subsystem
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;

	// Registra un actor zombi en el sistema Mass Entity
	UFUNCTION(BlueprintCallable, Category = "Zombi Mass")
	void RegisterZombiEntity(AActor *ZombiActor);

	// Desregistra un actor zombi del sistema Mass Entity
	UFUNCTION(BlueprintCallable, Category = "Zombi Mass")
	void UnregisterZombiEntity(AActor *ZombiActor);

	// Obtiene el subsystem de Mass Entity
	UFUNCTION(BlueprintCallable, Category = "Zombi Mass")
	UMassEntitySubsystem *GetMassEntitySubsystem() const { return MassEntitySubsystem; }

private:
	// Referencia al subsystem de Mass Entity
	UPROPERTY()
	TObjectPtr<UMassEntitySubsystem> MassEntitySubsystem;

	// Mapa que relaciona actores con sus entidades Mass
	UPROPERTY()
	TMap<AActor *, FMassEntityHandle> RegisteredEntities;
};
