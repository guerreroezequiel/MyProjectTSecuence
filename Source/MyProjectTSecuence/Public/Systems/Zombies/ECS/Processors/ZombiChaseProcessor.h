// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "ZombiChaseProcessor.generated.h"

/**
 * Procesador especializado para el comportamiento de persecución
 * DOP: Solo maneja lógica específica de chase, no otros comportamientos
 * Performance: Query optimizado para entidades persiguiendo
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiChaseProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiChaseProcessor();

protected:
    // Implementación del procesador Mass
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades persiguiendo
    UPROPERTY()
    FMassEntityQuery ChaseQuery;

    // Lógica de persecución
    void ProcessChaseLogic(FZombiStateFragment &StateFragment,
                           const FZombiTransformFragment &TransformFragment,
                           FZombiMovementFragment &MovementFragment,
                           const FZombiStimuliFragment &StimuliFragment,
                           float DeltaTime);

    // Calcular dirección de persecución
    FVector CalculateChaseDirection(const FVector &ZombiePosition,
                                    const FZombiStimuliFragment &StimuliFragment);

    // Determinar velocidad de persecución
    uint8 CalculateChaseSpeed(float DistanceToTarget, const FZombiStimuliFragment &StimuliFragment);

    // Verificar si debe continuar persiguiendo
    bool ShouldContinueChasing(const FZombiStateFragment &StateFragment,
                               const FZombiStimuliFragment &StimuliFragment,
                               float DistanceToTarget);

    // Cache para optimización
    FVector CachedPlayerLocation;
    float LastPlayerLocationUpdate;
    static constexpr float PLAYER_LOCATION_CACHE_DURATION = 0.1f; // 100ms cache
};
