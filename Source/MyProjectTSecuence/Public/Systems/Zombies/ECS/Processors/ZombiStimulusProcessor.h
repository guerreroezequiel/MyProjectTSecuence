#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "Systems/StimulusSubsystem/StimulusTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "ZombiStimulusProcessor.generated.h"

// Forward declarations
class UStimulusSubsystem;

/**
 * Processor para procesar estímulos en todos los zombis
 * Ejecuta en Batch Processing - distribuye estímulos a zombis
 * Optimizado para 10,000 entidades
 * ORDEN CRÍTICO: Se ejecuta antes de BehaviorProcessor
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiStimulusProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiStimulusProcessor();

protected:
    // Configuración del processor
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // Query para zombis que pueden recibir estímulos
    FMassEntityQuery StimulusQuery;

    // Referencia al StimulusSubsystem (cacheada en game thread)
    UPROPERTY()
    TObjectPtr<UStimulusSubsystem> StimulusSubsystem;

    // Cache de estímulos activos (actualizado en game thread)
    TArray<FStimulusData> CachedActiveStimuli;

    // Configuración de procesamiento
    UPROPERTY()
    float StimulusDetectionRange = 1000.0f; // Rango de detección de estímulos

    UPROPERTY()
    float StimulusUpdateInterval = 0.1f; // Intervalo de actualización de estímulos

    // Timer para actualizar cache
    float CacheUpdateTimer = 0.0f;

    // Métodos auxiliares
    void UpdateStimulusCache(float DeltaTime);
    void ProcessStimuliForZombie(const FVector &ZombiePosition, FZombiStimuliFragment &StimuliFragment);
    bool IsStimulusInRange(const FVector &ZombiePosition, const FStimulusData &Stimulus) const;
    void UpdateResponseTimers(FZombiStimuliFragment &StimuliFragment, float DeltaTime);
};
