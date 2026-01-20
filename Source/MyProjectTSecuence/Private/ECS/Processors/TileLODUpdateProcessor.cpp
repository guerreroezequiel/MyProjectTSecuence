#include "ECS/Processors/TileLODUpdateProcessor.h"

#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GridSystem/Core/GridWorld.h"
// For execution order constraints
#include "ECS/Processors/UpdateCellLocationProcessor.h"
#include "ECS/Processors/FlowDirReadPlayersProcessor.h"

UTileLODUpdateProcessor::UTileLODUpdateProcessor()
{
	bAutoRegisterWithProcessingPhases = true;
	ProcessingPhase = EMassProcessingPhase::PrePhysics;
	// Debe ejecutar después de UpdateCellLocation (para tener TileXY válido)
	ExecutionOrder.ExecuteAfter.Add(UUpdateCellLocationProcessor::StaticClass()->GetFName());
	// Debe ejecutar antes de FlowDirRead (para que el LOD esté disponible)
	ExecutionOrder.ExecuteBefore.Add(UFlowDirReadPlayersProcessor::StaticClass()->GetFName());
	UE_LOG(LogTemp, Log, TEXT("TileLODUpdateProcessor: Initialized (Phase=PrePhysics)"));
}

void UTileLODUpdateProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FCellLocationFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTileLODFragment>(EMassFragmentAccess::ReadWrite);
	EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
	EntityQuery.RegisterWithProcessor(*this);
}

void UTileLODUpdateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* World = EntityManager.GetWorld();
	if (!World) return;

	const APlayerController* PC = World->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		return;
	}

	const FVector PlayerPos = Pawn->GetActorLocation();
	const FIntPoint PlayerTile = GridWorld::WorldToTileXY(PlayerPos);

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [PlayerTile](FMassExecutionContext& Ctx)
	{
		const TConstArrayView<FCellLocationFragment> CellLocs = Ctx.GetFragmentView<FCellLocationFragment>();
		const TArrayView<FTileLODFragment> LODs = Ctx.GetMutableFragmentView<FTileLODFragment>();

		const int32 Num = Ctx.GetNumEntities();
		for (int32 i = 0; i < Num; ++i)
		{
			FTileLODFragment& LOD = LODs[i];
			const FCellLocationFragment& Cell = CellLocs[i];

			if (!Cell.bValid)
			{
				LOD.LOD = ETileLOD::Cold;
				continue;
			}

			const FIntPoint TileXY = Cell.TileXY;

			if (TileXY == PlayerTile)
			{
				LOD.LOD = ETileLOD::Hot;
				continue;
			}

			const int dx = FMath::Abs(TileXY.X - PlayerTile.X);
			const int dy = FMath::Abs(TileXY.Y - PlayerTile.Y);
			if (dx <= 1 && dy <= 1)
			{
				LOD.LOD = ETileLOD::Warm;
			}
			else
			{
				LOD.LOD = ETileLOD::Cold;
			}
		}
	});
}
