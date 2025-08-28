// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"

#include "TurboSequence_Manager_Lf.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * ✅ PATRÓN OFICIAL: Procesador ECS que solo maneja lógica de comportamiento
 * Según documentación TurboSequence: "ECS is totally fine, you can update all instances in an ECS Loop"
 * PERO: NO llama funciones visuales - solo prepara datos para el controlador principal
 */
UCLASS()
class UZombiTurboSequenceProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiTurboSequenceProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
	// Query optimizada para State Sync
	FMassEntityQuery TransformSyncQuery{*this};

	// Configuración de rotación (configurable)
	UPROPERTY(EditAnywhere, Category = "TurboSequence")
	float RotationOffset = -90.0f;

	// ✅ PATRÓN OFICIAL: Solo funciones de lógica, NO visuales
	void PrepareAnimationLogic(FZombiTurboSequenceFragment &TurboSequenceFragment,
							   const FZombiBehaviorFragment &BehaviorFragment,
							   const FZombiCoreFragment &CoreFragment);

	void PrepareTransformLogic(FZombiTurboSequenceFragment &TurboSequenceFragment,
							   const FZombiCoreFragment &CoreFragment);

	// Función de preparación de optimización por distancia
	void PrepareDistanceOptimization(FZombiTurboSequenceFragment &TurboSequenceFragment,
									 const FZombiCoreFragment &CoreFragment);

	UAnimSequence *GetAnimationForState(const FZombiBehaviorFragment &BehaviorFragment,
										const FZombiCoreFragment &CoreFragment);
};
