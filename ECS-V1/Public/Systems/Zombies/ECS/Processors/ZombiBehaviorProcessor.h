// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiBehaviorProcessor.generated.h"

/**
 * Procesador especializado SOLO para IA y decisiones de comportamiento
 * Optimizado para cache locality - accede solo a fragmentos de comportamiento
 * Procesa por orden de prioridad usando queries por frecuencia
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
    // Query base para entidades activas
    FMassEntityQuery BehaviorQuery{*this};

    // Queries por frecuencia de actualización (orden de prioridad)
    FMassEntityQuery Update60FPSQuery{*this}; // TakeDamage, Attack - Crítico
    FMassEntityQuery Update30FPSQuery{*this}; // Chase - Alta prioridad
    FMassEntityQuery Update15FPSQuery{*this}; // Seek, WalkAround - Normal
    FMassEntityQuery Update5FPSQuery{*this};  // Idle, Dead - Mínima

    // Referencia al jugador para cálculos de distancia
    UPROPERTY()
    APawn *PlayerPawn;

    // Funciones auxiliares de IA - COMPATIBLE CON DOP
    void UpdateHordeBehavior(FZombiBehaviorFragment &BehaviorFragment, const FZombiCoreFragment &CoreFragment, float DeltaTime);
    void UpdateActionTimers(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);

    // Funciones de estados simplificadas
    void EvaluateStateTransitions(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer);
    void UpdateCurrentState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime);

    // Funciones específicas de estados
    void UpdateChaseState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime);
    void UpdateSeekState(FZombiBehaviorFragment &BehaviorFragment, float DistanceToPlayer, float DeltaTime);
    void UpdateWalkAroundState(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);
    void UpdateIdleState(FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);

    // Función para calcular distancia al jugador
    float CalculateDistanceToPlayer(const FVector &ZombieLocation);
};