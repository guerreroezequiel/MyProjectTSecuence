// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiTransformProcessor.generated.h"

/**
 * Procesador especializado para transformación de zombies
 * OPTIMIZADO: Solo maneja transformación y validación
 * DOP: Procesamiento especializado para mejor cache locality
 * Objetivo: 10,000 entidades con máximo rendimiento
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiTransformProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiTransformProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query principal para transformaciones
    FMassEntityQuery TransformQuery{*this};

    // Funciones de validación
    bool ValidateTransform(const FZombiTransformFragment &TransformFragment) const;
    void FixInvalidTransform(FZombiTransformFragment &TransformFragment) const;
};