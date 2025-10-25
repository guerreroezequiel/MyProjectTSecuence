// GridDebugWidgetBase.h
#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "GridDebugWidgetBase.generated.h"

class UImage;

/**
 * Widget base para la visualización de depuración de la grilla
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class MYPROJECTTSECUENCE_API UGridDebugWidgetBase : public UEditorUtilityWidget
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
    UPROPERTY(Transient)
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

    /** Actualiza la visualización */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateGridVisualization();

private:
    /** Delegado para actualizar el render target */
    UFUNCTION()
    void OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height);
};