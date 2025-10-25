#include "GridSystem/Debug/GridDebugDrawComponent.h"
#include "DrawDebugHelpers.h"
#include "HAL/IConsoleManager.h"

// Core/grid helpers
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"

// Capas
#include "GridSystem/Occupancy/CapacityGrid.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"

static TAutoConsoleVariable<int32> CVarGridDebugCap(
    TEXT("grid.debug.cap"),
    1,
    TEXT("")
);

static TAutoConsoleVariable<int32> CVarGridDebugGridStep(
    TEXT("grid.debug.grid_step"),
    4,
    TEXT("")
);

static TAutoConsoleVariable<int32> CVarGridDebugOcc(
    TEXT("grid.debug.occ"),
    0,
    TEXT("")
);

// Toggle para mostrar/ocultar las cajas de cada celda (líneas verdes)
static TAutoConsoleVariable<int32> CVarGridDebugBoxes(
    TEXT("grid.debug.boxes"),
    1,
    TEXT("")
);

UGridDebugDrawComponent::UGridDebugDrawComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}

void UGridDebugDrawComponent::DrawOccupancy()
{
    const FIntPoint TileXY = GetFocusTileXY();

    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    // Contorno del tile
    if (bDrawTileOutline)
    {
        const FVector2D TileOrigin2D = GridWorld::TileToWorldOriginXY(TileXY);
        const float TileSizeUU = GridConfig::TileDim * GridConfig::CellSizeUU;
        const FVector TileCenter(TileOrigin2D.X + TileSizeUU * 0.5f, TileOrigin2D.Y + TileSizeUU * 0.5f, GridWorld::GetOriginWS().Z);
        const FVector TileExtent(TileSizeUU * 0.5f, TileSizeUU * 0.5f, 2.f);
        DrawDebugBox(GetWorld(), TileCenter, TileExtent, FQuat::Identity, FColor(0, 128, 255, 64), false, 0.f, 0, 2.f);
    }

    for (int32 y = MinCell.Y; y <= MaxCell.Y; ++y)
    {
        for (int32 x = MinCell.X; x <= MaxCell.X; ++x)
        {
            const FIntPoint CellXY(x, y);

            const Grid::ECellState State = Grid::GetCellState(CellXY);

            // Centro de celda
            const FVector2D Center2D = GridWorld::CellToWorldCenterXY(CellXY);
            const float Z = GridWorld::GetOriginWS().Z;
            const FVector Center(Center2D.X, Center2D.Y, Z);

            // Color por estado
            FColor Color = FColor::Green; // Empty
            TCHAR StateCh = TEXT('E');
            switch (State)
            {
                case Grid::ECellState::Empty:   Color = FColor::Green;  StateCh = TEXT('E'); break;
                case Grid::ECellState::Obstacle:Color = FColor::Red;    StateCh = TEXT('B'); break;
                case Grid::ECellState::Portal:  Color = FColor::Cyan;   StateCh = TEXT('P'); break;
                default:                         Color = FColor::White;  StateCh = TEXT('?'); break;
            }

            // Caja exacta a la celda con margen mínimo (usar el mismo margen que capacity)
            const float HalfCell = GridConfig::CellSizeUU * 0.5f;
            const float Margin = HalfCell * 0.10f;
            const FVector Extent(HalfCell - Margin, HalfCell - Margin, 2.f);
            if (CVarGridDebugBoxes.GetValueOnGameThread() != 0)
            {
                DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, false, 0.f, 0, 1.f);
            }

            // Texto prefijado 'O' de Occupancy por celda (en el centro de la celda)
            {
                const FString Txt = FString::Printf(TEXT("O %c"), StateCh);
                const FVector LabelPos = Center + FVector(0.f, 0.f, TextZOffset);
                DrawDebugString(GetWorld(), LabelPos, Txt, nullptr, FColor::White, 0.f, false, 1.0f);
            }
        }
    }
}

FIntPoint UGridDebugDrawComponent::GetFocusTileXY() const
{
    const UWorld* World = GetWorld();
    if (!World) { return FIntPoint(0,0); }
    const APlayerController* PC = World->GetFirstPlayerController();
    const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (Pawn)
    {
        return GridWorld::WorldToTileXY(Pawn->GetActorLocation());
    }
    return FIntPoint(0,0);
}

void UGridDebugDrawComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UGridDebugDrawComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetWorld()) return;
    // Origen per-instancia opcional
    if (bUseCustomOrigin)
    {
        GridWorld::SetOriginWS(CustomOriginWS);
    }
    const int32 CapOn = CVarGridDebugCap.GetValueOnGameThread();
    bDrawCapacity = (CapOn != 0);
    GridStep = FMath::Max(1, CVarGridDebugGridStep.GetValueOnGameThread());

	if (bDrawCapacity)
	{
		DrawCapacity();
	}

    if (CVarGridDebugOcc.GetValueOnGameThread() != 0)
    {
        DrawOccupancy();
    }

}

void UGridDebugDrawComponent::DrawCapacity()
{
    // Tile en foco: bajo la cámara si existe, sino (0,0)
    const FIntPoint TileXY = GetFocusTileXY();

    // Determinar bounds del tile en coords de celda [min,max] (incluyentes)
    FIntPoint MinCell, MaxCell;
    GridWorld::TileBoundsInCells(TileXY, MinCell, MaxCell);

    // Dibuja el contorno del tile completo (64x64 celdas) para verificar tamaño
    if (bDrawTileOutline)
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
            if (CVarGridDebugBoxes.GetValueOnGameThread() != 0)
            {
                DrawDebugBox(GetWorld(), Center, Extent, FQuat::Identity, Color, /*bPersistentLines*/ false, /*LifeTime*/ 0.f, /*DepthPriority*/ 0, /*Thickness*/ 1.f);
            }

            // Mostrar texto por celda (incluye 0/0). Prefijo: 'C' (en el centro de la celda)
            {
                const FString Txt = FString::Printf(TEXT("C %d/%d"), Count, Base);
                const FVector LabelPos = Center + FVector(0.f, 0.f, TextZOffset);
                DrawDebugString(GetWorld(), LabelPos, Txt, nullptr, FColor::White, 0.f, false, 1.0f);
            }
		}
	}
}
