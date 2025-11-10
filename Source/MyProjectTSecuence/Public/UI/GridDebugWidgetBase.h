// GridDebugWidgetBase.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Math/IntPoint.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridDebugWidgetBase.generated.h"

class UImage;
class UCanvasRenderTarget2D;

USTRUCT(BlueprintType)
struct FGridDebugSettings
{
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    bool bShowPlayerCell = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    FLinearColor PlayerCellColor = FLinearColor::Green;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float PlayerCellOpacity = 0.3f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    bool bShowTileBounds = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    bool bShowCellGrid = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    FLinearColor GridLineColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    float UpdateRate = 0.1f; // Seconds between updates
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerCellChanged, const FIntPoint&, NewCell);

UCLASS(Abstract, Blueprintable, BlueprintType)
class MYPROJECTTSECUENCE_API UGridDebugWidgetBase : public UUserWidget
{
    GENERATED_BODY()

public:
    UGridDebugWidgetBase(const FObjectInitializer& ObjectInitializer);

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // Widget bindings
    UPROPERTY(meta = (BindWidget))
    UImage* GridImage_GridWorld;

    // Debug settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid Debug")
    FGridDebugSettings DebugSettings;
    
    // Event called when player cell changes
    UPROPERTY(BlueprintAssignable, Category = "Grid Debug")
    FOnPlayerCellChanged OnPlayerCellChanged;

    // Update player position (call this from your player/controller)
    UFUNCTION(BlueprintCallable, Category = "Grid Debug")
    void UpdatePlayerPosition(const FVector& WorldLocation);

protected:
    UPROPERTY()
    UCanvasRenderTarget2D* GridRenderTarget = nullptr;
    
    // Cached values for optimization
    FIntPoint CachedPlayerCell;
    FIntPoint LastRenderedPlayerCell;
    int32 LastCellSize = 0;
    float UpdateTimer = 0.0f;
    bool bNeedsUpdate = false;

    // Rendering functions
    void CreateGridRenderTarget();
    void UpdateDebugDisplay();
    void RenderGrid(UCanvas* Canvas, const FVector2D& ImageSize, int32 InCellSize, const FLinearColor& LineColor);
    void RenderTileBounds(UCanvas* Canvas, const FVector2D& ImageSize, int32 CellSize, const FLinearColor& LineColor);
    void RenderPlayerCell(UCanvas* Canvas, int32 CellSize);
    
    /** Handles render target updates */
    UFUNCTION()
    void OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height);

    /** Calculates the optimal cell size based on the current view */
    int32 CalculateOptimalCellSize(int32 CanvasWidth, int32 CanvasHeight) const;
};