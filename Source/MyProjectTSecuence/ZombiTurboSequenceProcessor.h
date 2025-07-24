// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "ZombiTurboSequenceProcessor.generated.h"

// Procesador que maneja la creación y sincronización de instancias visuales de TurboSequence
// Sigue el patrón State Sync: solo maneja la parte visual, sin lógica de juego
UCLASS()
class MYPROJECTTSECUENCE_API UZombiTurboSequenceProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiTurboSequenceProcessor();

public:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
	// Query para entidades que necesitan instancias visuales
	FMassEntityQuery VisualInstanceQuery;

	// Query para entidades que necesitan sincronización de transformación
	FMassEntityQuery TransformSyncQuery;

	// Crea una instancia visual de TurboSequence
	void CreateTurboSequenceInstance(FZombiTurboSequenceFragment &TurboSequenceFragment,
									 const FZombiMovementFragment &MovementFragment,
									 UWorld *World);

	// Actualiza la transformación de la instancia visual
	void UpdateTurboSequenceTransform(const FZombiTurboSequenceFragment &TurboSequenceFragment,
									  const FZombiMovementFragment &MovementFragment);

	// Destruye una instancia visual de TurboSequence
	void DestroyTurboSequenceInstance(const FZombiTurboSequenceFragment &TurboSequenceFragment,
									  UWorld *World);
};
