// Copyright 2024 MyProjectTSec

#include "GridSystem/Systems/FlowFieldSystem/FlowFieldSystem.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/FlowField/FlowField.h"
#include "GridSystem/FlowField/StaticCostBaker.h"
#include "GridSystem/Core/TileRegistry.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GridSystem/Core/GridEpochSubsystem.h"

UFlowFieldSystem::UFlowFieldSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFlowFieldSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Inicialización de la grilla de tiles
    InitializeWorldTiles();
}

void UFlowFieldSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bAutoUpdate)
    {
        // Actualizar HOT/WARM a partir de la posición del jugador y derivar ActiveTiles
        UpdateHotWarmFromPlayer();

        // Actualizar solo tiles activos (HOT ∪ WARM)
        for (const FIntPoint& TileXY : ActiveTiles)
        {
            TSharedPtr<FTileContext> Tile = Grid::Tiles::GetTileContext(TileXY);
            if (!Tile.IsValid()) { continue; }
            // Pipeline: UpdateEpochs -> (si cambió StaticCostEpoch) Bake -> luego IsValid/Rebuild
            const int32 PrevStaticCostEpoch = Tile->GetStaticCostEpoch();
            if (Tile->IsDirty())
                Tile->UpdateEpochs();

            const int32 CurrStaticCostEpoch = Tile->GetStaticCostEpoch();
            if (CurrStaticCostEpoch != PrevStaticCostEpoch)
            {
                const FIntPoint TileXYCtx = Tile->GetTileXY();
                Grid::StaticCost::BakeFinalCostStatic(TileXYCtx, *Tile);
            }

            // Verificar y reconstruir FlowField para MVP (solo Players)
            const EFlowIntent CurrentIntent = EFlowIntent::Players;
            if (!FlowFields.Contains(CurrentIntent))
            {
                FlowFields.Add(CurrentIntent, MakeShared<FFlowField>());
            }
            if (!FlowFields[CurrentIntent]->IsValid(*Tile, CurrentIntent))
            {
                RebuildFlowField(Tile, CurrentIntent);
            }
        }
    }

    // Dibujar debug si está habilitado
    if (bDebugVisualization)
    {
        DrawDebugInfo();
    }
}

void UFlowFieldSystem::ToggleDebugVisualization(bool bShow)
{
    // TODO: Implementar lógica de visualización de debug
}

void UFlowFieldSystem::RebuildFlowField(const TSharedPtr<FTileContext>& Tile, EFlowIntent Intent)
{
    if (!Tile.IsValid() || !FlowFields.Contains(Intent))
    {
        return;
    }

    // Obtener el FlowField para este Intent
    TSharedPtr<FFlowField> FlowField = FlowFields[Intent];
    
    // Reconstruir el FlowField
    FlowField->Rebuild(*Tile, Intent);
    
    // Aquí podrías añadir lógica adicional después de reconstruir, como:
    // - Actualizar estadísticas
    // - Disparar eventos
    // - Actualizar visualización
    
    UE_LOG(LogTemp, Verbose, TEXT("FlowField reconstruido para Intent: %d"), static_cast<int32>(Intent));
}

void UFlowFieldSystem::InitializeWorldTiles()
{
    const int32 Dim = GridConfig::WorldDim;
    for (int32 ty = 0; ty < Dim; ++ty)
    {
        for (int32 tx = 0; tx < Dim; ++tx)
        {
            const FIntPoint TileXY(tx, ty);
            TSharedPtr<FTileContext> Ctx = Grid::Tiles::EnsureTileContext(TileXY);
            if (Ctx.IsValid())
            {
                Grid::Tiles::SetTileState(TileXY, Grid::Tiles::ETileState::Cold);
            }
        }
    }
}

