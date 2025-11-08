// GridDebugWidgetBase.cpp
#include "UI/GridDebugWidgetBase.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Canvas.h"
#include "Components/Image.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Math/UnrealMathUtility.h"
#include "Styling/SlateBrush.h"

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
            
            if (GridImage)
            {
                FSlateBrush Brush;
                Brush.SetResourceObject(GridRenderTarget);
                Brush.ImageSize = FVector2D(RTWidth, RTHeight);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                Brush.TintColor = FSlateColor(FLinearColor::White);
                GridImage->SetBrush(Brush);
            }
            
            UpdateGridVisualization();
        }
    }
}

void UGridDebugWidgetBase::UpdateGridVisualization()
{
    if (GridRenderTarget)
    {
        GridRenderTarget->UpdateResource();
    }
}

FIntPoint UGridDebugWidgetBase::GetGridDimensions_Implementation() const
{
    // Tamaño de la grilla basado en el radio (radio*2 + 1)
    const int32 GridSize = (GridRadius * 2) + 1;
    return FIntPoint(GridSize, GridSize);
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

    const FIntPoint GridDims = GetGridDimensions();
    if (GridDims.X <= 0 || GridDims.Y <= 0) return;

    const float CellWidth = static_cast<float>(Width) / GridDims.X;
    const float CellHeight = static_cast<float>(Height) / GridDims.Y;

    // 1. Dibujar celdas
    for (int32 Y = 0; Y < GridDims.Y; ++Y)
    {
        for (int32 X = 0; X < GridDims.X; ++X)
        {
            // Calcular coordenadas de mundo para esta celda de la interfaz
            const int32 WorldX = CenterCellX + (X - GridRadius);
            const int32 WorldY = CenterCellY + (Y - GridRadius);
            const FIntPoint WorldGridPos(WorldX, WorldY);
            
            // Obtener el color de la celda
            FLinearColor CellColor = GetCellColor(WorldGridPos);
            
            FCanvasTileItem TileItem(
                FVector2D(X * CellWidth, Y * CellHeight),
                FVector2D(CellWidth, CellHeight),
                CellColor
            );
            TileItem.BlendMode = SE_BLEND_Translucent;
            Canvas->DrawItem(TileItem);
        }
    }

    // 2. Dibujar bordes
    const float LineThickness = 1.0f;
    // Bordes verticales
    for (int32 X = 0; X <= GridDims.X; ++X)
    {
        float XPos = X * CellWidth;
        FCanvasLineItem LineItem(FVector2D(XPos, 0.0f), FVector2D(XPos, Height));
        LineItem.SetColor(GridLineColor);
        LineItem.LineThickness = LineThickness;
        Canvas->DrawItem(LineItem);
    }
    // Bordes horizontales
    for (int32 Y = 0; Y <= GridDims.Y; ++Y)
    {
        float YPos = Y * CellHeight;
        FCanvasLineItem LineItem(FVector2D(0.0f, YPos), FVector2D(Width, YPos));
        LineItem.SetColor(GridLineColor);
        LineItem.LineThickness = LineThickness;
        Canvas->DrawItem(LineItem);
    }

    // 3. Dibujar coordenadas
    UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr;
    if (!Font) return;
    
    const FLinearColor TextColor = FLinearColor::White;
    const float TextScale = 0.5f;
    const int32 LabelStep = FMath::Max(1, GridDims.X / 10);

    // Coordenadas X (inferior)
    for (int32 X = 0; X < GridDims.X; X += LabelStep)
    {
        const int32 WorldX = CenterCellX + (X - GridRadius);
        FString Text = FString::Printf(TEXT("%d"), WorldX);
        FVector2D Position(X * CellWidth + 2.0f, Height - 20.0f);
        
        FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
        TextItem.Scale = FVector2D(TextScale, TextScale);
        TextItem.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(TextItem);
    }

    // Coordenadas Y (izquierda)
    for (int32 Y = 0; Y < GridDims.Y; Y += LabelStep)
    {
        const int32 WorldY = CenterCellY + (Y - GridRadius);
        FString Text = FString::Printf(TEXT("%d"), WorldY);
        FVector2D Position(2.0f, Y * CellHeight + 2.0f);
        
        FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
        TextItem.Scale = FVector2D(TextScale, TextScale);
        TextItem.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(TextItem);
    }
    
    // Dibujar coordenadas de la celda central
    FString CenterText = FString::Printf(TEXT("(%d,%d)"), CenterCellX, CenterCellY);
    FVector2D CenterPosition((GridDims.X * 0.5f) * CellWidth, (GridDims.Y * 0.5f) * CellHeight);
    FCanvasTextItem CenterTextItem(CenterPosition, FText::FromString(CenterText), Font, FLinearColor::Black);
    CenterTextItem.Scale = FVector2D(1.0f, 1.0f);
    CenterTextItem.bCentreX = true;
    CenterTextItem.bCentreY = true;
    Canvas->DrawItem(CenterTextItem);
}
