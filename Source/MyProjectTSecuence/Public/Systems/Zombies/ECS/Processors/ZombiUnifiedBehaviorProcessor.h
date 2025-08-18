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
#include "ZombiUnifiedBehaviorProcessor.generated.h"

/**
 * NUEVO ENFOQUE: Un solo procesador que maneja todo el comportamiento zombie
 *
 * PATRÓN STATE SYNC + STIMULUS-DRIVEN:
 * 1. COMPORTAMIENTO AUTOMÁTICO: Idle ↔ WalkAround (default loop)
 * 2. STIMULUS OVERRIDE: Chase/Flee/Investigate (cuando hay estímulos)
 * 3. ANIMATION SYNC: Estado → TurboSequence sync optimizado
 *
 * DOP/ECS: Un solo processor, todos los fragments necesarios, optimizado para batching
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiUnifiedBehaviorProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiUnifiedBehaviorProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Una sola query para todos los zombies activos
    UPROPERTY()
    FMassEntityQuery UnifiedBehaviorQuery;

    // FASE 1: Evaluar estímulos y decidir comportamiento
    void EvaluateStimulusResponse(FZombiStateFragment &StateFragment,
                                  const FZombiStimuliFragment &StimuliFragment,
                                  FZombiMovementFragment &MovementFragment);

    // FASE 2: Ejecutar comportamiento automático (si no hay stimulus override)
    void ExecuteAutomaticBehavior(FZombiStateFragment &StateFragment,
                                  const FZombiTransformFragment &TransformFragment,
                                  FZombiMovementFragment &MovementFragment,
                                  float DeltaTime);

    // FASE 3: Sincronizar estado con animaciones TurboSequence (optimizado)
    void SynchronizeAnimationState(const FZombiStateFragment &StateFragment,
                                   const FZombiMovementFragment &MovementFragment);

    // Comportamientos específicos
    void ExecuteIdleBehavior(FZombiStateFragment &StateFragment,
                             const FZombiTransformFragment &TransformFragment,
                             FZombiMovementFragment &MovementFragment);

    void ExecuteWalkAroundBehavior(FZombiStateFragment &StateFragment,
                                   const FZombiTransformFragment &TransformFragment,
                                   FZombiMovementFragment &MovementFragment);

    void ExecuteChaseBehavior(FZombiStateFragment &StateFragment,
                              const FZombiStimuliFragment &StimuliFragment,
                              FZombiMovementFragment &MovementFragment);

    // Utilidades
    FVector GenerateRandomDirection();
    bool ShouldTransitionFromIdle(const FZombiStateFragment &StateFragment);
    bool ShouldTransitionFromWalk(const FZombiStateFragment &StateFragment);
    bool ShouldOverrideWithStimulus(const FZombiStimuliFragment &StimuliFragment);

    // Configuración (hardcoded por ahora)
    static constexpr float IDLE_TO_WALK_TIME = 2.0f;
    static constexpr float WALK_TO_IDLE_TIME = 4.0f;
    static constexpr float DIRECTION_CHANGE_TIME = 3.0f;
    static constexpr uint8 DEFAULT_WALK_SPEED = 50;
    static constexpr uint8 CHASE_SPEED = 100;
};
