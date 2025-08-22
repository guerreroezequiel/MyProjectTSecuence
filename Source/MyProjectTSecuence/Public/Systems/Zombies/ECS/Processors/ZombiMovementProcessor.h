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
	// Query para entidades activas que necesitan movimiento
	FMassEntityQuery MovementQuery{*this};

	// Funciones auxiliares
	FVector GenerateRandomDirection() const;
	FVector ClampToMovementArea(const FVector &Position, const FVector &Center, float Radius) const;
};
