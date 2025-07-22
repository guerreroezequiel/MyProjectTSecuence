// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "ZombiStateFragment.h"
#include "ZombiStateSyncProcessor.generated.h"

// Processor que sincroniza el estado lógico Mass con la animación visual de TurboSequence
UCLASS()
class MYPROJECTTSECUENCE_API UZombiStateSyncProcessor : public UMassProcessor
{
	GENERATED_BODY()

public:
	UZombiStateSyncProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

	FMassEntityQuery Query;
};
