#include "CoreMinimal.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/Core/TileRegistry.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "GridSystem/FlowField/AFlowFieldDebugActor.h"
#include "GridSystem/FlowField/GridFlowArrowActor.h"
#include "GridSystem/FlowField/UFlowFieldMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"
#include "GridSystem/Occupancy/GridObstaclePlate.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/Systems/FlowFieldSystem/FlowFieldSystem.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/FlowField/FlowField.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "DrawDebugHelpers.h"
#include "Containers/Ticker.h"

// Minimal console: Occupancy and Flow debug (no FlowFieldRegistry)
// Commands:
// - grid.occ.set x y state (0=Empty,1=Obstacle,2=Portal)
// - grid.occ.clear
// - grid.flow.set_goal x y
// - ff.valid tx ty
// - tile.info tx ty
// - tile.arrows.on / tile.arrows.off
// - grid.flow.arrow.spawn x y

// Globals for tile border debug tick
static bool GTileBordersEnabled = false;
static FTSTicker::FDelegateHandle GTileBordersTickerHandle;

static UWorld* GetDebugWorld()
{
    if (!GEngine) { return nullptr; }
    const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
    // Prefer PIE
    for (const FWorldContext& Ctx : Contexts)
    {
        if (Ctx.World() && Ctx.WorldType == EWorldType::PIE) { return Ctx.World(); }
    }
    // Then Game
    for (const FWorldContext& Ctx : Contexts)
    {
        if (Ctx.World() && Ctx.WorldType == EWorldType::Game) { return Ctx.World(); }
    }
    // Finally Editor world
    for (const FWorldContext& Ctx : Contexts)
    {
        if (Ctx.World() && Ctx.WorldType == EWorldType::Editor) { return Ctx.World(); }
    }
    return nullptr;
}

// ===== HOT/WARM ARROWS (per-tick) =====
static bool GTileArrowsEnabled = false;
static FTSTicker::FDelegateHandle GTileArrowsTickerHandle;
static TMap<FIntPoint, TWeakObjectPtr<AGridFlowArrowActor>> GCellToArrow;

static bool Tick_DrawHotWarmArrows(float DeltaTime)
{
    UWorld* World = GetDebugWorld();
    if (!World) { return true; }

    TSet<FIntPoint> TargetCells;
    const int32 Dim = GridConfig::WorldDim;
    const int32 TileDim = GridConfig::TileDim;

    // Compute desired cells for tiles with state Hot or Warm
    for (int32 ty = 0; ty < Dim; ++ty)
    {
        for (int32 tx = 0; tx < Dim; ++tx)
        {
            const FIntPoint TileXY(tx, ty);
            const auto State = Grid::Tiles::GetTileState(TileXY);
            if (State == Grid::Tiles::ETileState::Hot || State == Grid::Tiles::ETileState::Warm)
            {
                for (int32 cy = 0; cy < TileDim; ++cy)
                {
                    for (int32 cx = 0; cx < TileDim; ++cx)
                    {
                        const int32 X = tx * TileDim + cx;
                        const int32 Y = ty * TileDim + cy;
                        TargetCells.Add(FIntPoint(X, Y));
                    }
                }
            }
        }
    }

    // Spawn missing arrows
    for (const FIntPoint& Cell : TargetCells)
    {
        if (!GCellToArrow.Contains(Cell) || !GCellToArrow[Cell].IsValid())
        {
            const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(Cell);
            const FVector WorldPosition(WorldPos2D.X, WorldPos2D.Y, 20.0f);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AGridFlowArrowActor* Arrow = World->SpawnActor<AGridFlowArrowActor>(
                AGridFlowArrowActor::StaticClass(),
                WorldPosition,
                FRotator::ZeroRotator,
                SpawnParams
            );
            if (Arrow)
            {
                Arrow->SetCell(Cell);
                GCellToArrow.Add(Cell, Arrow);
            }
        }
    }

    // Remove arrows no longer needed
    TArray<FIntPoint> ToRemove;
    for (const auto& Pair : GCellToArrow)
    {
        if (!TargetCells.Contains(Pair.Key))
        {
            if (Pair.Value.IsValid())
            {
                Pair.Value->Destroy();
            }
            ToRemove.Add(Pair.Key);
        }
    }
    for (const FIntPoint& Key : ToRemove)
    {
        GCellToArrow.Remove(Key);
    }

    return GTileArrowsEnabled;
}

