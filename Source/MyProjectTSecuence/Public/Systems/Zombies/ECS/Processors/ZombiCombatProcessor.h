// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCombatFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiCombatProcessor.generated.h"

/**
 * Procesador especializado SOLO para combate y daño
 * Optimizado para cache locality - accede solo a fragmentos de combate
 */
UCLASS()
class UZombiCombatProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiCombatProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades activas que pueden combatir
    FMassEntityQuery CombatQuery{*this};

    // Funciones auxiliares de combate
    void UpdateCombatCooldowns(FZombiCombatFragment &CombatFragment, float DeltaTime);
    void ProcessAttackLogic(FZombiCombatFragment &CombatFragment, FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);
    void ProcessDamageLogic(FZombiCombatFragment &CombatFragment, FZombiBehaviorFragment &BehaviorFragment, float DeltaTime);
};