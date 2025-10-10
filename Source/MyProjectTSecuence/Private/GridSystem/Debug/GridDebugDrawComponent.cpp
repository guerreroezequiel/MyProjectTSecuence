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

    for (int32 y = MinCell.Y; y <= MaxCell.Y; y += GridStep)
    {
        for (int32 x = MinCell.X; x <= MaxCell.X; x += GridStep)
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

			const FVector Extent(BoxExtent, BoxExtent, 2.f);
			DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, /*bPersistentLines*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 1.f);

			const FString Txt = FString::Printf(TEXT("%d/%d"), Count, Base);
			DrawDebugString(GetWorld(), Center + FVector(0, 0, TextZOffset), Txt, nullptr, FColor::White, 0.f, false);
		}
	}
}
