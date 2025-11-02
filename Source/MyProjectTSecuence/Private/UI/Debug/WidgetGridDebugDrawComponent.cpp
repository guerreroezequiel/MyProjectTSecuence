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
    // Validate input parameters
    if (InGridSize <= 0.0f || InRadius < 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetGridDebugDrawComponent: Invalid parameters - GridSize: %.2f, Radius: %d"), 
            InGridSize, InRadius);
        return;
    }
    
    // Update component properties
    CenterWorldLocation = InCenterWorldLocation;
    GridSize = InGridSize;
    Radius = InRadius;
    
    // Calculate the total number of cells in the grid (including center and all neighbors within radius)
    const int32 GridWidth = (Radius * 2) + 1;  // Total cells in one dimension
    const int32 TotalCells = GridWidth * GridWidth;
    
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugDrawComponent: UpdateDebugArea - Center: %s, CellSize: %.2f, Radius: %d, TotalCells: %d"), 
        *InCenterWorldLocation.ToString(), GridSize, Radius, TotalCells);
    
    // Clear any previous debug drawings
    ClearDebugArea();
    
    // If radius is 0, we're just drawing a single cell
    if (Radius == 0)
    {
        UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugDrawComponent: Drawing single cell at %s"), 
            *CenterWorldLocation.ToString());
    }
    
    // Immediately draw the new grid if enabled
    if (bIsEnabled)
    {
        DrawDebugGrid();
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
    if (!GetWorld() || GridSize <= 0.0f)
    {
        return;
    }
    
    const FVector Center = CenterWorldLocation;
    const float HalfGrid = GridSize * 0.5f;
    const int32 CellCount = (Radius * 2) + 1;
    
    // Calculate the total grid size in world units
    const float TotalGridSize = CellCount * GridSize;
    const FVector GridOrigin = Center - FVector(Radius * GridSize, Radius * GridSize, 0);
    
    // Draw vertical grid lines
    for (int32 X = 0; X <= CellCount; ++X)
    {
        const float XPos = GridOrigin.X + (X * GridSize);
        const FVector Start = FVector(XPos, GridOrigin.Y, Center.Z);
        const FVector End = FVector(XPos, GridOrigin.Y + TotalGridSize, Center.Z);
        
        DrawDebugLine(GetWorld(), Start, End, DebugColor, false, DebugDuration, DepthPriority, LineThickness);
    }
    
    // Draw horizontal grid lines
    for (int32 Y = 0; Y <= CellCount; ++Y)
    {
        const float YPos = GridOrigin.Y + (Y * GridSize);
        const FVector Start = FVector(GridOrigin.X, YPos, Center.Z);
        const FVector End = FVector(GridOrigin.X + TotalGridSize, YPos, Center.Z);
        
        DrawDebugLine(GetWorld(), Start, End, DebugColor, false, DebugDuration, DepthPriority, LineThickness);
    }
    
    // Draw center cell highlight
    const FVector BoxExtent = FVector(HalfGrid, HalfGrid, 10.0f);
    DrawDebugBox(GetWorld(), Center, BoxExtent, FQuat::Identity, FColor::Yellow, false, DebugDuration, DepthPriority, 1.0f);
    
    // Draw cell coordinates for debugging
    if (Radius <= 3)  // Only draw coordinates for small radii to avoid clutter
    {
        for (int32 Y = 0; Y < CellCount; ++Y)
        {
            for (int32 X = 0; X < CellCount; ++X)
            {
                const FVector CellCenter = GridOrigin + FVector(X * GridSize + HalfGrid, Y * GridSize + HalfGrid, 0);
                const FString CoordString = FString::Printf(TEXT("(%d,%d)"), X - Radius, Y - Radius);
                DrawDebugString(GetWorld(), CellCenter, CoordString, nullptr, FColor::White, 0.0f, true);
            }
        }
    }
}