static bool Tick_DrawAllTileBorders(float DeltaTime)
{
    UWorld* World = GetDebugWorld();
    if (!World) { return true; }

    // Clear previous persistent lines and redraw
    FlushPersistentDebugLines(World);

    const int32 Dim = GridConfig::WorldDim;
    const float SizeUU = GridConfig::TileDim * GridConfig::CellSizeUU;
    const float Z = 20.0f;

    for (int32 ty = 0; ty < Dim; ++ty)
    {
        for (int32 tx = 0; tx < Dim; ++tx)
        {
            const FIntPoint TileXY(tx, ty);
            const auto State = Grid::Tiles::GetTileState(TileXY);

            FColor Color = FColor::Cyan;
            if (State == Grid::Tiles::ETileState::Hot) { Color = FColor::Red; }
            else if (State == Grid::Tiles::ETileState::Warm) { Color = FColor::Yellow; }

            const FVector2D Origin2D = GridWorld::TileToWorldOriginXY(TileXY);
            const FVector A(Origin2D.X,            Origin2D.Y,            Z);
            const FVector B(Origin2D.X + SizeUU,   Origin2D.Y,            Z);
            const FVector C(Origin2D.X + SizeUU,   Origin2D.Y + SizeUU,   Z);
            const FVector D(Origin2D.X,            Origin2D.Y + SizeUU,   Z);

            DrawDebugLine(World, A, B, Color, true, 0.0f, 0, 2.0f);
            DrawDebugLine(World, B, C, Color, true, 0.0f, 0, 2.0f);
            DrawDebugLine(World, C, D, Color, true, 0.0f, 0, 2.0f);
            DrawDebugLine(World, D, A, Color, true, 0.0f, 0, 2.0f);
        }
    }

    return GTileBordersEnabled; // keep ticking while enabled
}



// tile.borders.on
static FAutoConsoleCommand GCmdTileBordersOn(
    TEXT("tile.borders.on"),
    TEXT("Enable per-tick drawing of tile borders for all tiles (Hot=Red, Warm=Yellow, Cold=Cyan)"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        if (!GTileBordersEnabled)
        {
            GTileBordersEnabled = true;
            FTickerDelegate TickDel = FTickerDelegate::CreateStatic(&Tick_DrawAllTileBorders);
            GTileBordersTickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickDel);
        }
        UE_LOG(LogTemp, Log, TEXT("Tile borders ON (per-tick)"));
    })
);

// tile.borders.off
static FAutoConsoleCommand GCmdTileBordersOff(
    TEXT("tile.borders.off"),
    TEXT("Clear all persistent debug lines (including tile borders)"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        if (GTileBordersEnabled)
        {
            GTileBordersEnabled = false;
            if (GTileBordersTickerHandle.IsValid())
            {
                FTSTicker::GetCoreTicker().RemoveTicker(GTileBordersTickerHandle);
                GTileBordersTickerHandle.Reset();
            }
        }
        if (UWorld* World = GetDebugWorld())
        {
            FlushPersistentDebugLines(World);
        }
        UE_LOG(LogTemp, Log, TEXT("Tile borders OFF (cleared)"));
    })
);

// tile.arrows.on
static FAutoConsoleCommand GCmdTileArrowsOn(
    TEXT("tile.arrows.on"),
    TEXT("Enable per-tick spawning of flow arrows on all cells of HOT and WARM tiles"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        if (!GTileArrowsEnabled)
        {
            GTileArrowsEnabled = true;
            FTickerDelegate TickDel = FTickerDelegate::CreateStatic(&Tick_DrawHotWarmArrows);
            GTileArrowsTickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickDel);
        }
        UE_LOG(LogTemp, Log, TEXT("Tile arrows ON (per-tick)"));
    })
);

