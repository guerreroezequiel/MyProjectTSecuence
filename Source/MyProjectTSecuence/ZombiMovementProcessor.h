// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiMovementFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_Manager_Lf.h"
#include "MassProcessingTypes.h"
#include "ZombiMovementProcessor.generated.h"

// Procesador que maneja el movimiento y rotación de los zombis
// Optimizado para rendimiento con miles de entidades
UCLASS()
class MYPROJECTTSECUENCE_API UZombiMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiMovementProcessor();

public:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }

private:
	// Query para entidades que necesitan movimiento - inicializado con el procesador
	FMassEntityQuery MovementQuery{*this};

	// Genera una dirección aleatoria para el movimiento
	FVector GenerateRandomDirection() const;

	// Verifica si el zombi está dentro del radio de movimiento
	bool IsWithinMovementRadius(const FVector &Position, const FVector &Center, float Radius) const;

	// Ajusta la posición para mantener al zombi dentro del área
	FVector ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const;

	// Crea o actualiza la instancia visual de TurboSequence
	void UpdateTurboSequenceInstance(const FZombiMovementFragment &MovementFragment);
};
