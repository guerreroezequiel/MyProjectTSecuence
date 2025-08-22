// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"

#include "TurboSequence_Manager_Lf.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * Procesador optimizado para sincronizar entidades Mass con instancias visuales TurboSequence
 * OPTIMIZADO: Para 5000+ entidades con State Sync Architecture
 * Eliminadas transiciones manuales - TurboSequence maneja las transiciones automáticamente
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

	// Funciones optimizadas
	void UpdateAnimation(FZombiTurboSequenceFragment &TurboSequenceFragment,
						 const FZombiBehaviorFragment &BehaviorFragment,
						 const FZombiCoreFragment &CoreFragment);

	void SyncTransform(FZombiTurboSequenceFragment &TurboSequenceFragment,
					   const FZombiCoreFragment &CoreFragment);

	// Función de optimización de sombras
	void UpdateShadowSettings(FZombiTurboSequenceFragment &TurboSequenceFragment,
							  const FZombiCoreFragment &CoreFragment);

	UAnimSequence *GetAnimationForState(const FZombiBehaviorFragment &BehaviorFragment,
										const FZombiCoreFragment &CoreFragment);
};
