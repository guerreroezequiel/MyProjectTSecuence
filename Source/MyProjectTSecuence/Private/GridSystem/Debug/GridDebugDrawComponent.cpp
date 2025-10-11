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
}

void UGridDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld()) return;
	// Sanidad
	GridStep = FMath::Max(1, GridStep);

	if (bDrawCapacity)
	{
		DrawCapacity();
	}

}

void UGridDebugDrawComponent::DrawCapacity()
{
    // Por simplicidad inicial, iteramos el tile (0,0). Luego: tiles visibles o definidos por CVar.
    const FIntPoint TileXY(0, 0);

    // Determinar bounds del tile en coords de celda [min,max] (incluyentes)
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    // Dibuja el contorno del tile completo (64x64 celdas) para verificar tamaño
    {
        const FVector2D TileOrigin2D = GridWorld::TileToWorldOriginXY(TileXY);
        const float TileSizeUU = GridConfig::TileDim * GridConfig::CellSizeUU;
        const FVector TileCenter(TileOrigin2D.X + TileSizeUU * 0.5f, TileOrigin2D.Y + TileSizeUU * 0.5f, GridWorld::GetOriginWS().Z);
        const FVector TileExtent(TileSizeUU * 0.5f, TileSizeUU * 0.5f, 2.f);
        DrawDebugBox(GetWorld(), TileCenter, TileExtent, FQuat::Identity, FColor(0, 255, 0, 64), /*bPersistentLines*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 2.f);
    }

    for (int32 y = MinCell.Y; y <= MaxCell.Y; ++y)
    {
        for (int32 x = MinCell.X; x <= MaxCell.X; ++x)
        {
            const FIntPoint CellXY(x, y);

            const int32 Base = Grid::Capacity::GetBaseCapacity(CellXY);
            const int32 Count = Grid::Capacity::GetCurrentCount(CellXY);

            // Centro en mundo de la celda: GridWorld da XY como FVector2D
            const FVector2D Center2D = GridWorld::CellToWorldCenterXY(CellXY);
            const float Z = GridWorld::GetOriginWS().Z;
            const FVector Center(Center2D.X, Center2D.Y, Z);

			const bool bOver = (Base > 0) && (Count > Base);
			const FColor Color = bOver ? FColor::Red : FColor::Green;

            // Ajuste al tamaño real de celda (1 m = 100 uu) con margen visual
            const float HalfCell = GridConfig::CellSizeUU * 0.5f;
            const float Margin = HalfCell * 0.10f; // 10% de margen
            const FVector Extent(HalfCell - Margin, HalfCell - Margin, 2.f);
            DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, /*bPersistentLines*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 1.f);

            // Mostrar texto solo si hay datos distintos de 0/0 y respetando GridStep para densidad
            const bool bTextCell = ((x % GridStep) == 0) && ((y % GridStep) == 0);
            if (bTextCell && (Base > 0 || Count > 0))
            {
                const FString Txt = FString::Printf(TEXT("%d/%d"), Count, Base);
                DrawDebugString(GetWorld(), Center + FVector(0, 0, TextZOffset), Txt, nullptr, FColor::White, 0.f, false);
            }
		}
	}
}
