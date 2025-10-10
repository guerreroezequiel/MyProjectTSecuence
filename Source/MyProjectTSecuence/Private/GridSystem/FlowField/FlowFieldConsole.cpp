//#if 0
//// FlowFieldConsole temporarily disabled. To be fixed and re-enabled.
//#include "CoreMinimal.h"
//#include "GridSystem/FlowField/FlowFieldRebuilder.h"
//#include "GridSystem/FlowField/FlowFieldSolver.h"
//#include "GridSystem/FlowField/FlowFieldStorage.h"
//#include "GridSystem/Core/GridWorld.h"
//#include "GridSystem/Density/DensityHeatGrid.h"
//#include "GridSystem/Occupancy/OccupancyGrid.h"
//#include "GridSystem/Occupancy/CapacityGrid.h"
//
//// Consola mínima para probar FlowField rebuild
//// Comandos:
//// - grid.flow.set_goal x y
//// - grid.flow.mark_tile tx ty
//// - grid.flow.rebuild_step [tiles] [cells]
//// - grid.flow.rebuild_now
//// - grid.flow.clear_all
//
//namespace Grid
//{
//	namespace Flow
//	{
//		static FGoalSet      GConsoleGoals;
//		static FSolverParams GConsoleParams; // usa defaults (T=0.2, etc.)
//static FAutoConsoleCommand GCmdGridFlowSetGoal(
//	TEXT("grid.flow.set_goal"),
//	TEXT("Setea una meta (celda) para el solver: grid.flow.set_goal <x> <y>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		using namespace Grid::Flow;
//		if (Args.Num() < 2) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.flow.set_goal <x> <y>")); return; }
//		int32 X = 0, Y = 0;
//		LexFromString(X, *Args[0]);
//
//// grid.flow.set_params T a b
//static FAutoConsoleCommand GCmdGridFlowSetParams(
//    TEXT("grid.flow.set_params"),
//    TEXT("Setea parámetros del solver: grid.flow.set_params <T> <Alphaheat> <BetaCapacity>"),
//    FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//    {
//        using namespace Grid::Flow;
//        if (Args.Num() >= 1) { LexFromString(GConsoleParams.T, *Args[0]); }
//        if (Args.Num() >= 2) { LexFromString(GConsoleParams.Alphaheat, *Args[1]); }
//        if (Args.Num() >= 3) { LexFromString(GConsoleParams.BetaCapacity, *Args[2]); }
//        UE_LOG(LogTemp, Log, TEXT("Flow params set: T=%.3f Alphaheat=%.3f BetaCapacity=%.3f"), GConsoleParams.T, GConsoleParams.Alphaheat, GConsoleParams.BetaCapacity);
//// ...
//    FConsoleCommandDelegate::CreateStatic([]()
//	{
//		using namespace Grid::Flow;
//		ClearAll();
//		GDirtyTiles.Reset();
//		while (!GDirtyQueue.IsEmpty()) { FIntPoint Tmp; GDirtyQueue.Dequeue(Tmp); }
//	})
//);
//
//// grid.heat.inject x y v
//static FAutoConsoleCommand GCmdGridHeatInject(
//// ...
//#endif // FlowFieldConsole disabled
//	TEXT("Inyecta heat en una celda: grid.heat.inject <x> <y> <value>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.heat.inject <x> <y> <value>")); return; }
//		int32 X=0, Y=0; float V=0;
//		LexFromString(X, *Args[0]);
//		LexFromString(Y, *Args[1]);
//		LexFromString(V, *Args[2]);
//		Grid::Density::AccumulateTraffic(FIntPoint(X,Y), V);
//		UE_LOG(LogTemp, Log, TEXT("Injected heat %.2f at cell (%d,%d)"), V, X, Y);
//	})
//);
//
//// grid.heat.decay dt
//static FAutoConsoleCommand GCmdGridHeatDecay(
//	TEXT("grid.heat.decay"),
//	TEXT("Aplica decay a todo el heat: grid.heat.decay <DeltaSeconds>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		if (Args.Num() < 1) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.heat.decay <DeltaSeconds>")); return; }
//		float Dt=0; LexFromString(Dt, *Args[0]);
//		Grid::Density::Decay(Dt);
//		UE_LOG(LogTemp, Log, TEXT("Heat decayed by Dt=%.3f"), Dt);
//	})
//);
//
//// grid.heat.clear
//static FAutoConsoleCommand GCmdGridHeatClear(
//	TEXT("grid.heat.clear"),
//	TEXT("Limpia todo el heat acumulado"),
//	FConsoleCommandDelegate::CreateStatic([]()
//	{
//		Grid::Density::ClearAll();
//		UE_LOG(LogTemp, Log, TEXT("All heat cleared"));
//	})
//);
//
//// grid.occ.set x y state (0=Empty,1=Obstacle,2=Portal)
//static FAutoConsoleCommand GCmdGridOccSet(
//	TEXT("grid.occ.set"),
//	TEXT("Setea el estado de una celda de occupancy: grid.occ.set <x> <y> <state:0|1|2>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.occ.set <x> <y> <state:0|1|2>")); return; }
//		int32 X=0, Y=0, S=0;
//		LexFromString(X, *Args[0]);
//		LexFromString(Y, *Args[1]);
//		LexFromString(S, *Args[2]);
//		S = FMath::Clamp(S, 0, 2);
//		Grid::ECellState State = static_cast<Grid::ECellState>(S);
//		const FIntPoint Cell(X,Y);
//		Grid::SetCellState(Cell, State);
//		const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
//		Grid::Flow::MarkTileDirty(TileXY);
//		UE_LOG(LogTemp, Log, TEXT("Occupancy set cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, S, TileXY.X, TileXY.Y);
//	})
//);
//
//// grid.occ.clear
//static FAutoConsoleCommand GCmdGridOccClear(
//	TEXT("grid.occ.clear"),
//	TEXT("Limpia toda la capa de occupancy"),
//	FConsoleCommandDelegate::CreateStatic([]()
//	{
//		Grid::ClearAll();
//		UE_LOG(LogTemp, Log, TEXT("Occupancy cleared"));
//	})
//);
//
//// grid.cap.set_base x y v
//static FAutoConsoleCommand GCmdGridCapSetBase(
//	TEXT("grid.cap.set_base"),
//	TEXT("Setea la capacidad base de una celda: grid.cap.set_base <x> <y> <value>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.cap.set_base <x> <y> <value>")); return; }
//		int32 X=0, Y=0, V=0;
//		LexFromString(X, *Args[0]);
//		LexFromString(Y, *Args[1]);
//		LexFromString(V, *Args[2]);
//		const FIntPoint Cell(X,Y);
//		Grid::Capacity::SetBaseCapacity(Cell, V);
//		const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
//		Grid::Flow::MarkTileDirty(TileXY);
//		UE_LOG(LogTemp, Log, TEXT("Capacity base set at cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, V, TileXY.X, TileXY.Y);
//	})
//);
//
//// grid.cap.set_count x y v
//static FAutoConsoleCommand GCmdGridCapSetCount(
//	TEXT("grid.cap.set_count"),
//	TEXT("Setea el conteo actual de una celda: grid.cap.set_count <x> <y> <value>"),
//	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args)
//	{
//		if (Args.Num() < 3) { UE_LOG(LogTemp, Warning, TEXT("Usage: grid.cap.set_count <x> <y> <value>")); return; }
//		int32 X=0, Y=0, V=0;
//		LexFromString(X, *Args[0]);
//		LexFromString(Y, *Args[1]);
//		LexFromString(V, *Args[2]);
//		const FIntPoint Cell(X,Y);
//		Grid::Capacity::SetCurrentCount(Cell, V);
//		const FIntPoint TileXY = GridWorld::CellToTileXY(Cell);
//		Grid::Flow::MarkTileDirty(TileXY);
//		UE_LOG(LogTemp, Log, TEXT("Capacity count set at cell (%d,%d) = %d; marked tile (%d,%d) dirty"), X, Y, V, TileXY.X, TileXY.Y);
//	})
//);
//
//// grid.cap.clear
//static FAutoConsoleCommand GCmdGridCapClear(
//	TEXT("grid.cap.clear"),
//	TEXT("Limpia toda la capa de capacidad"),
//	FConsoleCommandDelegate::CreateStatic([]()
//	{
//		Grid::Capacity::ClearAll();
//		UE_LOG(LogTemp, Log, TEXT("Capacity cleared"));
//	})
//);
