#include "GridSystem/FlowField/FlowFieldStorageSubsystem.h"
#include "Engine/World.h"
#include "GridSystem/Core/GridConfig.h"

UFlowFieldStorageSubsystem* UFlowFieldStorageSubsystem::Get(const UWorld* World)
{
	return World ? World->GetSubsystem<UFlowFieldStorageSubsystem>() : nullptr;
}

void UFlowFieldStorageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UFlowFieldStorageSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

Grid::Flow::FFieldView UFlowFieldStorageSubsystem::TryGetFieldView(const FIntPoint& TileXY, EFlowIntent Intent) const
{
	// Intentar leer desde el storage per-World primero
	Grid::Flow::FFieldView View;
	if (const TMap<EFlowIntent, FFieldSnapshot>* ByIntent = Storage.Find(TileXY))
	{
		if (const FFieldSnapshot* Snap = ByIntent->Find(Intent))
		{
			View.DirPtr = &Snap->Dir;
			View.StaticCostEpoch = Snap->StaticCostEpoch;
			View.GoalsEpoch = Snap->GoalsEpoch;
			View.Epoch = FMath::Max(View.StaticCostEpoch, View.GoalsEpoch);
			const bool bEpochsReady = (View.StaticCostEpoch >= 0) && (View.GoalsEpoch >= 0);
			const int32 ExpectedCells = GridConfig::TileDim * GridConfig::TileDim;
			const bool bSizeOK = (View.DirPtr && View.DirPtr->Num() == ExpectedCells);
			View.bValid = bEpochsReady && bSizeOK;
			if (View.bValid)
			{
				return View;
			}
		}
	}

	// Fallback al storage global existente
	return Grid::Flow::TryGetFieldView(TileXY, Intent);
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	FFieldSnapshot& Slot = Storage.FindOrAdd(TileXY).FindOrAdd(Intent);
	Slot.Dir = Dir; // copia defensiva; optimizable con MoveTemp si el caller puede ceder propiedad
	Slot.StaticCostEpoch = StaticCostEpoch;
	Slot.GoalsEpoch = GoalsEpoch;
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, const TArray<float>& Dist, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	FFieldSnapshot& Slot = Storage.FindOrAdd(TileXY).FindOrAdd(Intent);
	Slot.Dir = Dir;
	Slot.Dist = Dist;
	Slot.StaticCostEpoch = StaticCostEpoch;
	Slot.GoalsEpoch = GoalsEpoch;
}
