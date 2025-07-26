// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiStateFragment.h"
#include "TurboSequence_Manager_Lf.h"
#include "MyTurboSequenceAnimComponent.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * Procesador para sincronizar entidades Mass con instancias visuales TurboSequence
 * Enfoque: State Sync Architecture con Blend Space para animaciones
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
									 const FZombiTurboSequenceFragment &TurboSequenceFragment,
									 const FZombiStateFragment &StateFragment,
									 const FZombiMovementFragment &MovementFragment);
};
