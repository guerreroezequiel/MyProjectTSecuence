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
    if (GridRenderTarget)
    {
        RenderWorldGrid();
    }
}

void UGridDebugWidgetBase::RenderWorldGrid()
{
    if (!IsValid(GridRenderTarget) || !IsValid(GridImage_GridWorld)) return;
    
    // Obtener el tamaño de la imagen
    FVector2D ImageSize = GridImage_GridWorld->GetCachedGeometry().GetLocalSize();
    if (ImageSize.IsNearlyZero())
    {
        // Tamaño por defecto si no se puede obtener el tamaño de la imagen
        ImageSize = FVector2D(512, 512);
    }

    // Obtener las dimensiones del GridWorld
    // Usamos un tamaño fijo basado en la configuración del grid
    const int32 WorldCellsX = ::GridConfig::TileDim; // 64 celdas por defecto
    const int32 WorldCellsY = ::GridConfig::TileDim; // 64 celdas por defecto
    
    // Calcular el tamaño de celda para que todas las celdas quepan en la imagen
    const float CellWidth = ImageSize.X / WorldCellsX;
    const float CellHeight = ImageSize.Y / WorldCellsY;
    
    // Usar el tamaño más pequeño para mantener la relación de aspecto
    CellSize = FMath::FloorToInt(FMath::Min(CellWidth, CellHeight));
    CellSize = FMath::Max(1, CellSize); // Asegurar un tamaño mínimo de 1 píxel
    
    // Calcular el tamaño del render target para que quepan todas las celdas
    const int32 TargetWidth = WorldCellsX * CellSize;
    const int32 TargetHeight = WorldCellsY * CellSize;
    
    // Actualizar el tamaño del render target si es necesario
    if (GridRenderTarget->SizeX != TargetWidth || GridRenderTarget->SizeY != TargetHeight)
    {
        GridRenderTarget->ResizeTarget(TargetWidth, TargetHeight);
    }

    // Forzar la actualización del render target
    GridRenderTarget->UpdateResource();
}


void UGridDebugWidgetBase::UpdateFromWorldPosition(const FVector& WorldPosition)
{
    // Actualiza la celda central basada en la posición del mundo
    const FIntPoint NewCenterCell = GridWorld::WorldToCellXY(WorldPosition);
    if (NewCenterCell.X != CenterCellX || NewCenterCell.Y != CenterCellY)
    {
        CenterCellX = NewCenterCell.X;
        CenterCellY = NewCenterCell.Y;
        UpdateGridVisualization();
    }
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

void UGridDebugWidgetBase::OnRenderTargetUpdate(UCanvas* Canvas, int32 Width, int32 Height)
{
    if (!Canvas) return;

    // Clear the render target
    FCanvasTileItem ClearItem(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor::Transparent);
    ClearItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearItem);
    
    // Get the grid origin in world space
    const FVector2D GridOriginWS = FVector2D(GridWorld::GetOriginWS());
    
    // Calculate pixels per unit based on cell size
    const float PixelsPerUnit = static_cast<float>(CellSize) / ::GridConfig::CellSizeUU;
    
    // Calculate the origin position on screen (bottom-left corner)
    const FVector2D OriginScreen = FVector2D(
        -GridOriginWS.X * PixelsPerUnit,  // X: from left
        Height + (GridOriginWS.Y * PixelsPerUnit)  // Y: from bottom (invert Y)
    );
    
    // Draw grid lines
    const float GridLineThickness = 1.0f;
    const FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
    
    // Calculate how many cells fit in width and height
    const int32 NumCellsX = FMath::CeilToInt(Width / CellSize) + 1;
    const int32 NumCellsY = FMath::CeilToInt(Height / CellSize) + 1;
    
    // Draw vertical lines (from bottom to top)
    for (int32 i = 0; i < NumCellsX; ++i)
    {
        const float X = OriginScreen.X + (i * CellSize);
        if (X >= 0 && X <= Width)
        {
            FCanvasLineItem LineItem(
                FVector2D(X, 0), 
                FVector2D(X, Height)
            );
            LineItem.SetColor(GridColor);
            LineItem.LineThickness = GridLineThickness;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Draw horizontal lines (from left to right)
    for (int32 i = 0; i < NumCellsY; ++i)
    {
        const float Y = OriginScreen.Y - (i * CellSize); // Subtract because Y grows downward
        if (Y >= 0 && Y <= Height)
        {
            FCanvasLineItem LineItem(
                FVector2D(0, Y), 
                FVector2D(Width, Y)
            );
            LineItem.SetColor(GridColor);
            LineItem.LineThickness = GridLineThickness;
            Canvas->DrawItem(LineItem);
        }
    }
    
    // Draw origin (bottom-left corner)
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
