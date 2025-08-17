// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "ZombiIdleProcessor.generated.h"

/**
 * Procesador especializado para el comportamiento inactivo SIMPLE
 * DOP: Solo maneja quedarse parado y rotar un poco cada tanto
 * Performance: Query optimizado para entidades inactivas, procesamiento mínimo (15 FPS)
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiIdleProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiIdleProcessor();

protected:
    // Implementación del procesador Mass
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades inactivas
    UPROPERTY()
    FMassEntityQuery IdleQuery;

    // Lógica simple de idle
    void ProcessIdleLogic(FZombiStateFragment &StateFragment,
                          FZombiTransformFragment &TransformFragment,
                          FZombiMovementFragment &MovementFragment,
                          float DeltaTime);

    // Verificar si debe rotar (animación idle simple)
    bool ShouldPlayIdleAnimation(const FZombiStateFragment &StateFragment);

    // Configuración SIMPLE de idle
    static constexpr float IDLE_ANIMATION_MIN_TIME = 3.0f;
    static constexpr float IDLE_ANIMATION_MAX_TIME = 8.0f;
};