// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "FlowFieldSystem.generated.h"

// Forward declarations
class FTileContext;
class FFlowField;

UCLASS(ClassGroup=("FlowField"), meta=(BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UFlowFieldSystem : public UActorComponent
{
    GENERATED_BODY()

public:    
    UFlowFieldSystem();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Debug
    UFUNCTION(BlueprintCallable, Category = "FlowField|Debug")
    void ToggleDebugVisualization(bool bShow);

private:
    // Array de tiles (simplificado)
    TArray<TSharedPtr<class FTileContext>> Tiles;
    
    // FlowFields por intención
    TMap<EFlowIntent, TSharedPtr<class FFlowField>> FlowFields;

    // Configuración
    UPROPERTY(EditAnywhere, Category = "FlowField")
    bool bAutoUpdate = true;

    // Debug
    UPROPERTY(EditAnywhere, Category = "FlowField|Debug")
    bool bDebugVisualization = false;

    void RebuildFlowField(const TSharedPtr<FTileContext>& Tile, EFlowIntent Intent);
    void DrawDebugInfo() const;
    void InitializeWorldTiles();
    void UpdateHotWarmFromPlayer();
    void RecomputeWarmAroundHot(const FIntPoint& NewHot);
    FIntPoint CurrentHotTileXY = FIntPoint(-1, -1);
    FIntPoint CurrentPlayerCellXY = FIntPoint(-1, -1);
    TSet<FIntPoint> HotTiles;
    TSet<FIntPoint> WarmTiles;
    TSet<FIntPoint> ActiveTiles;
};
