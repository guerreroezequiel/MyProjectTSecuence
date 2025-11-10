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

protected:
    /** Referencia al Render Target */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    TObjectPtr<UCanvasRenderTarget2D> GridRenderTarget;

    /** Color de las líneas de la grilla */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Debug")
    FLinearColor GridLineColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);

    /** Referencia al Image que mostrará la grilla del mundo */
    UPROPERTY(BlueprintReadOnly, Category = "Grid|Debug", meta = (BindWidget))
    TObjectPtr<UImage> GridImage_GridWorld;

    /** Crea el render target */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void CreateGridRenderTarget();

    /** Actualiza la visualización de la grilla */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug")
    void UpdateGridVisualization();

private:
    /** Obtiene el color de una celda específica */
    FLinearColor GetCellColor_Implementation(const FIntPoint& CellCoord) const;
    
    /** Renderiza una grilla en el canvas proporcionado */
    void RenderGrid(UCanvas* Canvas, const FVector2D& ImageSize, int32 InCellSize, const FLinearColor& LineColor);
    
    /** Maneja la actualización del render target */
    UFUNCTION()
    void OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height);
};