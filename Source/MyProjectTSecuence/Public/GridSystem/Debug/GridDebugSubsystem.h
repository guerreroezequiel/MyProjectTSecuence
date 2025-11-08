// GridDebugSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridSystem/Core/GridEpoch.h"
#include "GridSystem/Core/GridWorld.h"
#include "DrawDebugHelpers.h"
#include "Delegates/Delegate.h"

#include "GridDebugSubsystem.generated.h"

// Forward declarations
class UGridDebugWidgetBase;

/**
 * Estructura que contiene la información del área de debug
 */
USTRUCT(BlueprintType)
struct FGridDebugArea
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Debug")
    FVector CenterLocation;

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Debug")
    FVector Extent;

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Debug")
    FColor Color = FColor::Green;

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Debug")
    float LineThickness = 2.0f;

    UPROPERTY(BlueprintReadWrite, Category = "Grid|Debug")
    float Duration = 0.0f; // 0 = solo un frame
};

/**
 * Subsistema de depuración para el sistema de grilla
 * Se integra con el GridEpochSubsystem para la sincronización de actualizaciones
 */
UCLASS(Blueprintable, BlueprintType)
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
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    bool IsPaused() const { return bIsPaused; }

    /**
     * Actualiza el área de debug que se dibujará en el mundo
     * @param Center Celda central del área
     * @param Radius Radio del área en celdas
     * @param CellSize Tamaño de cada celda
     * @param Color Color del debug
     * @param Duration Duración del debug (0 = solo un frame)
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateDebugArea(const FIntPoint& Center, int32 Radius, float CellSize, FLinearColor Color = FLinearColor::Green, float Duration = 0.0f);
    
    /**
     * Limpia el área de debug actual
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void ClearDebugArea();
    
    /**
     * Actualiza todos los componentes de depuración con la nueva ubicación de la cámara
     * @param CenterWorldLocation Ubicación central en el mundo
     * @param GridSize Tamaño de la cuadrícula
     * @param Radius Radio de visualización
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateAllDebugComponents(const FVector& CenterWorldLocation, float GridSize, int32 Radius);

    // Delegates para notificar cambios a los widgets suscritos
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDebugModeChanged, bool, bIsEnabled);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPauseStateChanged, bool, bIsPaused);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDebugAreaUpdated, const FGridDebugArea&, DebugArea);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDebugAreaCleared);
    
    /** Evento que se dispara cuando cambia el estado del modo de depuración */
    UPROPERTY(BlueprintAssignable, Category = "Grid|Debug|Events")
    FOnDebugModeChanged OnDebugModeChanged;
    
    /** Evento que se dispara cuando cambia el estado de pausa */
    UPROPERTY(BlueprintAssignable, Category = "Grid|Debug|Events")
    FOnPauseStateChanged OnPauseStateChanged;
    
    /** Evento que se dispara cuando se actualiza el área de depuración */
    UPROPERTY(BlueprintAssignable, Category = "Grid|Debug|Events")
    FOnDebugAreaUpdated OnDebugAreaUpdated;
    
    /** Evento que se dispara cuando se limpia el área de depuración */
    UPROPERTY(BlueprintAssignable, Category = "Grid|Debug|Events")
    FOnDebugAreaCleared OnDebugAreaCleared;

    // Configuración de depuración
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    bool bEnableDebugDrawing = true;

    /**
     * Registra un objeto para recibir actualizaciones de depuración
     * @param Subscriber Objeto que se suscribirá a las actualizaciones
     */
    template<typename T>
    void RegisterDebugSubscriber(T* Subscriber);

    /**
     * Elimina un objeto de la lista de suscriptores
     * @param Subscriber Objeto que ya no desea recibir actualizaciones
     */
    template<typename T>
    void UnregisterDebugSubscriber(T* Subscriber);

private:
    // Lista de objetos suscritos a las actualizaciones de depuración
    UPROPERTY(Transient)
    TArray<TWeakObjectPtr<UObject>> DebugSubscribers;

protected:
    // Callback para el tick del epoch
    bool OnEpochTick(float DeltaTime);

private:
    // Estado del modo de depuración
    bool bIsDebugActive = false;
    
    // Área de debug actual
    FGridDebugArea CurrentDebugArea;
    bool bHasDebugArea = false;
    
    // Estado de pausa
    bool bIsPaused = false;
    
    // Bandera para avanzar un solo paso
    bool bStepRequested = false;
    
    // Manejador del ticker
    FTSTicker::FDelegateHandle EpochTickHandle;
};