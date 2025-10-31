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
    // Implementación por defecto: retorna una grilla 10x10
    return FIntPoint(10, 10);
}

FLinearColor UGridDebugWidgetBase::GetCellColor_Implementation(const FIntPoint& CellCoord) const
{
    // Implementación por defecto: alternar colores para celdas pares/impares
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
            FLinearColor CellColor = GetCellColor(FIntPoint(X, Y));
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
    for (int32 X = 0; X <= GridDims.X; ++X)
    {
        float XPos = X * CellWidth;
        FCanvasLineItem LineItem(FVector2D(XPos, 0.0f), FVector2D(XPos, Height));
        LineItem.SetColor(GridLineColor);
        LineItem.LineThickness = LineThickness;
        Canvas->DrawItem(LineItem);
    }

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
    if (!Font)
    {
        return;
    }
    const FLinearColor TextColor = FLinearColor::White;
    const float TextScale = 0.5f;

    // Coordenadas X
    for (int32 X = 0; X < GridDims.X; X += FMath::Max(1, GridDims.X / 10))
    {
        FString Text = FString::FromInt(X);
        FVector2D Position(X * CellWidth + 2.0f, 2.0f);
        
        FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
        TextItem.Scale = FVector2D(TextScale, TextScale);
        TextItem.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(TextItem);
    }

    // Coordenadas Y
    for (int32 Y = 0; Y < GridDims.Y; Y += FMath::Max(1, GridDims.Y / 10))
    {
        FString Text = FString::FromInt(Y);
        FVector2D Position(2.0f, Y * CellHeight + 2.0f);
        
        FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
        TextItem.Scale = FVector2D(TextScale, TextScale);
        TextItem.EnableShadow(FLinearColor::Black);
        Canvas->DrawItem(TextItem);
    }
}