// tile.arrows.off
static FAutoConsoleCommand GCmdTileArrowsOff(
    TEXT("tile.arrows.off"),
    TEXT("Disable per-tick arrows and destroy existing arrow actors"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        if (GTileArrowsEnabled)
        {
            GTileArrowsEnabled = false;
            if (GTileArrowsTickerHandle.IsValid())
            {
                FTSTicker::GetCoreTicker().RemoveTicker(GTileArrowsTickerHandle);
                GTileArrowsTickerHandle.Reset();
            }
        }
        // Destroy any remaining arrows
        for (auto& Pair : GCellToArrow)
        {
            if (Pair.Value.IsValid())
            {
                Pair.Value->Destroy();
            }
        }
        GCellToArrow.Empty();
        UE_LOG(LogTemp, Log, TEXT("Tile arrows OFF (cleared)"));
    })
);
// grid.flow.arrow.spawn x y
static FAutoConsoleCommand GCmdGridFlowArrowSpawn(
    TEXT("grid.flow.arrow.spawn"),
    TEXT("Spawn a flow arrow actor at cell center to visualize real-time direction: grid.flow.arrow.spawn <x> <y>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2)
        {
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.arrow.spawn <x> <y>"));
            return;
        }

        int32 X = 0, Y = 0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);

        if (GWorld)
        {
            const FIntPoint Cell(X, Y);
            const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(Cell);
            const FVector WorldPosition(WorldPos2D.X, WorldPos2D.Y, 20.0f);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            AGridFlowArrowActor* Arrow = GWorld->SpawnActor<AGridFlowArrowActor>(
                AGridFlowArrowActor::StaticClass(),
                WorldPosition,
                FRotator::ZeroRotator,
                SpawnParams
            );

            if (Arrow)
            {
                Arrow->SetCell(Cell);
                UE_LOG(LogTemp, Log, TEXT("Flow arrow spawned at cell (%d,%d)"), X, Y);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn GridFlowArrowActor"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// grid.flow.arrows.show
static FAutoConsoleCommand GCmdGridFlowShowArrows(
    TEXT("grid.flow.arrows.show"),
    TEXT("Show flow arrows for one tile (16x16 cells) starting from (0,0)"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (!GWorld)
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
            return;
        }

        const FIntPoint MinCell(0, 0);
        const FIntPoint MaxCell(GridConfig::TileDim - 1, GridConfig::TileDim - 1);
        
        int32 ArrowCount = 0;
        FActorSpawnParameters SpawnParams;
        SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        // Iterate through all cells in the grid
        for (int32 Y = MinCell.Y; Y <= MaxCell.Y; ++Y)
        {
            for (int32 X = MinCell.X; X <= MaxCell.X; ++X)
            {
                const FIntPoint Cell(X, Y);
                const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(Cell);
                const FVector WorldPosition(WorldPos2D.X, WorldPos2D.Y, 20.0f);

                AGridFlowArrowActor* Arrow = GWorld->SpawnActor<AGridFlowArrowActor>(
                    AGridFlowArrowActor::StaticClass(),
                    WorldPosition,
                    FRotator::ZeroRotator,
                    SpawnParams
                );

                if (Arrow)
                {
                    Arrow->SetCell(Cell);
                    ArrowCount++;
                }
            }
        }

        UE_LOG(LogTemp, Log, TEXT("Spawned %d flow arrows in a %dx%d grid starting from (0,0)"), 
            ArrowCount, GridConfig::TileDim, GridConfig::TileDim);
    })
);

 

// grid.occ.set x y state
static FAutoConsoleCommand GCmdGridOccSet(
    TEXT("grid.occ.set"),
    TEXT("Set occupancy state: grid.occ.set <x> <y> <state:0|1|2> OR grid.occ.set <tileX> <tileY> <cellX> <cellY> <state>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() != 3 && Args.Num() != 5) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.occ.set <x> <y> <state> OR grid.occ.set <tileX> <tileY> <cellX> <cellY> <state>")); return; }

        int32 S = 0;
        int32 X = 0, Y = 0;
        if (Args.Num() == 3)
        {
            LexFromString(X, *Args[0]);
            LexFromString(Y, *Args[1]);
            LexFromString(S, *Args[2]);
        }
        else
        {
            int32 TX=0, TY=0, CX=0, CY=0;
            LexFromString(TX, *Args[0]);
            LexFromString(TY, *Args[1]);
            LexFromString(CX, *Args[2]);
            LexFromString(CY, *Args[3]);
            LexFromString(S,  *Args[4]);
            CX = FMath::Clamp(CX, 0, GridConfig::TileDim - 1);
            CY = FMath::Clamp(CY, 0, GridConfig::TileDim - 1);
            X = TX * GridConfig::TileDim + CX;
            Y = TY * GridConfig::TileDim + CY;
        }

        S = FMath::Clamp(S, 0, 2);
        Grid::ECellState State = static_cast<Grid::ECellState>(S);
        const FIntPoint Cell(X,Y);
        Grid::SetCellState(Cell, State);
        const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
        // Marca explícita para el nuevo pipeline
        if (TSharedPtr<FTileContext> Ctx = Grid::Tiles::EnsureTileContext(TileXY); Ctx.IsValid())
        {
            Ctx->MarkStaticCostDirty();
        }
        UE_LOG(LogTemp, Log, TEXT("Occupancy set cell (%d,%d) = %d; tile (%d,%d) flagged StaticCostDirty"), X, Y, S, TileXY.X, TileXY.Y);
    })
);

