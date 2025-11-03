// WidgetGridDebugSubsystem.cpp
#include "UI/Debug/WidgetGridDebugSubsystem.h"
#include "UI/Debug/WidgetGridDebugDrawComponent.h"
#include "Engine/World.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"

UWidgetGridDebugSubsystem* UWidgetGridDebugSubsystem::GetWidgetGridDebugSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UWidgetGridDebugSubsystem>();
}

void UWidgetGridDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Initialize default values
    CurrentCellLocation = FVector::ZeroVector;
    CurrentGridSize = 100.0f;
    CurrentRadius = 5;
    
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem initialized"));
}

void UWidgetGridDebugSubsystem::Deinitialize()
{
    // Clear all debug visualizations
    ClearDebugVisualization();
    
    // Clear the components array
    DebugComponents.Empty();
    
    Super::Deinitialize();
}

void UWidgetGridDebugSubsystem::RegisterDebugComponent(UWidgetGridDebugDrawComponent* Component)
{
    if (Component && !DebugComponents.Contains(Component))
    {
        UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem - Registrando nuevo componente: %s"), 
            *GetNameSafe(Component->GetOwner()));
            
        DebugComponents.Add(Component);
        
        // Update the component with current debug settings
        UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem - Actualizando componente con pos: %s, tamaño: %.2f, radio: %d"), 
            *CurrentCellLocation.ToString(), CurrentGridSize, CurrentRadius);
            
        Component->UpdateDebugArea(CurrentCellLocation, CurrentGridSize, CurrentRadius);
    }
    else if (!Component)
    {
        UE_LOG(LogTemp, Warning, TEXT("WidgetGridDebugSubsystem - Intento de registrar un componente nulo"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem - El componente ya está registrado"));
    }
}

void UWidgetGridDebugSubsystem::UnregisterDebugComponent(UWidgetGridDebugDrawComponent* Component)
{
    if (Component)
    {
        DebugComponents.Remove(Component);
    }
}

void UWidgetGridDebugSubsystem::UpdateDebugVisualization(const FVector& CellWorldLocation, float GridSize, int32 Radius)
{
    // Update current settings
    CurrentCellLocation = CellWorldLocation;
    CurrentGridSize = GridSize;
    CurrentRadius = FMath::Max(0, Radius);
    
    // Update all registered components
    for (UWidgetGridDebugDrawComponent* Component : DebugComponents)
    {
        if (Component)
        {
            Component->UpdateDebugArea(CurrentCellLocation, CurrentGridSize, CurrentRadius);
        }
    }
    
    // Broadcast the cell selection event
    OnCellSelected.Broadcast(CurrentCellLocation);
}

void UWidgetGridDebugSubsystem::UpdateDebugVisualizationFromCoord(const FIntPoint& CellCoord, float GridSize, int32 Radius)
{
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem::UpdateDebugVisualizationFromCoord - Celda: (%d, %d), Tamaño: %.2f, Radio: %d"), 
        CellCoord.X, CellCoord.Y, GridSize, Radius);
    
    // Convert grid coordinates to world position
    const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(CellCoord);
    FVector WorldLocation(WorldPos2D.X, WorldPos2D.Y, 0.0f);
    
    UE_LOG(LogTemp, Log, TEXT("WidgetGridDebugSubsystem - Posición en mundo: %s"), *WorldLocation.ToString());
    
    // Update the visualization with the calculated world position
    UpdateDebugVisualization(WorldLocation, GridSize, Radius);
}

void UWidgetGridDebugSubsystem::ClearDebugVisualization()
{
    // Clear all registered components
    for (UWidgetGridDebugDrawComponent* Component : DebugComponents)
    {
        if (Component)
        {
            Component->ClearDebugArea();
        }
    }
    
    // Reset current location
    CurrentCellLocation = FVector::ZeroVector;
}
