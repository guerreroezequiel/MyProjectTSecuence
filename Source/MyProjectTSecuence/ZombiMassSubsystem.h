// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntitySubsystem.h"
#include "ZombiStateFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementProcessor.h"
#include "ZombiTurboSequenceProcessor.h"
#include "ZombiUpdateProcessor.h"
#include "ZombiMassSubsystem.generated.h"

// Subsystem que maneja el registro de entidades zombi en el sistema Mass Entity
// Ahora usa el nuevo State Sync sin dependencia de Actors
UCLASS()
class MYPROJECTTSECUENCE_API UZombiMassSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UZombiMassSubsystem();

	// Inicialización del subsystem
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld &InWorld) override;

	// Registra una entidad zombi en el sistema Mass Entity
	// Ahora crea entidades puras sin Actors
	FMassEntityHandle RegisterZombiEntity(const FVector &SpawnLocation,
										  class UTurboSequence_MeshAsset_Lf *TurboSequenceAsset);

	// Desregistra una entidad zombi
	void UnregisterZombiEntity(FMassEntityHandle EntityHandle);

	// Obtiene el número de entidades registradas
	int32 GetRegisteredEntityCount() const { return RegisteredEntities.Num(); }

	// Limpia todas las entidades registradas
	void ClearAllEntities();

private:
	// Referencia al Mass Entity Subsystem
	UPROPERTY()
	UMassEntitySubsystem *MassEntitySubsystem;

	// Lista de entidades registradas (para limpieza)
	TArray<FMassEntityHandle> RegisteredEntities;

	// Banderas para verificar si el sistema está configurado
	bool bProcessorsRegistered = false;

	// Genera una rotación aleatoria para spawning
	FRotator GenerateRandomRotation() const;

	// Registra los procesadores de Mass Entity
	void RegisterMassProcessors();

public:
	// Ejecuta los procesadores manualmente cada frame (DEPRECATED - Ahora se ejecutan automáticamente)
	void ExecuteProcessorsManually(float DeltaTime);
};
