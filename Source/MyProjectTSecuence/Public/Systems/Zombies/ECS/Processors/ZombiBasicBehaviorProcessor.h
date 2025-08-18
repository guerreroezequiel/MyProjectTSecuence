// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "ZombiBasicBehaviorProcessor.generated.h"

/**
 * Procesador unificado para comportamientos básicos: Idle + WalkAround
 * DOP: Solo procesa entidades con FIdleTag o FWalkingTag
 * Performance: Procesamiento optimizado para comportamientos simples
 * Reemplaza: ZombiIdleProcessor + ZombiWalkAroundProcessor
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiBasicBehaviorProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiBasicBehaviorProcessor();

protected:
    // Implementación del procesador Mass
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades Idle
    UPROPERTY()
    FMassEntityQuery IdleBehaviorQuery;

    // Query para entidades WalkAround
    UPROPERTY()
    FMassEntityQuery WalkAroundBehaviorQuery;

    // Lógica unificada para Idle
    void ProcessIdleBehavior(FZombiStateFragment &StateFragment,
                             FZombiTransformFragment &TransformFragment,
                             FZombiMovementFragment &MovementFragment,
                             float DeltaTime);

    // Lógica unificada para WalkAround
    void ProcessWalkAroundBehavior(FZombiStateFragment &StateFragment,
                                   const FZombiTransformFragment &TransformFragment,
                                   FZombiMovementFragment &MovementFragment,
                                   float DeltaTime);

    // Utilidades compartidas
    FVector GenerateRandomDirection();
    uint8 CalculateWalkSpeed();
    bool ShouldChangeDirection(const FZombiStateFragment &StateFragment);
    bool ShouldPlayIdleAnimation(const FZombiStateFragment &StateFragment);

    // Configuración
    static constexpr float DIRECTION_CHANGE_MIN_TIME = 2.0f;
    static constexpr float DIRECTION_CHANGE_MAX_TIME = 4.0f;
    static constexpr uint8 MIN_WALK_SPEED = 30;
    static constexpr uint8 MAX_WALK_SPEED = 70;
    static constexpr float IDLE_ANIMATION_MIN_TIME = 3.0f;
    static constexpr float IDLE_ANIMATION_MAX_TIME = 8.0f;
};
