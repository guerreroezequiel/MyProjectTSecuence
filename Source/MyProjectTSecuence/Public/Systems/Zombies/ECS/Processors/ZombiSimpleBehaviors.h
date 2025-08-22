#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUpdateFrequencyFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiMovementFragment.h"
#include "ZombiSimpleBehaviors.generated.h"

/**
 * Procesador unificado para todos los comportamientos básicos de zombies
 * Combina: Stimulus + Behavior + Movement + LOD en un solo procesador
 * Optimizado para 10,000 entidades
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiSimpleBehaviors : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiSimpleBehaviors();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query unificada para todos los comportamientos
    FMassEntityQuery SimpleBehaviorsQuery;

    // Referencia al StimulusSubsystem
    UPROPERTY()
    TObjectPtr<UStimulusSubsystem> StimulusSubsystem;

    // Cache de estímulos activos
    TArray<FStimulusData> CachedActiveStimuli;

    // Cache del último estímulo del jugador
    FStimulusData CachedLatestPlayerStimulus;
    bool bHasCachedPlayerStimulus = false;

    // Configuración
    float StimulusDetectionRange = 1000.0f;
    float StimulusUpdateInterval = 0.1f;
    float CacheUpdateTimer = 0.0f;

    // Métodos auxiliares
    void UpdateStimulusCache(float DeltaTime);
    void ProcessStimuliForZombie(const FVector &ZombiePosition, FZombiStimuliFragment &StimuliFragment);
    void ProcessBehaviorForZombie(FZombiStateFragment &StateFragment,
                                  FZombiMovementFragment &MovementFragment,
                                  const FZombiStimuliFragment &StimuliFragment,
                                  float DeltaTime);
    void ProcessMovementForZombie(FZombiTransformFragment &TransformFragment,
                                  FZombiMovementFragment &MovementFragment,
                                  const FZombiStateFragment &StateFragment,
                                  float DeltaTime);
    void SyncVisualState(const FZombiStateFragment &StateFragment,
                         const FZombiMovementFragment &MovementFragment,
                         int32 EntityIndex);

    bool IsStimulusInRange(const FVector &ZombiePosition, const FStimulusData &Stimulus) const;
    void UpdateResponseTimers(FZombiStimuliFragment &StimuliFragment, float DeltaTime);

    // Utilidades de comportamiento
    FVector GenerateRandomDirection();
    bool ShouldTransitionFromIdle(const FZombiStateFragment &StateFragment);
    bool ShouldTransitionFromWalk(const FZombiStateFragment &StateFragment);
    bool ShouldOverrideWithStimulus(const FZombiStimuliFragment &StimuliFragment);

    // Configuración de comportamiento
    static constexpr float IDLE_TO_WALK_TIME = 2.0f;
    static constexpr float WALK_TO_IDLE_TIME = 4.0f;
    static constexpr float DIRECTION_CHANGE_TIME = 3.0f;
    static constexpr uint8 DEFAULT_WALK_SPEED = 50;
    static constexpr uint8 CHASE_SPEED = 150;
};
