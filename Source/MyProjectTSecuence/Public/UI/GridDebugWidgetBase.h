// GridDebugWidgetBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Math/IntPoint.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridDebugWidgetBase.generated.h"

class UImage;

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

    /** Actualiza la posición central del grid basada en una posición del mundo */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateFromWorldPosition(const FVector& WorldPosition);

    /** Tamaño de la celda en píxeles (debe coincidir con GridConfig::CellSizeUU) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug", meta = (ClampMin = "1.0"))
    float CellSize = 100.0f;

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
};