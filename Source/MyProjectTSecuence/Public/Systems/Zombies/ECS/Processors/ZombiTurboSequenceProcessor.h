// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUltraConsolidatedFragment.h"
// Helpers eliminados - lógica movida al procesador ultra-consolidado
#include "TurboSequence_Manager_Lf.h"
#include "ZombiTurboSequenceProcessor.generated.h"

/**
 * Procesador para sincronizar entidades Mass con instancias visuales TurboSequence
 * Simplificado para usar fragmento ultra-consolidado
 * Solo sincroniza transformaciones - las animaciones se manejan automáticamente por TurboSequence
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

	/**
	 * @brief Método simplificado para el fragmento ultra-consolidado
	 * @param Context Contexto de ejecución
	 * @param EntityIndex Índice de la entidad
	 * @param UltraFragment Fragmento ultra-consolidado
	 */
	void UpdateAnimationBasedOnState(FMassExecutionContext &Context, int32 EntityIndex,
									 FZombiUltraConsolidatedFragment &UltraFragment);
};
