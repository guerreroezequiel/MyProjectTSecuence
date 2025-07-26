// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ZombiTransformFragment.h"
#include "ZombiVelocityFragment.h"
#include "ZombiTags.h"
#include "ZombiTransformProcessor.generated.h"

// Procesador especializado solo para transformaciones
// OPTIMIZADO para cache locality y paralelización
UCLASS()
class MYPROJECTTSECUENCE_API UZombiTransformProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiTransformProcessor();

public:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;
    virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }

private:
    // Query para entidades que necesitan actualización de transformación
    FMassEntityQuery TransformQuery{*this};
};