// grid.occ.clear
static FAutoConsoleCommand GCmdGridOccClear(
    TEXT("grid.occ.clear"),
    TEXT("Clear entire occupancy grid"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        // Limpiar ocupación global
        Grid::ClearAll();

        // Marcar todos los tiles como StaticCostDirty (pipeline nuevo)
        const int32 Dim = GridConfig::WorldDim;
        for (int32 ty = 0; ty < Dim; ++ty)
        {
            for (int32 tx = 0; tx < Dim; ++tx)
            {
                const FIntPoint TileXY(tx, ty);
                if (TSharedPtr<FTileContext> Ctx = Grid::Tiles::EnsureTileContext(TileXY); Ctx.IsValid())
                {
                    Ctx->MarkStaticCostDirty();
                }
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Occupancy cleared; all tiles flagged StaticCostDirty"));
    })
);

// grid.occ.spawn x y state
static FAutoConsoleCommand GCmdGridOccSpawn(
    TEXT("grid.occ.spawn"),
    TEXT("Spawn obstacle plate and set occupancy: grid.occ.spawn <x> <y> <state> OR grid.occ.spawn <tileX> <tileY> <cellX> <cellY> <state>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() != 3 && Args.Num() != 5) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.occ.spawn <x> <y> <state> OR grid.occ.spawn <tileX> <tileY> <cellX> <cellY> <state>")); return; }

        int32 S=0; int32 X=0, Y=0;
        if (Args.Num() == 3)
        {
            LexFromString(X, *Args[0]);
            LexFromString(Y, *Args[1]);
            LexFromString(S, *Args[2]);
        }
        else
        {
            int32 TX=0, TY=0, CX=0, CY=0;
            LexFromString(TX, *Args[0]);
            LexFromString(TY, *Args[1]);
            LexFromString(CX, *Args[2]);
            LexFromString(CY, *Args[3]);
            LexFromString(S,  *Args[4]);
            CX = FMath::Clamp(CX, 0, GridConfig::TileDim - 1);
            CY = FMath::Clamp(CY, 0, GridConfig::TileDim - 1);
            X = TX * GridConfig::TileDim + CX;
            Y = TY * GridConfig::TileDim + CY;
        }
        S = FMath::Clamp(S, 0, 2);
        Grid::ECellState State = static_cast<Grid::ECellState>(S);

        const FIntPoint Cell(X, Y);
        Grid::SetCellState(Cell, State);
        const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
        // Marca explícita para el nuevo pipeline
        if (TSharedPtr<FTileContext> Ctx = Grid::Tiles::EnsureTileContext(TileXY); Ctx.IsValid())
        {
            Ctx->MarkStaticCostDirty();
        }

        if (GWorld)
        {
            const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(Cell);
            const FVector WorldPosition(WorldPos2D.X, WorldPos2D.Y, 5.0f);

            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

            if (State == Grid::ECellState::Obstacle)
            {
                AGridObstaclePlate* Plate = GWorld->SpawnActor<AGridObstaclePlate>(
                    AGridObstaclePlate::StaticClass(),
                    WorldPosition,
                    FRotator::ZeroRotator,
                    SpawnParams
                );

                if (Plate)
                {
                    UE_LOG(LogTemp, Log, TEXT("OccSpawn: cell (%d,%d) state=%d; spawned plate at (%.1f, %.1f, %.1f); tile (%d,%d) flagged StaticCostDirty"),
                        X, Y, S, WorldPosition.X, WorldPosition.Y, WorldPosition.Z, TileXY.X, TileXY.Y);
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Failed to spawn GridObstaclePlate"));
                }
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("OccSpawn: cell (%d,%d) state=%d; no plate spawned; tile (%d,%d) flagged StaticCostDirty"),
                    X, Y, S, TileXY.X, TileXY.Y);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// ===== FLOW FIELD CONSOLE COMMANDS =====

// grid.flow.set_goal x y
static FAutoConsoleCommand GCmdGridFlowSetGoal(
    TEXT("grid.flow.set_goal"),
    TEXT("Set flow field goal: grid.flow.set_goal <x> <y>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.set_goal <x> <y>")); 
            return; 
        }
        
        int32 X = 0, Y = 0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        
        // Obtener el mundo actual
        if (GWorld)
        {
            UGridEpochSubsystem* Subsystem = GWorld->GetSubsystem<UGridEpochSubsystem>();
            if (Subsystem)
            {
                // Limpiar metas anteriores y establecer nueva meta
                Subsystem->Goals.GoalCells.Empty();
                Subsystem->Goals.GoalCells.Add(FIntPoint(X, Y));

                // Marcar tile del goal como dirty de goals para recalcular (intent Players)
                const FIntPoint TileXY = GridWorld::CellToTileXY(FIntPoint(X, Y));
                if (TSharedPtr<FTileContext> Ctx = Grid::Tiles::GetTileContext(TileXY)) { Ctx->MarkGoalsDirty(EFlowIntent::Players); }

                UE_LOG(LogTemp, Log, TEXT("FlowField goal set at cell (%d,%d); marked tile (%d,%d) goals dirty"), 
                    X, Y, TileXY.X, TileXY.Y);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get UGridEpochSubsystem"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// Array para mantener un seguimiento de los actores de depuración generados
TArray<TWeakObjectPtr<AFlowFieldDebugActor>> GDebugActors;

// Función para obtener todos los actores en una celda específica
TArray<AFlowFieldDebugActor*> GetActorsInCell(const FIntPoint& CellCoord)
{
    TArray<AFlowFieldDebugActor*> ActorsInCell;
    
    // Eliminar actores nulos primero
    GDebugActors.RemoveAll([](const TWeakObjectPtr<AFlowFieldDebugActor>& Actor) {
        return !Actor.IsValid();
    });
    
    // Encontrar actores en la celda especificada
    for (const auto& ActorPtr : GDebugActors)
    {
        if (ActorPtr.IsValid())
        {
            const FVector ActorLocation = ActorPtr->GetActorLocation();
            const FIntPoint ActorCell = GridWorld::WorldToCellXY(ActorLocation);
            
            if (ActorCell == CellCoord)
            {
                ActorsInCell.Add(ActorPtr.Get());
            }
        }
    }
    
    return ActorsInCell;
}

// tile.info tx ty
static FAutoConsoleCommand GCmdTileInfo(
    TEXT("tile.info"),
    TEXT("Show tile information: tile.info <tx> <ty>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: tile.info <tx> <ty>")); 
            return; 
        }

        int32 TX = 0, TY = 0;
        LexFromString(TX, *Args[0]);
        LexFromString(TY, *Args[1]);

        const FIntPoint TileCoord(TX, TY);
        
        // Get TileContext
        TSharedPtr<FTileContext> TileContext = Grid::Tiles::GetTileContext(TileCoord);
        if (!TileContext.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("No TileContext found for tile (%d,%d)"), TX, TY);
            return;
        }

        // Get current intent (assuming Players as default for debug)
        EFlowIntent CurrentIntent = EFlowIntent::Players;
        
        // Get epochs
        int32 StaticCostEpoch = TileContext->GetStaticCostEpoch();
        int32 GoalsEpoch = TileContext->GetGoalsEpoch(CurrentIntent);
        int32 FlowEpoch = TileContext->GetFlowEpoch(CurrentIntent);
        
        // Check dirty flags (respetando el pipeline: usamos IsDirty del TileContext)
        bool bIsDirty = TileContext->IsDirty();
        bool bStaticCostDirty = bIsDirty; // en MVP, dirty proviene de StaticCost
        
        // Read built meta from storage
        int32 BuiltS = -1, BuiltG = -1;
        const bool HasBuilt = Grid::Flow::GetBuiltMeta(TileCoord, CurrentIntent, BuiltS, BuiltG);

        // Log the information
        UE_LOG(LogTemp, Display, TEXT("=== Tile Info (%d,%d) ==="), TX, TY);
        UE_LOG(LogTemp, Display, TEXT("Epochs - StaticCost: %d, Goals: %d, Flow: %d"), 
            StaticCostEpoch, GoalsEpoch, FlowEpoch);
        UE_LOG(LogTemp, Display, TEXT("Dirty - Overall: %s, StaticCost: %s"), 
            bIsDirty ? TEXT("YES") : TEXT("NO"),
            bStaticCostDirty ? TEXT("YES") : TEXT("NO"));
        UE_LOG(LogTemp, Display, TEXT("Storage Built - Static:%d Goals:%d%s"), BuiltS, BuiltG, HasBuilt?TEXT(""):TEXT(" (no data)"));
        UE_LOG(LogTemp, Display, TEXT("========================"));
    })
);

