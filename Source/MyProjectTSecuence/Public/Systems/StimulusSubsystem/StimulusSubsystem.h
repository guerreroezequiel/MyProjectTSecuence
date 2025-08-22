#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StimulusTypes.h"
#include "Engine/Engine.h"
#include "StimulusSubsystem.generated.h"

/**
 * Subsystem central para manejar todos los estímulos sensoriales
 * Ejecuta en Game Thread - pre-procesa estímulos para batch processing
 * Optimizado para 10,000 entidades y escalable para futuros estímulos
 */
UCLASS()
class MYPROJECTTSECUENCE_API UStimulusSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    UStimulusSubsystem();

    // Inicialización del subsystem
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;

    // Tick del subsystem - se ejecuta en game thread
    virtual void Tick(float DeltaTime) override;

    // FTickableGameObject interface
    virtual bool IsTickable() const override;
    virtual bool IsTickableInEditor() const override;
    virtual TStatId GetStatId() const override;

    // Métodos para agregar estímulos (escalables para futuras fuentes)
    UFUNCTION(BlueprintCallable, Category = "Stimulus System")
    void AddPlayerStimulus(const FVector &Position, const FVector &Direction,
                           EStimulusType Type, uint8 Intensity, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Stimulus System")
    void AddItemStimulus(const FVector &Position, const FVector &Direction,
                         EStimulusType Type, uint8 Intensity, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Stimulus System")
    void AddAnimalStimulus(const FVector &Position, const FVector &Direction,
                           EStimulusType Type, uint8 Intensity, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Stimulus System")
    void AddEnvironmentStimulus(const FVector &Position, const FVector &Direction,
                                EStimulusType Type, uint8 Intensity, float Radius);

    UFUNCTION(BlueprintCallable, Category = "Stimulus System")
    void AddCustomStimulus(const FVector &Position, const FVector &Direction,
                           EStimulusType Type, EStimulusSource Source,
                           uint8 Intensity, float Radius);

    // Método genérico para cualquier estímulo
    void AddStimulus(const FStimulusData &Stimulus);

    // Métodos para obtener datos de estímulos (para batch processing)
    const TArray<FStimulusData> &GetActiveStimuli() const { return ActiveStimuli; }
    const TArray<FStimulusData> &GetPlayerStimuli() const { return PlayerStimuli; }
    const TArray<FStimulusData> &GetEnvironmentStimuli() const { return EnvironmentStimuli; }

    // Acceso rápido al último estímulo del jugador (vía rápida)
    bool TryGetLatestPlayerStimulus(FStimulusData &OutStimulus) const;

    // Limpiar estímulos expirados
    void CleanupExpiredStimuli();

    // Métodos de utilidad
    TArray<FStimulusData> GetStimuliInRange(const FVector &Position, float Range) const;
    TArray<FStimulusData> GetStimuliByType(EStimulusType Type) const;
    TArray<FStimulusData> GetStimuliBySource(EStimulusSource Source) const;

private:
    // Estímulos activos organizados por categoría
    UPROPERTY()
    TArray<FStimulusData> ActiveStimuli;

    UPROPERTY()
    TArray<FStimulusData> PlayerStimuli;

    UPROPERTY()
    TArray<FStimulusData> EnvironmentStimuli;

    // Cache del último estímulo del jugador (vía rápida)
    UPROPERTY()
    FStimulusData LatestPlayerStimulus;
    bool bHasLatestPlayerStimulus = false;

    // Grid espacial para optimizar queries (NUEVO)
    UPROPERTY()
    FStimulusGrid StimulusGrid;

    // Flags de estado
    bool bSystemInitialized = false;

    // Configuración de estímulos
    UPROPERTY()
    float StimulusDecayRate = 1.0f; // Velocidad de decaimiento

    UPROPERTY()
    float CleanupInterval = 1.0f; // Intervalo de limpieza

    UPROPERTY()
    int32 MaxActiveStimuli = 1000; // Límite de estímulos activos

    // Timers
    float CleanupTimer = 0.0f;

    // Métodos auxiliares
    void UpdateStimulusDecay(float DeltaTime);
    void OrganizeStimuli();
    void LimitActiveStimuli();
    bool IsStimulusValid(const FStimulusData &Stimulus) const;

    void UpdateLatestPlayerStimulus(const FStimulusData &Stimulus);
};
