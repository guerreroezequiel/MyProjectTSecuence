// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiMovementProcessor.generated.h"

/**
 * Procesador especializado SOLO para movimiento y rotación
 * Optimizado para cache locality - accede solo a fragmentos de movimiento
 * Procesa por orden de prioridad usando queries por frecuencia
 */
UCLASS()
class UZombiMovementProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiMovementProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
	// Query base para entidades activas
	FMassEntityQuery MovementQuery{*this};

	// Queries por frecuencia de actualización (orden de prioridad)
	FMassEntityQuery Update60FPSQuery{*this}; // TakeDamage, Attack - Crítico
	FMassEntityQuery Update30FPSQuery{*this}; // Chase - Alta prioridad
	FMassEntityQuery Update15FPSQuery{*this}; // Seek, WalkAround - Normal
	FMassEntityQuery Update5FPSQuery{*this};  // Idle, Dead - Mínima

	// Referencia al jugador para cálculos de movimiento
	UPROPERTY()
	APawn *PlayerPawn;

	// Funciones auxiliares
	FVector GenerateRandomDirection() const;
	FVector ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const;

	// Funciones de movimiento específicas
	void ProcessPlayerChaseMovement(FZombiCoreFragment &CoreFragment, float DeltaTime);
	void ProcessRandomMovement(FZombiCoreFragment &CoreFragment, float DeltaTime);
	void ProcessRotationAndMovement(FZombiCoreFragment &CoreFragment, const FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);
};