// ff.valid tx ty
static FAutoConsoleCommand GCmdFFValid(
    TEXT("ff.valid"),
    TEXT("Check if flow field is valid for tile: ff.valid <tx> <ty>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: ff.valid <tx> <ty>")); 
            return; 
        }

        int32 TX = 0, TY = 0;
        LexFromString(TX, *Args[0]);
        LexFromString(TY, *Args[1]);

        const FIntPoint TileCoord(TX, TY);
        
        // Get TileContext
         TSharedPtr TileContext = Grid::Tiles::GetTileContext(TileCoord);
        if (!TileContext.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("No TileContext found for tile (%d,%d)"), TX, TY);
            return;
        }

        // Check if valid
        EFlowIntent CurrentIntent = EFlowIntent::Players; // Default intent for debug
        bool bIsValid = Grid::Flow::IsValid(TileCoord, CurrentIntent, TileContext->GetStaticCostEpoch(), TileContext->GetGoalsEpoch(CurrentIntent));
        
        // Log the result
        UE_LOG(LogTemp, Display, TEXT("=== FlowField Validity (%d,%d) ==="), TX, TY);
        UE_LOG(LogTemp, Display, TEXT("Intent: Players"));
        UE_LOG(LogTemp, Display, TEXT("Is Valid: %s"), bIsValid ? TEXT("YES") : TEXT("NO"));
        
        // Get epochs for more context
        int32 StaticCostEpoch = TileContext->GetStaticCostEpoch();
        int32 GoalsEpoch = TileContext->GetGoalsEpoch(CurrentIntent);
        int32 FlowEpoch = TileContext->GetFlowEpoch(CurrentIntent);
        
        UE_LOG(LogTemp, Display, TEXT("Current Epochs - StaticCost: %d, Goals: %d, Flow: %d"), 
            StaticCostEpoch, GoalsEpoch, FlowEpoch);
        
        UE_LOG(LogTemp, Display, TEXT("================================"));
    })
);

