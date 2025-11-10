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
    
    const FVector2D WorldOrigin = FVector2D(GridWorld::GetOriginWS());
    const float PixelsPerUnit = static_cast<float>(InCellSize) / ::GridConfig::CellSizeUU;
    const float Width = ImageSize.X;
    const float Height = ImageSize.Y;
    
    const FVector2D OriginScreen = FVector2D(
        -WorldOrigin.X * PixelsPerUnit,
        Height + (WorldOrigin.Y * PixelsPerUnit)
    );
    
    const int32 NumCellsX = FMath::CeilToInt(Width / InCellSize) + 1;
    const int32 NumCellsY = FMath::CeilToInt(Height / InCellSize) + 1;
    
    // Draw vertical lines
    for (int32 i = 0; i < NumCellsX; ++i)
    {
        const float X = OriginScreen.X + (i * InCellSize);
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 1.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Draw horizontal lines
    for (int32 i = 0; i < NumCellsY; ++i)
    {
        const float Y = OriginScreen.Y - (i * InCellSize);
        if (Y >= 0 && Y <= Height)
        {
            FCanvasLineItem LineItem(FVector2D(0, Y), FVector2D(Width, Y));
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
    const FVector2D WorldOrigin = FVector2D(GridWorld::GetOriginWS());
    const float PixelsPerUnit = static_cast<float>(CellSize) / ::GridConfig::CellSizeUU;
    const float Width = ImageSize.X;
    const float Height = ImageSize.Y;
    
    const FVector2D OriginScreen = FVector2D(
        -WorldOrigin.X * PixelsPerUnit,
        Height + (WorldOrigin.Y * PixelsPerUnit)
    );
    
    const int32 NumTilesX = FMath::CeilToInt(Width / TileSize) + 1;
    const int32 NumTilesY = FMath::CeilToInt(Height / TileSize) + 1;
    
    // Draw tile bounds
    for (int32 i = 0; i <= NumTilesX; ++i)
    {
        const float X = OriginScreen.X + (i * TileSize);
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 2.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    for (int32 i = 0; i <= NumTilesY; ++i)
    {
        const float Y = OriginScreen.Y - (i * TileSize);
        if (Y >= 0 && Y <= Height)
        {
            FCanvasLineItem LineItem(FVector2D(0, Y), FVector2D(Width, Y));
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 2.0f;
            Canvas->DrawItem(LineItem);
        }
    }
}

void UGridDebugWidgetBase::RenderPlayerCell(UCanvas* Canvas, int32 CellSize)
{
    if (!Canvas) return;
    
    const FVector2D WorldOrigin = FVector2D(GridWorld::GetOriginWS());
    const float PixelsPerUnit = static_cast<float>(CellSize) / ::GridConfig::CellSizeUU;
    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    
    // Calculate screen position of the player's cell
    const FVector2D CellScreenPos = FVector2D(
        -WorldOrigin.X * PixelsPerUnit + CachedPlayerCell.X * CellSize,
        Height + (WorldOrigin.Y * PixelsPerUnit) - (CachedPlayerCell.Y + 1) * CellSize
    );
    
    // Draw player cell highlight
    FCanvasTileItem PlayerCellTile(
        CellScreenPos,
        FVector2D(CellSize, CellSize),
        DebugSettings.PlayerCellColor.CopyWithNewOpacity(DebugSettings.PlayerCellOpacity)
    );
    PlayerCellTile.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(PlayerCellTile);
    
    // Draw cell border
    FCanvasBoxItem BorderBox(
        FVector2D(CellScreenPos.X - 1, CellScreenPos.Y - 1),
        FVector2D(CellSize + 2, CellSize + 2)
    );
    BorderBox.SetColor(DebugSettings.PlayerCellColor);
    BorderBox.BlendMode = SE_BLEND_Translucent;
    Canvas->DrawItem(BorderBox);
    
    // Draw cell coordinates
    if (GEngine && GEngine->GetSmallFont())
    {
        const FString CellText = FString::Printf(TEXT("(%d,%d)"), CachedPlayerCell.X, CachedPlayerCell.Y);
        const FVector2D TextPos = FVector2D(
            CellScreenPos.X + 5,
            CellScreenPos.Y + 5
        );
        
        FCanvasTextItem TextItem(TextPos, FText::FromString(CellText), GEngine->GetSmallFont(), FLinearColor::White);
        TextItem.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(TextItem);
    }
}

void UGridDebugWidgetBase::OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || !IsValid(GridImage_GridWorld)) return;
    
    // Clear the render target
    FCanvasTileItem ClearItem(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor::Transparent);
    ClearItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearItem);
    
    // Calculate cell size based on grid dimensions
    const FIntPoint GridDimensions(::GridConfig::TileDim * 2, ::GridConfig::TileDim * 2);
    const int32 CellSize = FMath::Max(10, FMath::Min(Width / GridDimensions.X, Height / GridDimensions.Y));
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
