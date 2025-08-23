// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiCoreFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiBehaviorFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiLODFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "ZombiLODProcessor.generated.h"

/**
 * Procesador LOD para optimización inteligente
 * Maneja Level of Detail basado en estado + distancia + estímulos
 * Sincroniza tags con estados para enfoque híbrido
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiLODProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiLODProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    // Query para entidades con LOD
    FMassEntityQuery LODQuery;

    // Referencia al jugador para cálculos de distancia
    UPROPERTY()
    APawn* PlayerPawn;

    // Tiempo actual para cálculos de LOD
    float CurrentTime;

    // Configuración de LOD
    UPROPERTY()
    float CriticalDistance = 200.0f;    // < 200u = 60 FPS
    UPROPERTY()
    float HighDistance = 500.0f;        // 200-500u = 30 FPS
    UPROPERTY()
    float NormalDistance = 800.0f;      // 500-800u = 15 FPS
    UPROPERTY()
    float LowDistance = 800.0f;         // > 800u = 5 FPS

    // Métodos de LOD
    void CalculateDistanceToPlayer(const FVector& ZombieLocation, float& OutDistance);
    void UpdateLODSettings(FZombiLODFragment& LODFragment, const FZombiBehaviorFragment& BehaviorFragment, float Distance);
    void SynchronizeTagsWithState(FMassEntityManager& EntityManager, FMassEntityHandle Entity, const FZombiBehaviorFragment& BehaviorFragment);
    void ApplyFrustumCulling(FZombiLODFragment& LODFragment, const FVector& ZombieLocation);
    void SetZombieState(FMassEntityManager& EntityManager, FMassEntityHandle Entity, EZombiState NewState);

    // Métodos de utilidad
    bool IsInFrustum(const FVector& Location) const;
    float CalculateStimulusIntensity(const FZombiBehaviorFragment& BehaviorFragment) const;
};
