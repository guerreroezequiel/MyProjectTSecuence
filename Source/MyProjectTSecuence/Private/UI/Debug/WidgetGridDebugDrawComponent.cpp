#include "UI/Debug/WidgetGridDebugDrawComponent.h"
#include "GridSystem/Debug/GridDebugSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UWidgetGridDebugDrawComponent::UWidgetGridDebugDrawComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    
    // Default values
    GridSize = 100.0f;
    Radius = 5;
    bIsEnabled = true;
    DebugColor = FColor::Green;
    LineThickness = 2.0f;
    DebugDuration = -1.0f; // Persistent
    DepthPriority = 0;
}

void UWidgetGridDebugDrawComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get the debug subsystem
    DebugSubsystem = GetWorld()->GetSubsystem<UGridDebugSubsystem>();
    if (DebugSubsystem)
    {
        DebugSubsystem->RegisterWidgetDebugComponent(this);
    }
}

void UWidgetGridDebugDrawComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear any active debug drawings
    ClearDebugArea();
    
    // Unregister from the debug subsystem
    if (DebugSubsystem)
    {
        DebugSubsystem->UnregisterWidgetDebugComponent(this);
    }
    
    Super::EndPlay(EndPlayReason);
}

void UWidgetGridDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (bIsEnabled)
    {
        DrawDebugGrid();
    }
}

void UWidgetGridDebugDrawComponent::UpdateDebugArea(const FVector& InCenterWorldLocation, float InGridSize, int32 InRadius)
{
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugDrawComponent: UpdateDebugArea - Location: %s, GridSize: %.2f, Radius: %d"), 
        *InCenterWorldLocation.ToString(), InGridSize, InRadius);
        
    CenterWorldLocation = InCenterWorldLocation;
    GridSize = FMath::Max(1.0f, InGridSize);
    Radius = FMath::Max(0, InRadius);
    
    // Clear any previous debug drawings
    ClearDebugArea();
    
    // Immediately draw the new grid if enabled
    if (bIsEnabled)
    {
        UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugDrawComponent: Drawing debug grid immediately"));
        DrawDebugGrid();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetGridDebugDrawComponent: Debug drawing is disabled, not drawing grid"));
    }
}

void UWidgetGridDebugDrawComponent::ClearDebugArea()
{
    // No need to manually clear debug drawings as we're using persistent debug lines
    // The engine will handle cleaning them up when the component is destroyed
    DebugLineHandles.Reset();
    DebugBoxHandles.Reset();
}

void UWidgetGridDebugDrawComponent::UpdateDebugSettings(bool bInEnabled, const FColor& InColor, float InThickness, float InDuration, uint8 InDepthPriority)
{
    bIsEnabled = bInEnabled;
    DebugColor = InColor;
    LineThickness = FMath::Max(0.1f, InThickness);
    DebugDuration = InDuration;
    DepthPriority = InDepthPriority;
}

void UWidgetGridDebugDrawComponent::DrawDebugGrid()
{
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("WidgetGridDebugDrawComponent: No valid world to draw in"));
        return;
    }
    
    if (Radius <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetGridDebugDrawComponent: Invalid radius: %d"), Radius);
        return;
    }
    
    if (GridSize <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetGridDebugDrawComponent: Invalid grid size: %.2f"), GridSize);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugDrawComponent: Drawing grid at %s with size %.2f and radius %d"), 
        *CenterWorldLocation.ToString(), GridSize, Radius);
    
    const FVector Center = CenterWorldLocation;
    const float HalfGrid = GridSize * 0.5f;
    const int32 CellCount = (Radius * 2) + 1;
    
    // Draw grid lines
    for (int32 X = -Radius; X <= Radius + 1; ++X)
    {
        const float XPos = Center.X + (X * GridSize);
        const FVector Start = FVector(XPos, Center.Y - (Radius * GridSize), Center.Z);
        const FVector End = FVector(XPos, Center.Y + ((Radius + 1) * GridSize), Center.Z);
        
        int32 LineHandle = -1;
        DrawDebugLine(GetWorld(), Start, End, DebugColor, false, DebugDuration, DepthPriority, LineThickness);
    }
    
    for (int32 Y = -Radius; Y <= Radius + 1; ++Y)
    {
        const float YPos = Center.Y + (Y * GridSize);
        const FVector Start = FVector(Center.X - (Radius * GridSize), YPos, Center.Z);
        const FVector End = FVector(Center.X + ((Radius + 1) * GridSize), YPos, Center.Z);
        
        int32 LineHandle = -1;
        DrawDebugLine(GetWorld(), Start, End, DebugColor, false, DebugDuration, DepthPriority, LineThickness);
    }
    
    // Draw center cell highlight
    const FVector BoxExtent = FVector(HalfGrid, HalfGrid, 10.0f);
    DrawDebugBox(GetWorld(), Center, BoxExtent, FQuat::Identity, FColor::Yellow, false, DebugDuration, DepthPriority, 1.0f);
}
