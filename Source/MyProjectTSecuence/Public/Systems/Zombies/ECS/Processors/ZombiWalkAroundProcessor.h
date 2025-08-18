// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "ZombiWalkAroundProcessor.generated.h"

/**
 * Procesador especializado para el comportamiento de caminar sin rumbo
 * DOP: Solo maneja lógica específica de walkaround, no otros comportamientos
 * Performance: Query optimizado para entidades caminando
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiWalkAroundProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiWalkAroundProcessor();

protected:
    // Implementación del procesador Mass
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para entidades caminando
    UPROPERTY()
    FMassEntityQuery WalkAroundQuery;

    // Lógica de caminar sin rumbo
    void ProcessWalkAroundLogic(FZombiStateFragment &StateFragment,
                                const FZombiTransformFragment &TransformFragment,
                                FZombiMovementFragment &MovementFragment,
                                float DeltaTime);

    // Generar nueva dirección aleatoria
    FVector GenerateRandomDirection();

    // Calcular velocidad de caminar
    uint8 CalculateWalkSpeed();

    // Verificar si debe cambiar de dirección
    bool ShouldChangeDirection(const FZombiStateFragment &StateFragment);

    // Configuración SIMPLE
    static constexpr float DIRECTION_CHANGE_MIN_TIME = 2.0f;
    static constexpr float DIRECTION_CHANGE_MAX_TIME = 4.0f;
    static constexpr uint8 MIN_WALK_SPEED = 30;
    static constexpr uint8 MAX_WALK_SPEED = 70;
};
