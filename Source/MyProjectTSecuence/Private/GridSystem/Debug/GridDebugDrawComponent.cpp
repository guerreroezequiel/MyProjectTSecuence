#include "GridSystem/Debug/GridDebugDrawComponent.h"
#include "DrawDebugHelpers.h"

// Core/grid helpers
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"

// Capas
#include "GridSystem/Occupancy/CapacityGrid.h"

UGridDebugDrawComponent::UGridDebugDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UGridDebugDrawComponent::InitializeComponent()
{
	Super::InitializeComponent();
	UE_LOG(LogTemp, Log, TEXT("GridDebugDrawComponent inicializado. bDrawTileBorder = %s"), 
        bDrawTileBorder ? TEXT("true") : TEXT("false"));
}

void UGridDebugDrawComponent::SetDebugCell(int32 X, int32 Y)
{
    DebugCell = FIntPoint(X, Y);
}

void UGridDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld()) return;

    // Dibujar según los flags activos
    if (bDrawCapacity)
    {
        DrawCapacity();
    }
    
    if (bDrawTileBorder)
    {
        DrawTileBorder();
    }
    
    if (DebugCell.X != INDEX_NONE && DebugCell.Y != INDEX_NONE)
    {
        DrawDebugCell(DebugCell);
    }
}

void UGridDebugDrawComponent::DrawTileBorder()
{
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Warning, TEXT("No valid world in DrawTileBorder"));
        return;
    }

    const FIntPoint TileXY(0, 0); // Tile (0,0) por ahora
    
    // Obtener los límites del tile en celdas
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);
    
    // Convertir las coordenadas de celda a mundo
    const FVector2D MinWorld = GridWorld::CellToWorldCenterXY(MinCell) - FVector2D(GridConfig::CellSizeUU * 0.5f);
    const FVector2D MaxWorld = GridWorld::CellToWorldCenterXY(MaxCell) + FVector2D(GridConfig::CellSizeUU * 0.5f);
    
    const float Z = GridWorld::GetOriginWS().Z + 10.0f; // Pequeño offset Z para evitar z-fighting
    
    // Calcular las esquinas del tile
    const FVector BottomLeft(MinWorld.X, MinWorld.Y, Z);
    const FVector BottomRight(MaxWorld.X, MinWorld.Y, Z);
    const FVector TopLeft(MinWorld.X, MaxWorld.Y, Z);
    const FVector TopRight(MaxWorld.X, MaxWorld.Y, Z);
    
    // Dibujar las 4 líneas del borde con un color más visible
    const FColor BorderColor = FColor::Green;
    const float LineThickness = 5.0f;
    
    DrawDebugLine(GetWorld(), BottomLeft, BottomRight, BorderColor, false, -1.0f, 0, LineThickness);
    DrawDebugLine(GetWorld(), BottomRight, TopRight, BorderColor, false, -1.0f, 0, LineThickness);
    DrawDebugLine(GetWorld(), TopRight, TopLeft, BorderColor, false, -1.0f, 0, LineThickness);
    DrawDebugLine(GetWorld(), TopLeft, BottomLeft, BorderColor, false, -1.0f, 0, LineThickness);
    
    // Dibujar una pequeña cruz en el centro del tile
    const FVector Center = (BottomLeft + TopRight) * 0.5f;
    const float CrossSize = 50.0f; // Tamaño de la cruz en unidades del mundo
    
    DrawDebugLine(GetWorld(), 
                 Center - FVector(CrossSize, 0, 0), 
                 Center + FVector(CrossSize, 0, 0), 
                 FColor::Red, false, -1.0f, 0, LineThickness);
    
    DrawDebugLine(GetWorld(), 
                 Center - FVector(0, CrossSize, 0), 
                 Center + FVector(0, CrossSize, 0), 
                 FColor::Red, false, -1.0f, 0, LineThickness);
    
    // Etiqueta del tile con información de depuración
    const FString TileLabel = FString::Printf(TEXT("Tile (%d,%d)\nCells: (%d,%d)-(%d,%d)"), 
        TileXY.X, TileXY.Y, 
        MinCell.X, MinCell.Y, MaxCell.X, MaxCell.Y);
        
    DrawDebugString(GetWorld(), 
                   Center + FVector(0, 0, 100.0f), 
                   TileLabel, 
                   nullptr, 
                   FColor::Yellow, 
                   0.0f,  // Tiempo de vida (0 = un solo frame)
                   true,  // bDrawShadow
                   1.5f); // Tamaño de la fuente
    
    // Tile border drawing code here - log removed to reduce log spam
}

void UGridDebugDrawComponent::DrawDebugCell(const FIntPoint& CellXY)
{
    const int32 Base = Grid::Capacity::GetBaseCapacity(CellXY);
    const int32 Count = Grid::Capacity::GetCurrentCount(CellXY);

    // Centro en mundo de la celda
    const FVector2D Center2D = GridWorld::CellToWorldCenterXY(CellXY);
    const float Z = GridWorld::GetOriginWS().Z;
    const FVector Center(Center2D.X, Center2D.Y, Z);

    // Color según capacidad
    const bool bOver = (Base > 0) && (Count > Base);
    const FColor Color = bOver ? FColor::Red : FColor::Green;

    // Dibujar caja de la celda
    const float HalfCell = GridConfig::CellSizeUU * 0.5f;
    const float Margin = HalfCell * 0.10f;
    const FVector Extent(HalfCell - Margin, HalfCell - Margin, 2.f);
    
    DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, false, -1.0f, 0, 2.0f);
    
    // Mostrar información de la celda
    const FString Info = FString::Printf(TEXT("(%d,%d)\n%d/%d"), 
        CellXY.X, CellXY.Y, Count, Base);
    DrawDebugString(GetWorld(), Center + FVector(0, 0, TextZOffset), Info, nullptr, FColor::White, 0.f, true, 1.0f);
}

void UGridDebugDrawComponent::DrawCapacity()
{
    // Por simplicidad, solo dibujamos el tile (0,0)
    const FIntPoint TileXY(0, 0);
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    for (int32 y = MinCell.Y; y <= MaxCell.Y; ++y)
    {
        for (int32 x = MinCell.X; x <= MaxCell.X; ++x)
        {
            const FIntPoint CellXY(x, y);
            const int32 Base = Grid::Capacity::GetBaseCapacity(CellXY);
            const int32 Count = Grid::Capacity::GetCurrentCount(CellXY);

            // Solo dibujar celdas con datos
            if (Base > 0 || Count > 0)
            {
                const FVector2D Center2D = GridWorld::CellToWorldCenterXY(CellXY);
                const float Z = GridWorld::GetOriginWS().Z;
                const FVector Center(Center2D.X, Center2D.Y, Z);

                const bool bOver = (Base > 0) && (Count > Base);
                const FColor Color = bOver ? FColor::Red : FColor::Green;

                const float HalfCell = GridConfig::CellSizeUU * 0.5f;
                const FVector Extent(HalfCell * 0.9f, HalfCell * 0.9f, 1.0f);
                DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, false, -1.0f, 0, 1.0f);
            }
        }
    }
}
