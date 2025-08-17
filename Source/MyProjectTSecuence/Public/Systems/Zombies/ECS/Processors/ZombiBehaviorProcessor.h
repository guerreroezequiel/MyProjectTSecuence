// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"

#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiBehaviorProcessor.generated.h"

/**
 * Procesador especializado SOLO para IA y decisiones de comportamiento
 * Optimizado para cache locality - accede solo a fragmentos de comportamiento
 */
UCLASS()
class UZombiBehaviorProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiBehaviorProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades activas que necesitan decisiones de IA
    FMassEntityQuery BehaviorQuery{*this};

    // Funciones auxiliares de IA - COMPATIBLE CON DOP
    void UpdateHordeBehavior(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime);
    void UpdateActionTimers(FZombiStateFragment &StateFragment, float DeltaTime);

    // Funciones de estados DOP-compatibles
    void EvaluateStateTransitions(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment);
    void UpdateCurrentState(FZombiStateFragment &StateFragment, const FZombiTransformFragment &TransformFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime);

    // Funciones específicas de estados
    void UpdateChaseState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiStimuliFragment &StimuliFragment, float DeltaTime);
    void UpdateWalkAroundState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, const FZombiTransformFragment &TransformFragment, float DeltaTime);
    void UpdateIdleState(FZombiStateFragment &StateFragment, FZombiMovementFragment &MovementFragment, float DeltaTime);
};