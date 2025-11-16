// GridDebugWidgetBase.cpp
#include "UI/GridDebugWidgetBase.h"
#include "Components/Image.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateBrush.h"
#include "GridSystem/Core/GridConfig.h"

UGridDebugWidgetBase::UGridDebugWidgetBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , CachedPlayerCell(FIntPoint(0, 0))
    , LastRenderedPlayerCell(FIntPoint(0, 0))
    , LastCellSize(0)
    , UpdateTimer(0.0f)
    , bNeedsUpdate(false)
{
}

void UGridDebugWidgetBase::NativeConstruct()
{
    Super::NativeConstruct();
    CreateGridRenderTarget();
}

void UGridDebugWidgetBase::NativeDestruct()
{
    if (GridRenderTarget)
    {
        GridRenderTarget->OnCanvasRenderTargetUpdate.Clear();
        GridRenderTarget = nullptr;
    }
    Super::NativeDestruct();
}

void UGridDebugWidgetBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    // Update the debug display at the specified rate
    if (bNeedsUpdate)
    {
        UpdateTimer += InDeltaTime;
        if (UpdateTimer >= DebugSettings.UpdateRate)
        {
            UpdateDebugDisplay();
            UpdateTimer = 0.0f;
            bNeedsUpdate = false;
        }
    }
}

void UGridDebugWidgetBase::CreateGridRenderTarget()
{
    if (!GridRenderTarget)
    {
        const int32 RTWidth = 1024;
        const int32 RTHeight = 1024;

        GridRenderTarget = NewObject<UCanvasRenderTarget2D>(this);
        if (GridRenderTarget)
        {
            GridRenderTarget->InitAutoFormat(RTWidth, RTHeight);
            GridRenderTarget->ClearColor = FLinearColor::Transparent;
            
            if (IsValid(GridImage_GridWorld))
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(GridRenderTarget);
                Brush.ImageSize = FVector2D(RTWidth, RTHeight);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                Brush.TintColor = FSlateColor(FLinearColor::White);
                GridImage_GridWorld->SetBrush(Brush);
            }
            
            // Set up the render target update delegate
            GridRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &UGridDebugWidgetBase::OnRenderTargetUpdate);
            
            // Force initial render
            GridRenderTarget->UpdateResource();
        }
    }
}

void UGridDebugWidgetBase::UpdatePlayerPosition(const FVector& WorldLocation)
{
    const FIntPoint NewCell = GridWorld::WorldToCellXY(WorldLocation);
    
    // Debug: Mostrar coordenadas para verificar conversión
    UE_LOG(LogTemp, Warning, TEXT("World: (%.1f,%.1f,%.1f) -> GridCell: (%d,%d)"), 
        WorldLocation.X, WorldLocation.Y, WorldLocation.Z, NewCell.X, NewCell.Y);
    
    // Debug adicional: mostrar límites de celda y posición relativa
    const float CellMinX = NewCell.X * GridConfig::CellSizeUU;
    const float CellMaxX = (NewCell.X + 1) * GridConfig::CellSizeUU;
    const float CellMinY = NewCell.Y * GridConfig::CellSizeUU;
    const float CellMaxY = (NewCell.Y + 1) * GridConfig::CellSizeUU;
    
    const float RelX = WorldLocation.X - CellMinX;
    const float RelY = WorldLocation.Y - CellMinY;
    
    UE_LOG(LogTemp, Warning, TEXT("Cell[%d,%d] Bounds: X[%.0f,%.0f] Y[%.0f,%.0f] PlayerRel: (%.1f,%.1f)"), 
        NewCell.X, NewCell.Y, CellMinX, CellMaxX, CellMinY, CellMaxY, RelX, RelY);
    
    if (NewCell != CachedPlayerCell)
    {
        CachedPlayerCell = NewCell;
        bNeedsUpdate = true;
        OnPlayerCellChanged.Broadcast(NewCell);
    }
}

void UGridDebugWidgetBase::UpdateDebugDisplay()
{
    if (GridRenderTarget)
    {
        GridRenderTarget->UpdateResource();
        LastRenderedPlayerCell = CachedPlayerCell;
    }
}

