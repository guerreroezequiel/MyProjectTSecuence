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
        RenderWorldGrid();
    }
}

void UGridDebugWidgetBase::RenderWorldGrid()
{
    if (!GridRenderTarget || !GridImage) return;
    
    // Obtener el tamaño de la imagen
    FVector2D ImageSize = GridImage->GetDesiredSize();
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

    // Limpiar el render target
    FCanvasTileItem ClearItem(FVector2D(0, 0), FVector2D(Width, Height), FLinearColor::Transparent);
    ClearItem.BlendMode = SE_BLEND_Opaque;
    Canvas->DrawItem(ClearItem);
    
    // Dibujar la grilla del mundo
    // Calcular la relación píxeles/unidad basada en el tamaño de celda calculado
    const float PixelsPerUnit = static_cast<float>(CellSize) / ::GridConfig::CellSizeUU;
    
    // Obtener el origen del mundo en píxeles
    const FVector2D OriginWorld = FVector2D(GridWorld::GetOriginWS());
    const FVector2D OriginScreen = FVector2D(
        (Width * 0.5f) + (OriginWorld.X * PixelsPerUnit),
        (Height * 0.5f) + (OriginWorld.Y * PixelsPerUnit)
    );
    
    // Dibujar líneas de la grilla
    const float GridLineThickness = 1.0f;
    const FLinearColor GridColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.3f);
    
    // Líneas verticales
    for (float X = OriginScreen.X; X < Width; X += CellSize)
    {
        FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
        LineItem.SetColor(GridColor);
        LineItem.LineThickness = GridLineThickness;
        Canvas->DrawItem(LineItem);
    }
    for (float X = OriginScreen.X - CellSize; X >= 0; X -= CellSize)
    {
        FCanvasLineItem LineItem(FVector2D(X, 0), FVector2D(X, Height));
        LineItem.SetColor(GridColor);
        LineItem.LineThickness = GridLineThickness;
        Canvas->DrawItem(LineItem);
    }
    
    // Líneas horizontales
    for (float Y = OriginScreen.Y; Y < Height; Y += CellSize)
    {
        FCanvasLineItem LineItem(FVector2D(0, Y), FVector2D(Width, Y));
        LineItem.SetColor(GridColor);
        LineItem.LineThickness = GridLineThickness;
        Canvas->DrawItem(LineItem);
    }
    for (float Y = OriginScreen.Y - CellSize; Y >= 0; Y -= CellSize)
    {
        FCanvasLineItem LineItem(FVector2D(0, Y), FVector2D(Width, Y));
        LineItem.SetColor(GridColor);
        LineItem.LineThickness = GridLineThickness;
        Canvas->DrawItem(LineItem);
    }
    
    // Dibujar el origen
    const float OriginMarkerSize = 5.0f;
    FCanvasBoxItem OriginBox(
        FVector2D(OriginScreen.X - OriginMarkerSize, OriginScreen.Y - OriginMarkerSize),
        FVector2D(OriginMarkerSize * 2, OriginMarkerSize * 2)
    );
    OriginBox.SetColor(FLinearColor::Red);
    Canvas->DrawItem(OriginBox);

    // Dibujar coordenadas - usar la fuente ya declarada
    if (UFont* Font = GEngine ? GEngine->GetSmallFont() : nullptr)
    {
        const FLinearColor TextColor = FLinearColor::White;
        const float TextScale = 0.5f;
        const int32 LabelStep = FMath::Max(1, FMath::FloorToInt(100.0f / CellSize)); // Una etiqueta cada 100 unidades
        const float TextOffset = 5.0f;

        // Coordenadas X (eje horizontal) - abajo
        for (float X = OriginScreen.X; X < Width; X += CellSize * LabelStep)
        {
            int32 WorldX = FMath::RoundToInt((X - OriginScreen.X) / PixelsPerUnit);
            FString Text = FString::Printf(TEXT("%d"), WorldX);
            FVector2D TextSize = FVector2D(10, 10); // Tamaño aproximado
            FVector2D Position(X - (TextSize.X * 0.5f), OriginScreen.Y + TextOffset);
            
            FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
            TextItem.Scale = FVector2D(TextScale, TextScale);
            Canvas->DrawItem(TextItem);
        }
        for (float X = OriginScreen.X - CellSize; X >= 0; X -= CellSize * LabelStep)
        {
            int32 WorldX = FMath::RoundToInt((X - OriginScreen.X) / PixelsPerUnit);
            FString Text = FString::Printf(TEXT("%d"), WorldX);
            FVector2D TextSize = FVector2D(10, 10);
            FVector2D Position(X - (TextSize.X * 0.5f), OriginScreen.Y + TextOffset);
            
            FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
            TextItem.Scale = FVector2D(TextScale, TextScale);
            Canvas->DrawItem(TextItem);
        }

        // Coordenadas Y (eje vertical) - izquierda (invertidas porque Y crece hacia abajo en pantalla)
        for (float Y = OriginScreen.Y; Y < Height; Y += CellSize * LabelStep)
        {
            int32 WorldY = FMath::RoundToInt((Y - OriginScreen.Y) / PixelsPerUnit);
            FString Text = FString::Printf(TEXT("%d"), -WorldY); // Invertir Y para que crezca hacia arriba
            FVector2D TextSize = FVector2D(10, 10);
            FVector2D Position(OriginScreen.X - TextSize.X - TextOffset, Y - (TextSize.Y * 0.5f));
            
            FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
            TextItem.Scale = FVector2D(TextScale, TextScale);
            Canvas->DrawItem(TextItem);
        }
        for (float Y = OriginScreen.Y - CellSize; Y >= 0; Y -= CellSize * LabelStep)
        {
            int32 WorldY = FMath::RoundToInt((Y - OriginScreen.Y) / PixelsPerUnit);
            FString Text = FString::Printf(TEXT("%d"), -WorldY); // Invertir Y para que crezca hacia arriba
            FVector2D TextSize = FVector2D(10, 10);
            FVector2D Position(OriginScreen.X - TextSize.X - TextOffset, Y - (TextSize.Y * 0.5f));
            
            FCanvasTextItem TextItem(Position, FText::FromString(Text), Font, TextColor);
            TextItem.Scale = FVector2D(TextScale, TextScale);
            Canvas->DrawItem(TextItem);
        }
    }
    
    // Resto del código existente para la cuadrícula de depuración
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
