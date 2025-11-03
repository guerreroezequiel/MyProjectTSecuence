// GridDebugWidgetBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Math/IntPoint.h"

// Forward declarations
class UImage;
class UWidgetGridDebugSubsystem;

#include "GridDebugWidgetBase.generated.h"

/**
 * Widget base para la visualización de depuración de la grilla
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class MYPROJECTTSECUENCE_API UGridDebugWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UGridDebugWidgetBase(const FObjectInitializer& ObjectInitializer);

    //~ Begin UEditorUtilityWidget Interface
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    //~ End UEditorUtilityWidget Interface

    /** Obtiene las dimensiones de la grilla */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Debug")
    FIntPoint GetGridDimensions() const;

    /** Obtiene el color de una celda específica */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Grid|Debug")
    FLinearColor GetCellColor(const FIntPoint& CellCoord) const;

protected:
    /** Referencia al Render Target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    TObjectPtr<UCanvasRenderTarget2D> GridRenderTarget;

    /** Color de las líneas de la grilla */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    FLinearColor GridLineColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);

    /** Referencia al Image que mostrará la grilla */
    UPROPERTY(BlueprintReadOnly, Category = "Grid|Debug", meta = (BindWidget))
    TObjectPtr<UImage> GridImage;

    /** Crea el render target */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void CreateGridRenderTarget();

    /** Actualiza la visualización de la grilla */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateGridVisualization();

    /** Obtiene las coordenadas del mundo de la grilla para una coordenada de widget */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    FIntPoint GetWorldGridPosition(const FIntPoint& WidgetGridPosition) const;

    /** Obtiene las coordenadas del widget para una coordenada del mundo de la grilla */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    FIntPoint GetWidgetGridPosition(const FIntPoint& WorldGridPosition) const;
    
    /**
     * Updates the debug visualization for the specified world location
     * @param WorldLocation The world location to debug
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void UpdateDebugVisualization(const FVector& WorldLocation);
    
    /**
     * Updates the debug visualization using grid coordinates
     * @param CellCoord The grid coordinates to debug
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void UpdateDebugVisualizationFromCoord(const FIntPoint& CellCoord);
    
    /**
     * Clears the current debug visualization
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void ClearDebugVisualization();

    /** Tamaño de la celda en píxeles */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    float CellSize = 50.0f;

    /** Radio de la grilla (0 = solo la celda central, 1 = 3x3, 2 = 5x5, etc.) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug", meta = (ClampMin = "0", ClampMax = "31", UIMin = "0", UIMax = "31"))
    int32 GridRadius = 0;

    /** Coordenada X de la celda central */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    int32 CenterCellX = 0;

    /** Coordenada Y de la celda central */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    int32 CenterCellY = 0;

private:
    /** Delegado para actualizar el render target */
    UFUNCTION()
    void OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height);
    
    /** Reference to the widget debug subsystem */
    UPROPERTY(Transient)
    TObjectPtr<UWidgetGridDebugSubsystem> WidgetDebugSubsystem;
};