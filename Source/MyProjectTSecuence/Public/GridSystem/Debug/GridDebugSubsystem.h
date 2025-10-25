// GridDebugSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridSystem/Core/GridEpoch.h"
#include "GridDebugSubsystem.generated.h"

/**
 * Subsistema de depuración para el sistema de grilla
 * Se integra con el GridEpochSubsystem para la sincronización de actualizaciones
 */
UCLASS()
class MYPROJECTTSECUENCE_API UGridDebugSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    //~ End USubsystem Interface

    /**
     * Obtiene la instancia del subsistema para el mundo actual
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug", meta = (WorldContext = "WorldContextObject"))
    static UGridDebugSubsystem* GetGridDebugSubsystem(const UObject* WorldContextObject);

    /**
     * Activa/desactiva el modo de depuración
     * @param bEnable True para activar, false para desactivar
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void SetDebugMode(bool bEnable);

    /**
     * Pausa/reanuda la simulación de depuración
     * @param bPause True para pausar, false para reanudar
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void SetPauseState(bool bPause);

    /**
     * Avanza un paso de simulación cuando está en pausa
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void StepSimulation();

    /**
     * Verifica si el modo de depuración está activo
     * @return True si el modo de depuración está activo
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid|Debug")
    bool IsDebugActive() const { return bIsDebugActive; }

    /**
     * Verifica si la simulación está en pausa
     * @return True si la simulación está pausada
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Grid|Debug")
    bool IsPaused() const { return bIsPaused; }

protected:
    // Callback para el tick del epoch
    bool OnEpochTick(float DeltaTime);

private:
    // Estado del modo de depuración
    bool bIsDebugActive = false;
    
    // Estado de pausa
    bool bIsPaused = false;
    
    // Bandera para avanzar un solo paso
    bool bStepRequested = false;
    
    // Manejador del ticker
    FTSTicker::FDelegateHandle EpochTickHandle;
};