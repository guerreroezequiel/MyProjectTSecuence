#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StimulusTypes.h"
#include "PlayerSignalSubsystem.generated.h"

// Forward declarations
class AMyProjectTSecuenceCharacter;
class UStimulusSubsystem;

/**
 * Subsystem para emitir señales del jugador al StimulusSubsystem
 * Ejecuta en Game Thread - emite estímulos del jugador
 * Optimizado para 10,000 entidades
 */
UCLASS()
class MYPROJECTTSECUENCE_API UPlayerSignalSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPlayerSignalSubsystem();

    // Inicialización del subsystem
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;

    // Tick del subsystem - se ejecuta en game thread
    virtual void Tick(float DeltaTime);

    // Métodos para emitir señales del jugador
    UFUNCTION(BlueprintCallable, Category = "Player Signals")
    void EmitPositionSignal();

    UFUNCTION(BlueprintCallable, Category = "Player Signals")
    void EmitMovementSignal();

    UFUNCTION(BlueprintCallable, Category = "Player Signals")
    void EmitSoundSignal(EStimulusType SoundType, uint8 Intensity = 100, float Radius = 500.0f);

    UFUNCTION(BlueprintCallable, Category = "Player Signals")
    void EmitActionSignal(EStimulusType ActionType, uint8 Intensity = 100, float Radius = 300.0f);

private:
    // Referencia al personaje del jugador
    UPROPERTY()
    TObjectPtr<AMyProjectTSecuenceCharacter> PlayerCharacter;

    // Referencia al StimulusSubsystem
    UPROPERTY()
    TObjectPtr<UStimulusSubsystem> StimulusSubsystem;

    // Posición actual del jugador
    FVector CurrentPlayerPosition = FVector::ZeroVector;

    // Posición anterior del jugador
    FVector LastPlayerPosition = FVector::ZeroVector;

    // Flags de estado
    bool bPlayerHasMoved = false;
    bool bSystemInitialized = false;

    // Configuración de señales
    UPROPERTY()
    float PositionUpdateInterval = 0.1f; // 10 FPS para posición

    UPROPERTY()
    float MovementThreshold = 10.0f; // Umbral para detectar movimiento

    // Timers
    float PositionUpdateTimer = 0.0f;

    // Métodos auxiliares
    void UpdatePlayerPosition();
    void DetectPlayerMovement();
    bool IsPlayerValid() const;
    bool IsStimulusSubsystemValid() const;
};
