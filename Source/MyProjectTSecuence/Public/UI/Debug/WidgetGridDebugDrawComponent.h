#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WidgetGridDebugDrawComponent.generated.h"

// Forward declarations
class UWidgetGridDebugSubsystem;

/**
 * Component that handles debug drawing for the widget's grid visualization in the world.
 * Controlled by the GridDebugSubsystem based on widget input.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UWidgetGridDebugDrawComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWidgetGridDebugDrawComponent();

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    //~ End UActorComponent Interface

    /** 
     * Update the debug drawing area based on widget input 
     * @param InCenterWorldLocation World location of the center cell
     * @param InGridSize Size of each grid cell
     * @param InRadius Number of cells to show in each direction from center
     */
    void UpdateDebugArea(const FVector& InCenterWorldLocation, float InGridSize, int32 InRadius);

    /** Clear the current debug area */
    void ClearDebugArea();

    /** Update debug drawing settings */
    void UpdateDebugSettings(bool bInEnabled, const FColor& InColor, float InThickness, float InDuration, uint8 InDepthPriority);

protected:
    /** Draw the debug grid in the world */
    void DrawDebugGrid();

private:
    // Reference to the widget debug subsystem
    UPROPERTY(Transient)
    TObjectPtr<UWidgetGridDebugSubsystem> WidgetDebugSubsystem;

    // Current debug area properties
    FVector CenterWorldLocation;
    float GridSize;
    int32 Radius;
    
    // Debug drawing settings
    bool bIsEnabled;
    FColor DebugColor;
    float LineThickness;
    float DebugDuration;
    uint8 DepthPriority;
    
    // Persistent debug handles
    TArray<int32> DebugLineHandles;
    TArray<int32> DebugBoxHandles;
};
