// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTurboSequenceFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"

#include "TurboSequence_Manager_Lf.h"
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
									 const FZombiStateFragment &StateFragment,
									 const FZombiTransformFragment &TransformFragment);

	// Función para cachear animaciones y optimizar búsquedas
	void CacheAnimations(FZombiTurboSequenceFragment &TurboSequenceFragment);
};