// grid.flow.spawn x y
static FAutoConsoleCommand GCmdGridFlowSpawn(
    TEXT("grid.flow.spawn"),
    TEXT("Spawn FlowField debug actor (without auto-movement): grid.flow.spawn <x> <y>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.spawn <x> <y>")); 
            return; 
        }
        
        int32 X = 0, Y = 0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        
        // Obtener el mundo actual
        if (GWorld)
        {
            // Convertir coordenadas de celda a mundo
            const FVector2D WorldPos2D = GridWorld::CellToWorldCenterXY(FIntPoint(X, Y));
            const FVector WorldPosition = FVector(WorldPos2D.X, WorldPos2D.Y, 0.0f);
            
            // Spawnear el debug actor
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            
            AFlowFieldDebugActor* DebugActor = GWorld->SpawnActor<AFlowFieldDebugActor>(
                AFlowFieldDebugActor::StaticClass(), 
                WorldPosition, 
                FRotator::ZeroRotator, 
                SpawnParams
            );
            
            if (DebugActor)
            {
                // Deshabilitar el movimiento por defecto
                UFlowFieldMovementComponent* MovementComp = DebugActor->FindComponentByClass<UFlowFieldMovementComponent>();
                if (MovementComp)
                {
                    MovementComp->SetMovementEnabled(false);
                }
                
                // Agregar a la lista de actores
                GDebugActors.Add(DebugActor);
                
                UE_LOG(LogTemp, Log, TEXT("FlowField debug actor spawned at cell (%d,%d) -> world (%.1f, %.1f, %.1f). Movement is disabled by default."), 
                    X, Y, WorldPosition.X, WorldPosition.Y, WorldPosition.Z);
                UE_LOG(LogTemp, Log, TEXT("Use 'grid.flow.start' to start movement and 'grid.flow.stop' to stop it."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to spawn FlowField debug actor"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// grid.flow.start
static FAutoConsoleCommand GCmdGridFlowStartMovement(
    TEXT("grid.flow.start"),
    TEXT("Start movement for all spawned FlowField debug actors: grid.flow.start"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        int32 EnabledCount = 0;
        
        // Eliminar actores nulos
        GDebugActors.RemoveAll([](const TWeakObjectPtr<AFlowFieldDebugActor>& Actor) {
            return !Actor.IsValid();
        });
        
        // Habilitar movimiento para todos los actores
        for (const auto& ActorPtr : GDebugActors)
        {
            if (ActorPtr.IsValid())
            {
                UFlowFieldMovementComponent* MovementComp = ActorPtr->FindComponentByClass<UFlowFieldMovementComponent>();
                if (MovementComp)
                {
                    MovementComp->SetMovementEnabled(true);
                    EnabledCount++;
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("Enabled movement for %d FlowField debug actors"), EnabledCount);
    })
);

// grid.flow.stop
static FAutoConsoleCommand GCmdGridFlowStopMovement(
    TEXT("grid.flow.stop"),
    TEXT("Stop movement for all spawned FlowField debug actors: grid.flow.stop"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        int32 DisabledCount = 0;
        
        // Eliminar actores nulos
        GDebugActors.RemoveAll([](const TWeakObjectPtr<AFlowFieldDebugActor>& Actor) {
            return !Actor.IsValid();
        });
        
        // Deshabilitar movimiento para todos los actores
        for (const auto& ActorPtr : GDebugActors)
        {
            if (ActorPtr.IsValid())
            {
                UFlowFieldMovementComponent* MovementComp = ActorPtr->FindComponentByClass<UFlowFieldMovementComponent>();
                if (MovementComp)
                {
                    MovementComp->SetMovementEnabled(false);
                    DisabledCount++;
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("Disabled movement for %d FlowField debug actors"), DisabledCount);
    })
);

// grid.flow.spawn.multiple x y count
static FAutoConsoleCommand GCmdGridFlowSpawnMultiple(
    TEXT("grid.flow.spawn.multiple"),
    TEXT("Spawn multiple FlowField debug actors in a cell: grid.flow.spawn.multiple <x> <y> <count> [spacing=50.0]"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 3) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.spawn.multiple <x> <y> <count> [spacing=50.0]")); 
            return; 
        }
        
        int32 X = 0, Y = 0, Count = 1;
        float Spacing = 50.0f;
        
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        LexFromString(Count, *Args[2]);
        
        if (Args.Num() > 3)
        {
            LexFromString(Spacing, *Args[3]);
        }
        
        // Limitar el número máximo de actores a generar
        Count = FMath::Clamp(Count, 1, 100);
        
        // Obtener el mundo actual
        if (GWorld)
        {
            // Convertir coordenadas de celda a mundo
            const FVector2D CenterWorldPos2D = GridWorld::CellToWorldCenterXY(FIntPoint(X, Y));
            const FVector CenterWorldPosition = FVector(CenterWorldPos2D.X, CenterWorldPos2D.Y, 0.0f);
            
            // Calcular el desplazamiento total basado en el conteo y el espaciado
            const int32 GridSize = FMath::CeilToInt(FMath::Sqrt(static_cast<float>(Count)));
            const float StartOffset = -((GridSize - 1) * Spacing) / 2.0f;
            
            int32 SpawnedCount = 0;
            
            // Generar actores en un patrón de cuadrícula
            for (int32 i = 0; i < GridSize && SpawnedCount < Count; i++)
            {
                for (int32 j = 0; j < GridSize && SpawnedCount < Count; j++, SpawnedCount++)
                {
                    // Calcular posición relativa
                    const float OffsetX = StartOffset + (i * Spacing);
                    const float OffsetY = StartOffset + (j * Spacing);
                    
                    const FVector SpawnPosition = CenterWorldPosition + FVector(OffsetX, OffsetY, 0.0f);
                    
                    // Spawnear el debug actor
                    FActorSpawnParameters SpawnParams;
                    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                    
                    AFlowFieldDebugActor* DebugActor = GWorld->SpawnActor<AFlowFieldDebugActor>(
                        AFlowFieldDebugActor::StaticClass(), 
                        SpawnPosition, 
                        FRotator::ZeroRotator, 
                        SpawnParams
                    );
                    
                    if (DebugActor)
                    {
                        // Deshabilitar el movimiento por defecto
                        UFlowFieldMovementComponent* MovementComp = DebugActor->FindComponentByClass<UFlowFieldMovementComponent>();
                        if (MovementComp)
                        {
                            MovementComp->SetMovementEnabled(false);
                        }
                        
                        // Agregar a la lista de actores
                        GDebugActors.Add(DebugActor);
                    }
                }
            }
            
            UE_LOG(LogTemp, Log, TEXT("Spawned %d FlowField debug actors in cell (%d,%d) with %.1f spacing"), 
                SpawnedCount, X, Y, Spacing);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// grid.flow.clear.cell x y
static FAutoConsoleCommand GCmdGridFlowClearCell(
    TEXT("grid.flow.clear.cell"),
    TEXT("Remove all FlowField debug actors from a cell: grid.flow.clear.cell <x> <y>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.clear.cell <x> <y>")); 
            return; 
        }
        
        int32 X = 0, Y = 0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        
        const FIntPoint TargetCell(X, Y);
        
        // Obtener el mundo actual
        if (GWorld)
        {
            // Obtener actores en la celda
            TArray<AFlowFieldDebugActor*> ActorsToRemove = GetActorsInCell(TargetCell);
            
            // Eliminar actores
            for (AFlowFieldDebugActor* Actor : ActorsToRemove)
            {
                if (Actor)
                {
                    // Eliminar de la lista global
                    GDebugActors.Remove(Actor);
                    
                    // Destruir el actor
                    Actor->Destroy();
                }
            }
            
            UE_LOG(LogTemp, Log, TEXT("Removed %d FlowField debug actors from cell (%d,%d)"), 
                ActorsToRemove.Num(), X, Y);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);

// grid.flow.reset
static FAutoConsoleCommand GCmdGridFlowReset(
    TEXT("grid.flow.reset"),
    TEXT("Reset FlowField goals and clear flow field data"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        if (GWorld)
        {
            UGridEpochSubsystem* Subsystem = GWorld->GetSubsystem<UGridEpochSubsystem>();
            if (Subsystem)
            {
                // Limpiar metas
                Subsystem->Goals.GoalCells.Empty();

                // Limpiar almacenamiento del flow (dist/dir)
                Grid::Flow::ClearAll();

                UE_LOG(LogTemp, Log, TEXT("FlowField reset: cleared goals and storage"));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Failed to get UGridEpochSubsystem"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("No valid world context"));
        }
    })
);
