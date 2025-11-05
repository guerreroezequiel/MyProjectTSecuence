#include "CoreMinimal.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GridSystem/FlowField/FlowFieldRebuilder.h" // MarkTileDirty
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"
#include "GridSystem/Occupancy/CapacityGrid.h"
#include "GridSystem/Debug/GridDebugDrawComponent.h"

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


// grid.debug.cell
static FAutoConsoleCommand GCmdGridDebugCell(
    TEXT("grid.debug.cell"),
    TEXT("Debug a specific cell: grid.debug.cell <x> <y> (use -1 -1 to disable)"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 2) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.debug.cell <x> <y>")); 
            return; 
        }
        
        int32 X = 0, Y = 0;
        LexFromString(X, *Args[0]);
        LexFromString(Y, *Args[1]);
        
        // Encontrar el componente de debug en el mundo
        UWorld* World = GEngine->GetWorld();
        if (!World) return;
        
        // Usar TArray para encontrar actores con el componente
        TArray<AActor*> Actors;
        UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
        
        for (AActor* Actor : Actors)
        {
            if (UGridDebugDrawComponent* DebugComp = Actor->FindComponentByClass<UGridDebugDrawComponent>())
            {
                if (X >= 0 && Y >= 0)
                {
                    DebugComp->SetDebugCell(X, Y);
                    UE_LOG(LogTemp, Log, TEXT("Debugging cell (%d, %d)"), X, Y);
                }
                else
                {
                    DebugComp->SetDebugCell(INDEX_NONE, INDEX_NONE);
                    UE_LOG(LogTemp, Log, TEXT("Cell debugging disabled"));
                }
                return;
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("No GridDebugDrawComponent found in the world"));
    })
);

// grid.debug.capacity
static FAutoConsoleCommand GCmdGridDebugCapacity(
    TEXT("grid.debug.capacity"),
    TEXT("Toggle drawing of capacity grid: grid.debug.capacity <0|1>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 1) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.debug.capacity <0|1>")); 
            return; 
        }
        
        int32 bEnable = 0;
        LexFromString(bEnable, *Args[0]);
        
        // Encontrar el componente de debug en el mundo
        UWorld* World = GEngine->GetWorld();
        if (!World) return;
        
        // Buscar específicamente el actor BP_GridDebugManager
        AActor* DebugActor = FindObject<AActor>(nullptr, TEXT("/Game/BP_GridDebugManager.BP_GridDebugManager_C"));
        if (!DebugActor)
        {
            // Si no lo encuentra por ruta, intentar encontrarlo en el mundo
            TArray<AActor*> Actors;
            UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
            for (AActor* Actor : Actors)
            {
                if (Actor->GetName().StartsWith(TEXT("BP_GridDebugManager")))
                {
                    DebugActor = Actor;
                    break;
                }
            }
        }

        if (DebugActor)
        {
            if (UGridDebugDrawComponent* DebugComp = DebugActor->FindComponentByClass<UGridDebugDrawComponent>())
            {
                DebugComp->bDrawCapacity = (bEnable != 0);
                UE_LOG(LogTemp, Log, TEXT("Capacity drawing %s on %s"), 
                    DebugComp->bDrawCapacity ? TEXT("ENABLED") : TEXT("DISABLED"),
                    *DebugActor->GetName());
                return;
            }
        }
        
        UE_LOG(LogTemp, Warning, TEXT("BP_GridDebugManager or GridDebugDrawComponent not found in the world"));
    })
);

// grid.debug.tileborder
static FAutoConsoleCommand GCmdGridDebugTileBorder(
    TEXT("grid.debug.tileborder"),
    TEXT("Toggle drawing of tile borders: grid.debug.tileborder <0|1>"),
    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
    {
        if (Args.Num() < 1) 
        { 
            UE_LOG(LogTemp, Warning, TEXT("Usage: grid.debug.tileborder <0|1>")); 
            return; 
        }
        
        int32 bEnable = 0;
        LexFromString(bEnable, *Args[0]);
        
        // Obtener el mundo actual
        UWorld* World = GEngine->GetWorld();
        if (!World) 
        {
            UE_LOG(LogTemp, Warning, TEXT("No valid world found"));
            return;
        }
        
        // Buscar específicamente el actor BP_GridDebugManager
        AActor* DebugActor = FindObject<AActor>(nullptr, TEXT("/Game/BP_GridDebugManager.BP_GridDebugManager_C"));
        if (!DebugActor)
        {
            // Si no lo encuentra por ruta, intentar encontrarlo en el mundo
            TArray<AActor*> Actors;
            UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
            for (AActor* Actor : Actors)
            {
                if (Actor->GetName().StartsWith(TEXT("BP_GridDebugManager")))
                {
                    DebugActor = Actor;
                    break;
                }
            }
        }

        if (DebugActor)
        {
            if (UGridDebugDrawComponent* DebugComp = DebugActor->FindComponentByClass<UGridDebugDrawComponent>())
            {
                DebugComp->bDrawTileBorder = (bEnable != 0);
                UE_LOG(LogTemp, Log, TEXT("Tile border drawing %s on %s"), 
                    DebugComp->bDrawTileBorder ? TEXT("ENABLED") : TEXT("DISABLED"),
                    *DebugActor->GetName());
                return;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("GridDebugDrawComponent not found on %s"), *DebugActor->GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("BP_GridDebugManager not found in the world"));
        }
    })
);
