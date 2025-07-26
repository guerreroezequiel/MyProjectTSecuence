// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "ZombiChaseFragment.h"
#include "ZombiTransformFragment.h"
#include "ZombiVelocityFragment.h"
#include "ZombiStateFragment.h"
#include "ZombiTags.h"
#include "ZombiChaseProcessor.generated.h"

// Procesador especializado para persecución al jugador
// OPTIMIZADO para cache locality y paralelización
UCLASS()
class MYPROJECTTSECUENCE_API UZombiChaseProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiChaseProcessor();

public:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;
    virtual bool ShouldAllowQueryBasedPruning(const bool bRuntimeMode = true) const override { return false; }

private:
    // Query para entidades que pueden perseguir
    FMassEntityQuery ChaseQuery{*this};

    // Query para entidades que están persiguiendo
    FMassEntityQuery ChasingQuery{*this};

    // Referencia al jugador (se actualiza cada frame)
    UPROPERTY()
    class AMyProjectTSecuenceCharacter *PlayerCharacter = nullptr;

    // Posición del jugador (cached para evitar búsquedas repetitivas)
    FVector CachedPlayerPosition = FVector::ZeroVector;

    // Timer global para el ciclo de persecución
    float GlobalChaseTimer = 0.0f;

    // Estado global de persecución
    bool bGlobalChaseActive = false;
};