// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "WidgetGridDebugSubsystem.generated.h"

// Forward declarations
class UWidgetGridDebugDrawComponent;

// Delegate for cell selection events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCellSelected, const FVector&, CellWorldLocation);

/**
 * Subsystem that manages widget-based grid debugging visualization
 */
UCLASS(Blueprintable, BlueprintType)
class MYPROJECTTSECUENCE_API UWidgetGridDebugSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

    /**
     * Gets the instance of the WidgetGridDebugSubsystem for the current world
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget", meta = (WorldContext = "WorldContextObject"))
    static UWidgetGridDebugSubsystem* GetWidgetGridDebugSubsystem(const UObject* WorldContextObject);

    /**
     * Registers a debug component with the subsystem
     */
    void RegisterDebugComponent(UWidgetGridDebugDrawComponent* Component);
    
    /**
     * Unregisters a debug component from the subsystem
     */
    void UnregisterDebugComponent(UWidgetGridDebugDrawComponent* Component);

    /**
     * Updates the debug visualization for a selected cell using world location
     * @param CellWorldLocation World location of the selected cell
     * @param GridSize Size of each grid cell
     * @param Radius Number of cells to show around the selected cell
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void UpdateDebugVisualization(const FVector& CellWorldLocation, float GridSize, int32 Radius);
    
    /**
     * Updates the debug visualization for a selected cell using grid coordinates
     * @param CellCoord Grid coordinates of the selected cell
     * @param GridSize Size of each grid cell
     * @param Radius Number of cells to show around the selected cell
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void UpdateDebugVisualizationFromCoord(const FIntPoint& CellCoord, float GridSize, int32 Radius);

    /**
     * Clears all debug visualizations
     */
    UFUNCTION(BlueprintCallable, Category = "Grid|Debug|Widget")
    void ClearDebugVisualization();

    /** Event triggered when a cell is selected for debugging */
    UPROPERTY(BlueprintAssignable, Category = "Grid|Debug|Widget")
    FOnCellSelected OnCellSelected;

private:
    /** Array of registered debug components */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UWidgetGridDebugDrawComponent>> DebugComponents;

    /** Current debug visualization settings */
    FVector CurrentCellLocation;
    float CurrentGridSize;
    int32 CurrentRadius;
};
