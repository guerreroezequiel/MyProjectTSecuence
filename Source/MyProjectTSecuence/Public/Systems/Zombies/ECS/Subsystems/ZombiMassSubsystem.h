// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntitySubsystem.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Coordinators/ZombiSystemCoordinator.h"
// Procesadores individuales están siendo migrados al coordinador unificado
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
	virtual void OnWorldBeginPlay(UWorld &InWorld) override;

	// Registra una entidad zombi en el sistema Mass Entity
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

	// Referencia al coordinador principal (reemplaza procesadores individuales)
	UPROPERTY()
	UZombiSystemCoordinator *SystemCoordinator = nullptr;

	// Banderas para verificar si el sistema está configurado
	bool bCoordinatorInitialized = false;

	// Genera una rotación aleatoria para spawning
	FRotator GenerateRandomRotation() const;

	// Inicializa el coordinador principal
	void InitializeSystemCoordinator();

private:
	// Verifica que el coordinador está inicializado correctamente
	void VerifyCoordinatorInitialization();
};
