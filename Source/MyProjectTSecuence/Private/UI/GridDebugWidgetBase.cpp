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
#include "GridSystem/Core/GridWorld.h"

UGridDebugWidgetBase::UGridDebugWidgetBase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
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

void UGridDebugWidgetBase::CreateGridRenderTarget()
{
    if (!GridRenderTarget)
    {
        const int32 RTWidth = 1024;
        const int32 RTHeight = 1024;

        GridRenderTarget = NewObject<UCanvasRenderTarget2D>(this);
        if (GridRenderTarget)
        {
            // Updated InitCustomFormat call for UE 5.5
            GridRenderTarget->InitAutoFormat(RTWidth, RTHeight);
            GridRenderTarget->ClearColor = FLinearColor::Transparent;
            GridRenderTarget->UpdateResource();
            
            GridRenderTarget->OnCanvasRenderTargetUpdate.AddDynamic(this, &UGridDebugWidgetBase::OnRenderTargetUpdate);
            
            if (IsValid(GridImage_GridWorld))
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(GridRenderTarget);
                Brush.ImageSize = FVector2D(RTWidth, RTHeight);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                Brush.TintColor = FSlateColor(FLinearColor::White);
                GridImage_GridWorld->SetBrush(Brush);
            }
            
            UpdateGridVisualization();
        }
    }
}

void UGridDebugWidgetBase::UpdateGridVisualization()
{
    if (!IsValid(GridRenderTarget) || !IsValid(GridImage_GridWorld)) return;
    
    // Get image size
    FVector2D ImageSize = GridImage_GridWorld->GetCachedGeometry().GetLocalSize();
    if (ImageSize.IsNearlyZero())
    {
        // Default size if we can't get the image size
        ImageSize = FVector2D(512, 512);
    }

    // Calculate grid dimensions and cell size
    const FIntPoint GridDimensions(::GridConfig::TileDim, ::GridConfig::TileDim);
    const float CellWidth = ImageSize.X / GridDimensions.X;
    const float CellHeight = ImageSize.Y / GridDimensions.Y;
    CellSize = FMath::Max(1, FMath::FloorToInt(FMath::Min(CellWidth, CellHeight)));
    
    // Update render target size if needed
    const int32 TargetWidth = GridDimensions.X * CellSize;
    const int32 TargetHeight = GridDimensions.Y * CellSize;
    
    if (GridRenderTarget->SizeX != TargetWidth || GridRenderTarget->SizeY != TargetHeight)
    {
        GridRenderTarget->ResizeTarget(TargetWidth, TargetHeight);
    }

    // Force render target update
    GridRenderTarget->UpdateResource();
}


FLinearColor UGridDebugWidgetBase::GetCellColor_Implementation(const FIntPoint& CellCoord) const
{
    // Usar GridWorld para convertir las coordenadas si es necesario
    // En este caso, CellCoord ya está en coordenadas de mundo
    
    // Resaltar la celda central
    if (CellCoord.X == CenterCellX && CellCoord.Y == CenterCellY)
    {
        return FLinearColor::Green;
    }
    
    // Alternar colores para mejor visualización
    return (CellCoord.X + CellCoord.Y) % 2 == 0 ? 
        FLinearColor(0.1f, 0.1f, 0.1f, 0.3f) : 
        FLinearColor(0.2f, 0.2f, 0.2f, 0.3f);
}

void UGridDebugWidgetBase::RenderGrid(UCanvas* Canvas, const FVector2D& ImageSize, int32 InCellSize, const FLinearColor& LineColor)
{
    if (!Canvas) return;
    
    // Obtener el origen del mundo desde GridWorld
    const FVector2D WorldOrigin = FVector2D(GridWorld::GetOriginWS());
    const float PixelsPerUnit = static_cast<float>(InCellSize) / ::GridConfig::CellSizeUU;
    const float Width = ImageSize.X;
    const float Height = ImageSize.Y;
    
    // Calcular la posición del origen en pantalla (esquina inferior izquierda)
    const FVector2D OriginScreen = FVector2D(
        -WorldOrigin.X * PixelsPerUnit,
        Height + (WorldOrigin.Y * PixelsPerUnit)
    );
    
    // Calcular cuántas celdas caben en el ancho y alto
    const int32 NumCellsX = FMath::CeilToInt(Width / InCellSize) + 1;
    const int32 NumCellsY = FMath::CeilToInt(Height / InCellSize) + 1;
    
    // Dibujar líneas verticales (de abajo hacia arriba)
    for (int32 i = 0; i < NumCellsX; ++i)
    {
        const float X = OriginScreen.X + (i * InCellSize);
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(
                FVector2D(X, 0), 
                FVector2D(X, Height)
            );
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 1.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Dibujar líneas horizontales (de izquierda a derecha)
    for (int32 i = 0; i < NumCellsY; ++i)
    {
        const float Y = OriginScreen.Y - (i * InCellSize); // Restar porque Y crece hacia abajo
        if (Y >= 0 && Y <= Height)
        {
            FCanvasLineItem LineItem(
                FVector2D(0, Y), 
                FVector2D(Width, Y)
            );
            LineItem.SetColor(LineColor);
            LineItem.LineThickness = 1.0f;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Dibujar el origen (esquina inferior izquierda)
    const float OriginMarkerSize = 5.0f;
    if (OriginScreen.X >= 0 && OriginScreen.X <= Width && 
        OriginScreen.Y >= 0 && OriginScreen.Y <= Height)
    {
        FCanvasBoxItem OriginBox(
            FVector2D(OriginScreen.X - OriginMarkerSize, OriginScreen.Y - OriginMarkerSize),
            FVector2D(OriginMarkerSize * 2, OriginMarkerSize * 2)
        );
        OriginBox.SetColor(FLinearColor::Red);
        Canvas->DrawItem(OriginBox);
    }
}

void UGridDebugWidgetBase::OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas || !IsValid(GridImage_GridWorld)) return;
    
    // Clear the render target
    FCanvasTileItem ClearItem(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor::Transparent);
    ClearItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearItem);
    
    // Render the main grid
    RenderGrid(Canvas, FVector2D(Width, Height), CellSize, GridLineColor);
}
