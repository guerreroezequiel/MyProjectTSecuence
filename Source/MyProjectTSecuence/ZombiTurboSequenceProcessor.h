// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiStateFragment.h"
#include "ZombiMovementFragment.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * Procesador para sincronizar entidades Mass con instancias visuales de TurboSequence
 * Implementa el patrón State Sync para separar lógica de visualización
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiTurboSequenceProcessor : public UMassProcessor
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

	// Query para controlar animaciones con Blend Space
	FMassEntityQuery VisualInstanceQuery{*this};

	/**
	 * Actualiza la animación usando Blend Space según el estado lógico
	 * Implementa State Sync: el host solo envía estado, el cliente maneja animaciones
	 */
	void UpdateBlendSpaceAnimation(const FZombiTurboSequenceFragment &TurboSequenceFragment,
								   const FZombiStateFragment &StateFragment);

	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }
};
