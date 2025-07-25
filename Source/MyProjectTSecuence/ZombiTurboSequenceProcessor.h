// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "MassProcessingTypes.h"
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
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }

private:
	// Query para entidades que necesitan instancias visuales - inicializado con el procesador
	FMassEntityQuery VisualInstanceQuery{*this};

	// Query para entidades que necesitan sincronización de transformación - inicializado con el procesador
	FMassEntityQuery TransformSyncQuery{*this};

	// Crea una instancia visual de TurboSequence
	void CreateTurboSequenceInstance(FZombiTurboSequenceFragment &TurboSequenceFragment,
									 const FZombiMovementFragment &MovementFragment,
									 UWorld *World);

	// Actualiza la transformación de la instancia visual
	void UpdateTurboSequenceTransform(const FZombiTurboSequenceFragment &TurboSequenceFragment,
									  const FZombiMovementFragment &MovementFragment);

	// Actualiza la animación de la instancia visual según el estado lógico
	void UpdateTurboSequenceAnimation(const FZombiTurboSequenceFragment &TurboSequenceFragment,
									  const FZombiStateFragment &StateFragment);

	// Destruye una instancia visual de TurboSequence
	void DestroyTurboSequenceInstance(const FZombiTurboSequenceFragment &TurboSequenceFragment,
									  UWorld *World);
};
