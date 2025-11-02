// GridDebugSubsystem.h
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridSystem/Core/GridEpoch.h"
#include "GridSystem/Core/GridWorld.h"
#include "DrawDebugHelpers.h"
// Forward declarations to resolve circular dependencies
class UGridDebugDrawComponent;
class UWidgetGridDebugDrawComponent;

#include "GridDebugSubsystem.generated.h"

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

    /** Register/Unregister debug components */
    void RegisterDebugComponent(UGridDebugDrawComponent* Component);
    void UnregisterDebugComponent(UGridDebugDrawComponent* Component);
    
    /** Register/Unregister widget debug components */
    void RegisterWidgetDebugComponent(UWidgetGridDebugDrawComponent* Component);
    void UnregisterWidgetDebugComponent(UWidgetGridDebugDrawComponent* Component);

    /** Update all registered debug components */
    void UpdateAllDebugComponents();
    
    /** 
     * Update the debug area for all widget debug components
     * @param CenterWorldLocation World location of the center cell
     * @param GridSize Size of each grid cell
     * @param Radius Number of cells to show in each direction from center
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void UpdateWidgetDebugArea(const FVector& CenterWorldLocation, float GridSize, int32 Radius);
    
    /** Clear all widget debug areas */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void ClearWidgetDebugAreas();

    // Debug Drawing Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual")
    bool bEnableDebugDrawing = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual")
    bool bDrawCapacity = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual")
    bool bDrawTileOutline = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual", meta = (ClampMin = "1", UIMin = "1"))
    int32 GridStep = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual")
    float BoxExtent = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug|Visual")
    float TextZOffset = 30.0f;

    // Usar el sistema de coordenadas de GridWorld
    UPROPERTY(EditAnywhere, Category = "Grid|Debug")
    bool bUseGridWorldOrigin = true;

protected:
    // Callback para el tick del epoch
    bool OnEpochTick(float DeltaTime);

private:
    // Estado del modo de depuración
    bool bIsDebugActive = false;
    
    // Área de debug actual
    FGridDebugArea CurrentDebugArea;
    bool bHasDebugArea = false;
    
    // Active debug components
    TArray<TWeakObjectPtr<UGridDebugDrawComponent>> DebugComponents;
    TArray<TWeakObjectPtr<UWidgetGridDebugDrawComponent>> WidgetDebugComponents;

    // Estado de pausa
    bool bIsPaused = false;
    
    // Bandera para avanzar un solo paso
    bool bStepRequested = false;
    
    // Manejador del ticker
    FTSTicker::FDelegateHandle EpochTickHandle;
};