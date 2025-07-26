// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiCoreFragment.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiCombatFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "ZombiBehaviorFragment.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * Procesador para sincronizar entidades Mass con instancias visuales TurboSequence
 * Optimizado para usar fragmentos especializados
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
	// Query para sincronizar transformaciones
	FMassEntityQuery TransformSyncQuery{*this};

	// Query para actualizar Blend Space (futuro)
	FMassEntityQuery BlendSpaceQuery{*this};

	// Función para actualizar animaciones basadas en estado del zombi
	void UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
									 FZombiTurboSequenceFragment &TurboSequenceFragment,
									 const FZombiBehaviorFragment &BehaviorFragment,
									 const FZombiCoreFragment &CoreFragment,
									 const FZombiCombatFragment &CombatFragment);

	// Función para cachear animaciones y optimizar búsquedas
	void CacheAnimations(FZombiTurboSequenceFragment &TurboSequenceFragment);
};
