#include "CoreMinimal.h"
#include "GridSystem/FlowField/FlowFieldRebuilder.h" // MarkTileDirty
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "GridSystem/FlowField/AFlowFieldDebugActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"
#include "GridSystem/Occupancy/CapacityGrid.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"

// Minimal console: re-enable only Capacity and Occupancy commands safely.
// Commands:
// - grid.cap.set_base x y v
// - grid.cap.set_count x y v
// - grid.cap.clear
// - grid.occ.set x y state (0=Empty,1=Obstacle,2=Portal)
// - grid.occ.clear

// grid.cap.set_base x y v
static FAutoConsoleCommand GCmdGridCapSetBase(
    TEXT("grid.cap.set_base"),
    TEXT("Set base capacity of a cell: grid.cap.set_base <x> <y> <value>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.cap.set_base <x> <y> <value>")); return; }
        int32 X=0, Y=0, V=0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        LexFromString(V, *Args[2]);
        const FIntPoint Cell(X,Y);
        Grid::Capacity::SetBaseCapacity(Cell, V);
        const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
        Grid::Flow::MarkTileDirty(TileXY);
        UE_LOG(LogTemp, Log, TEXT("Capacity base set at cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, V, TileXY.X, TileXY.Y);
    })
);

// grid.cap.set_count x y v
static FAutoConsoleCommand GCmdGridCapSetCount(
    TEXT("grid.cap.set_count"),
    TEXT("Set current count of a cell: grid.cap.set_count <x> <y> <value>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.cap.set_count <x> <y> <value>")); return; }
        int32 X=0, Y=0, V=0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        LexFromString(V, *Args[2]);
        const FIntPoint Cell(X,Y);
        Grid::Capacity::SetCurrentCount(Cell, V);
        const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
        Grid::Flow::MarkTileDirty(TileXY);
        UE_LOG(LogTemp, Log, TEXT("Capacity count set at cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, V, TileXY.X, TileXY.Y);
    })
);

// grid.cap.clear
static FAutoConsoleCommand GCmdGridCapClear(
    TEXT("grid.cap.clear"),
    TEXT("Clear entire capacity grid"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        Grid::Capacity::ClearAll();
        UE_LOG(LogTemp, Log, TEXT("Capacity cleared"));
    })
);

// grid.occ.set x y state
static FAutoConsoleCommand GCmdGridOccSet(
    TEXT("grid.occ.set"),
    TEXT("Set occupancy state: grid.occ.set <x> <y> <state:0|1|2> (0=Empty,1=Obstacle,2=Portal)"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.occ.set <x> <y> <state:0|1|2>")); return; }
        int32 X=0, Y=0, S=0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        LexFromString(S, *Args[2]);
        S = FMath::Clamp(S, 0, 2);
        Grid::ECellState State = static_cast<Grid::ECellState>(S);
        const FIntPoint Cell(X,Y);
        Grid::SetCellState(Cell, State);
        const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
        Grid::Flow::MarkTileDirty(TileXY);
        UE_LOG(LogTemp, Log, TEXT("Occupancy set cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, S, TileXY.X, TileXY.Y);
    })
);

// grid.occ.clear
static FAutoConsoleCommand GCmdGridOccClear(
    TEXT("grid.occ.clear"),
    TEXT("Clear entire occupancy grid"),
    FConsoleCommandDelegate::CreateStatic([]()
    {
        Grid::ClearAll();
        UE_LOG(LogTemp, Log, TEXT("Occupancy cleared"));
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
                
                // Marcar tiles cercanos como sucios para recalcular
                const FIntPoint TileXY = GridWorld::CellToTileXY(FIntPoint(X, Y));
                Grid::Flow::MarkTileDirty(TileXY);
                
                UE_LOG(LogTemp, Log, TEXT("FlowField goal set at cell (%d,%d); marked tile (%d,%d) dirty"), 
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

// grid.flow.spawn x y
static FAutoConsoleCommand GCmdGridFlowSpawn(
    TEXT("grid.flow.spawn"),
    TEXT("Spawn FlowField debug actor: grid.flow.spawn <x> <y>"),
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
                UE_LOG(LogTemp, Log, TEXT("FlowField debug actor spawned at cell (%d,%d) -> world (%.1f, %.1f, %.1f)"), 
                    X, Y, WorldPosition.X, WorldPosition.Y, WorldPosition.Z);
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
                
                UE_LOG(LogTemp, Log, TEXT("FlowField reset: cleared goals"));
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