void UGridDebugWidgetBase::RenderGrid(UCanvas* Canvas, const FVector2D& ImageSize, int32 InCellSize, const FLinearColor& LineColor)
{
    if (!Canvas) return;
    
    const float Width = ImageSize.X;
    const float Height = ImageSize.Y;
    
    // GridSystem: Origen en esquina inferior izquierda (0,0)
    // Canvas: Origen en esquina superior izquierda (0,0)
    // Visualización intuitiva: World Right → Screen Right, World Forward → Screen Up
    
    // IMPORTANTE: Usar dimensiones exactas del GridSystem (64x64)
    // No calcular basado en canvas size, usar TileDim
    const int32 NumCellsX = ::GridConfig::TileDim;  // 64 celdas en X
    const int32 NumCellsY = ::GridConfig::TileDim;  // 64 celdas en Y
    
    // Debug: Mostrar información del grid
    UE_LOG(LogTemp, Warning, TEXT("Grid Debug: Size(%.0f,%.0f) CellSize=%d Cells(%d,%d) [TileDim=%d]"), 
        Width, Height, InCellSize, NumCellsX, NumCellsY, ::GridConfig::TileDim);
    
    // Draw vertical lines (World Right = Screen X)
    // Estas líneas representan coordenadas Y del GridSystem
    // IMPORTANTE: Las líneas deben corresponder a las coordenadas de celda válidas
    for (int32 i = 0; i < NumCellsX; ++i)
    {
        // Calcular X basado en coordenada de celda, no índice de línea
        const float X = i * InCellSize;  // Grid.Y → Screen.X (i representa coordenada Y)
        
        // Debug: Comparar posición de línea con player cell
        if (i == CachedPlayerCell.Y)
        {
            UE_LOG(LogTemp, Warning, TEXT("RENDER GRID: Vertical line for Grid.Y=%d at X=%.1f (player Y=%d) CellSize=%d"), 
                i, X, CachedPlayerCell.Y, InCellSize);
        }
        
        // Debug adicional para primeras líneas
        if (i <= 3)
        {
            UE_LOG(LogTemp, Warning, TEXT("RENDER GRID: Vertical line[%d] X=%.1f (represents Grid.Y=%d)"), 
                i, X, i);
        }
        
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 1.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Draw horizontal lines (World Forward = Screen Y invertido)
    // Estas líneas representan coordenadas X del GridSystem
    for (int32 i = 0; i < NumCellsY; ++i)
    {
        const float GridX = i * InCellSize;        // Grid.X
        const float CanvasY = Height - GridX;      // Grid.X → Screen.Y (invertido)
        
        // Debug: Comparar posición de línea con player cell
        if (i == CachedPlayerCell.X)
        {
            UE_LOG(LogTemp, Warning, TEXT("RENDER GRID: Horizontal line %d at Y=%.1f (player X=%d) CellSize=%d"), 
                i, CanvasY, CachedPlayerCell.X, InCellSize);
        }
        
        if (CanvasY >= 0 && CanvasY <= Height)
        {
            FCanvasLineItem LineItem(FVector2D(0, CanvasY), FVector2D(Width, CanvasY));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 1.0f;
            Canvas->DrawItem(LineItem);
        }
    }
}

void UGridDebugWidgetBase::RenderTileBounds(UCanvas* Canvas, const FVector2D& ImageSize, int32 CellSize, const FLinearColor& LineColor)
{
    if (!Canvas) return;
    
    const int32 TileSize = CellSize * ::GridConfig::TileDim;
    const float Width = ImageSize.X;
    const float Height = ImageSize.Y;
    
    // GridSystem: Origen en esquina inferior izquierda (0,0)
    // Canvas: Origen en esquina superior izquierda (0,0)
    // Visualización intuitiva: World Right → Screen Right, World Forward → Screen Up
    
    const int32 NumTilesX = FMath::CeilToInt(Width / TileSize) + 1;
    const int32 NumTilesY = FMath::CeilToInt(Height / TileSize) + 1;
    
    // Draw vertical tile boundaries (World Right = Screen X)
    // Estas líneas representan coordenadas Y del GridSystem
    for (int32 i = 0; i < NumTilesX; ++i)
    {
        const float X = i * TileSize;  // Grid.Y → Screen.X
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 2.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Draw horizontal tile boundaries (World Forward = Screen Y invertido)
    // Estas líneas representan coordenadas X del GridSystem
    for (int32 i = 0; i < NumTilesY; ++i)
    {
        const float GridX = i * TileSize;        // Grid.X
        const float CanvasY = Height - GridX;    // Grid.X → Screen.Y (invertido)
        
        if (CanvasY >= 0 && CanvasY <= Height)
        {
            FCanvasLineItem LineItem(FVector2D(0, CanvasY), FVector2D(Width, CanvasY));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 2.0f;
            Canvas->DrawItem(LineItem);
        }
    }
}

void UGridDebugWidgetBase::RenderPlayerCell(UCanvas* Canvas, int32 CellSize)
{
    if (!Canvas) return;
    
    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    
    // Calcular posición de la celda del jugador
    const float ExpectedX = CachedPlayerCell.Y * CellSize;        // Grid.Y → Screen.X
    const float ExpectedY = Height - (CachedPlayerCell.X * CellSize); // Grid.X → Screen.Y (invertido)
    
    // Posición de la celda - alinear con bordes del grid
    const FVector2D CellScreenPos = FVector2D(ExpectedX, ExpectedY);
    
    // Solo dibujar el borde de la celda del jugador
    FCanvasBoxItem BorderBox(CellScreenPos, FVector2D(CellSize, CellSize));
    BorderBox.SetColor(DebugSettings.PlayerCellColor);
    BorderBox.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(BorderBox);
}

void UGridDebugWidgetBase::OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || !IsValid(GridImage_GridWorld)) return;
    
    // Clear the render target
    FCanvasTileItem ClearItem(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor::Transparent);
    ClearItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearItem);
    
    // Calculate cell size based on actual grid dimensions from GridConfig
    // Usar dimensiones basadas en TileDim para representar el GridSystem
    const int32 GridCellsToShow = ::GridConfig::TileDim; // Un tile = 64x64 celdas
    const int32 CellSize = FMath::Max(5, FMath::Min(Width / GridCellsToShow, Height / GridCellsToShow));
    LastCellSize = CellSize;
    
    // Draw tile bounds if enabled
    if (DebugSettings.bShowTileBounds)
    {
        RenderTileBounds(Canvas, FVector2D(Width, Height), CellSize, DebugSettings.GridLineColor * 0.7f);
    }
    
    // Draw cell grid if enabled
    if (DebugSettings.bShowCellGrid)
    {
        RenderGrid(Canvas, FVector2D(Width, Height), CellSize, DebugSettings.GridLineColor);
    }
    
    // Draw player cell if enabled and valid
    if (DebugSettings.bShowPlayerCell && CachedPlayerCell != FIntPoint(0, 0))
    {
        RenderPlayerCell(Canvas, CellSize);
    }
}