void UFlowFieldSystem::UpdateHotWarmFromPlayer()
{
    const UWorld* World = GetWorld();
    if (!World) { return; }

    const APlayerController* PC = World->GetFirstPlayerController();
    const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) { return; }

    const FVector PlayerPos = Pawn->GetActorLocation();
    const FIntPoint NewPlayerCell = GridWorld::WorldToCellXY(PlayerPos);
    FIntPoint NewHot = GridWorld::WorldToTileXY(PlayerPos);

    NewHot.X = FMath::Clamp(NewHot.X, 0, GridConfig::WorldDim - 1);
    NewHot.Y = FMath::Clamp(NewHot.Y, 0, GridConfig::WorldDim - 1);

    const bool bHotChanged = (NewHot != CurrentHotTileXY);
    const bool bCellChanged = (NewPlayerCell != CurrentPlayerCellXY);

    if (bHotChanged)
    {
        CurrentHotTileXY = NewHot;
        RecomputeWarmAroundHot(NewHot);
    }

    if (bCellChanged)
    {
        CurrentPlayerCellXY = NewPlayerCell;

        // Construir GoalSet (Players) en el Subsystem con la celda actual del jugador
        UGridEpochSubsystem* Subsystem = World->GetSubsystem<UGridEpochSubsystem>();
        if (Subsystem)
        {
            Subsystem->Goals.GoalCells.Reset(1);
            Subsystem->Goals.GoalCells.Add(CurrentPlayerCellXY);
        }

        // Marcar GoalsDirty(Players) en tiles activos (HOT ∪ WARM)
        for (const FIntPoint& TileXY : ActiveTiles)
        {
            if (TSharedPtr<FTileContext> Ctx = Grid::Tiles::GetTileContext(TileXY); Ctx.IsValid())
            {
                Ctx->MarkGoalsDirty(EFlowIntent::Players);
            }
        }
    }
}

void UFlowFieldSystem::RecomputeWarmAroundHot(const FIntPoint& NewHot)
{
    for (const FIntPoint& XY : HotTiles)
    {
        Grid::Tiles::SetTileState(XY, Grid::Tiles::ETileState::Cold);
    }
    for (const FIntPoint& XY : WarmTiles)
    {
        Grid::Tiles::SetTileState(XY, Grid::Tiles::ETileState::Cold);
    }
    HotTiles.Reset();
    WarmTiles.Reset();
    ActiveTiles.Reset();

    HotTiles.Add(NewHot);
    Grid::Tiles::SetTileState(NewHot, Grid::Tiles::ETileState::Hot);

    const int32 Dim = GridConfig::WorldDim;
    for (int32 dy = -1; dy <= 1; ++dy)
    {
        for (int32 dx = -1; dx <= 1; ++dx)
        {
            if (dx == 0 && dy == 0) { continue; }
            const int32 nx = NewHot.X + dx;
            const int32 ny = NewHot.Y + dy;
            if (nx < 0 || ny < 0 || nx >= Dim || ny >= Dim) { continue; }
            const FIntPoint NXY(nx, ny);
            WarmTiles.Add(NXY);
            Grid::Tiles::SetTileState(NXY, Grid::Tiles::ETileState::Warm);
        }
    }

    ActiveTiles = WarmTiles;
    ActiveTiles.Append(HotTiles);
}

void UFlowFieldSystem::DrawDebugInfo() const
{
    if (!bDebugVisualization)
    {
        return;
    }

    // TODO: Implementar visualización de debug
    // Mostrar epochs, dirty flags, etc.
    
    // Ejemplo de visualización básica
    for (const FIntPoint& TileXY : ActiveTiles)
    {
        const TSharedPtr<FTileContext> Tile = Grid::Tiles::GetTileContext(TileXY);
        if (!Tile.IsValid()) { continue; }

        for (const auto& FlowFieldPair : FlowFields)
        {
            const EFlowIntent Intent = FlowFieldPair.Key;
            const TSharedPtr<FFlowField>& FlowField = FlowFieldPair.Value;
            if (!FlowField.IsValid()) { continue; }

            const FString DebugText = FString::Printf(TEXT("Tile (%d,%d) Intent: %d\n%s"),
                TileXY.X, TileXY.Y,
                static_cast<int32>(Intent),
                *FlowField->GetDebugInfo());

            GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, DebugText);
        }
    }
}
