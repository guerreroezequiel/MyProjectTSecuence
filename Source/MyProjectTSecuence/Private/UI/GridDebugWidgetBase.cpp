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
            
            // Set up the render target update delegate
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
            
            // Force initial render
            GridRenderTarget->UpdateResource();
        }
    }
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
    
    // Calculate cell size based on grid dimensions
    const FIntPoint GridDimensions(::GridConfig::TileDim, ::GridConfig::TileDim);
    const int32 CellSize = FMath::Max(1, FMath::Min(Width / GridDimensions.X, Height / GridDimensions.Y));
    
    // Render the main grid
    RenderGrid(Canvas, FVector2D(Width, Height), CellSize, GridLineColor);
}
