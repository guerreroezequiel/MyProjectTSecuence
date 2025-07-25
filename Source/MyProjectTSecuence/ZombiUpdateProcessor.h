// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "TurboSequence_Manager_Lf.h"
#include "MassProcessingTypes.h"
#include "ZombiTurboSequenceFragment.h"
#include "ZombiUpdateProcessor.generated.h"

// Procesador que maneja Update Groups y llama a SolveMeshes cada frame
// Esencial para el rendimiento optimizado de TurboSequence
UCLASS()
class MYPROJECTTSECUENCE_API UZombiUpdateProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiUpdateProcessor();

public:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;
	virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }

private:
	// Número de grupos de actualización para distribuir la carga
	static const int32 NUM_UPDATE_GROUPS = 4;

	// Query para procesar grupos de actualización de TurboSequence
	FMassEntityQuery UpdateGroupQuery{*this};

	// Contexto de actualización para TurboSequence
	FTurboSequence_UpdateContext_Lf UpdateContext;

	// Resuelve las animaciones para un grupo específico
	void SolveUpdateGroup(int32 GroupIndex, float DeltaTime);
};
