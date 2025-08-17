// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiMovementProcessorOptimized.generated.h"

/**
 * Procesador especializado para movimiento de zombies
 * OPTIMIZADO: Solo maneja movimiento (16 bytes por entidad)
 * DOP: Procesamiento especializado para mejor cache locality
 * Objetivo: 10,000 entidades con máximo rendimiento
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiMovementProcessorOptimized : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiMovementProcessorOptimized();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    // Query principal para entidades con movimiento
    FMassEntityQuery MovementQuery{*this};

    // Query para entidades persiguiendo
    FMassEntityQuery ChasingQuery{*this};

    // Query para entidades caminando
    FMassEntityQuery WalkingQuery{*this};

    // Query para entidades inactivas
    FMassEntityQuery IdleQuery{*this};

    // OPTIMIZACIÓN: Tamaños de lote optimizados para cache locality
    static constexpr int32 BATCH_SIZE_CRITICAL = 32;   // Entidades cercanas (60 FPS)
    static constexpr int32 BATCH_SIZE_HIGH = 64;       // Entidades visibles (30 FPS)
    static constexpr int32 BATCH_SIZE_NORMAL = 128;    // Entidades de fondo (15 FPS)
    static constexpr int32 BATCH_SIZE_LOW = 256;       // Entidades lejanas (5 FPS)

    // OPTIMIZACIÓN: Cache para posición del jugador
    mutable FVector CachedPlayerLocation = FVector::ZeroVector;
    mutable float LastPlayerLocationUpdate = 0.0f;
    static constexpr float PLAYER_LOCATION_CACHE_DURATION = 0.1f; // 100ms cache

    /**
     * @brief Procesa movimiento de entidades persiguiendo (60 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param DeltaTime Tiempo delta
     */
    void ProcessChasingMovement(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float DeltaTime);

    /**
     * @brief Procesa movimiento de entidades caminando (30 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param DeltaTime Tiempo delta
     */
    void ProcessWalkingMovement(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float DeltaTime);

    /**
     * @brief Procesa movimiento de entidades inactivas (15 FPS)
     * @param EntityManager Manager de entidades Mass
     * @param Context Contexto de ejecución
     * @param DeltaTime Tiempo delta
     */
    void ProcessIdleMovement(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float DeltaTime);

    /**
     * @brief Aplica movimiento de persecución a una entidad
     * @param MovementFragment Fragmento de movimiento
     * @param TransformFragment Fragmento de transformación
     * @param StateFragment Fragmento de estado
     * @param DeltaTime Tiempo delta
     */
    void ApplyChasingMovement(FZombiMovementFragment& MovementFragment, 
                             FZombiTransformFragment& TransformFragment,
                             const FZombiStateFragment& StateFragment,
                             float DeltaTime);

    /**
     * @brief Aplica movimiento de caminata a una entidad
     * @param MovementFragment Fragmento de movimiento
     * @param TransformFragment Fragmento de transformación
     * @param StateFragment Fragmento de estado
     * @param DeltaTime Tiempo delta
     */
    void ApplyWalkingMovement(FZombiMovementFragment& MovementFragment, 
                             FZombiTransformFragment& TransformFragment,
                             const FZombiStateFragment& StateFragment,
                             float DeltaTime);

    /**
     * @brief Aplica movimiento inactivo a una entidad
     * @param MovementFragment Fragmento de movimiento
     * @param TransformFragment Fragmento de transformación
     * @param StateFragment Fragmento de estado
     * @param DeltaTime Tiempo delta
     */
    void ApplyIdleMovement(FZombiMovementFragment& MovementFragment, 
                          FZombiTransformFragment& TransformFragment,
                          const FZombiStateFragment& StateFragment,
                          float DeltaTime);

    /**
     * @brief Obtiene la posición del jugador usando el sistema de estímulos
     * @return Posición del jugador en el mundo (optimizado con cache)
     */
    FVector GetPlayerLocation() const;

    /**
     * @brief Obtiene la posición del jugador directamente del StimulusSubsystem
     * @return Posición del jugador o FVector::ZeroVector si no está disponible
     */
    FVector GetPlayerLocationFromStimulus() const;

    /**
     * @brief Valida el movimiento de una entidad
     * @param MovementFragment Fragmento de movimiento
     * @param TransformFragment Fragmento de transformación
     * @return true si el movimiento es válido
     */
    bool ValidateMovement(const FZombiMovementFragment& MovementFragment, 
                         const FZombiTransformFragment& TransformFragment) const;

    /**
     * @brief Corrige movimientos inválidos
     * @param MovementFragment Fragmento de movimiento
     * @param TransformFragment Fragmento de transformación
     */
    void FixInvalidMovement(FZombiMovementFragment& MovementFragment, 
                           FZombiTransformFragment& TransformFragment) const;
};
