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
	Grid::Flow::FFieldView View;
	if (const TMap<EFlowIntent, TSharedPtr<const FFieldSnapshot>>* ByIntent = Storage.Find(TileXY))
	{
		if (const TSharedPtr<const FFieldSnapshot>* SnapPtr = ByIntent->Find(Intent))
		{
			const TSharedPtr<const FFieldSnapshot>& Snap = *SnapPtr;
			if (Snap.IsValid())
			{
				View.DirPtr = &Snap->Dir;
				View.StaticCostEpoch = Snap->StaticCostEpoch;
				View.GoalsEpoch = Snap->GoalsEpoch;
				View.Epoch = FMath::Max(View.StaticCostEpoch, View.GoalsEpoch);
				const bool bEpochsReady = (View.StaticCostEpoch >= 0) && (View.GoalsEpoch >= 0);
				const int32 ExpectedCells = GridConfig::TileDim * GridConfig::TileDim;
				const bool bSizeOK = (View.DirPtr && View.DirPtr->Num() == ExpectedCells);
				View.bValid = bEpochsReady && bSizeOK;
			}
		}
	}
	return View;
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	TSharedPtr<FFieldSnapshot> NewSnap = MakeShared<FFieldSnapshot>();
	NewSnap->Dir = Dir;
	NewSnap->StaticCostEpoch = StaticCostEpoch;
	NewSnap->GoalsEpoch = GoalsEpoch;
	Storage.FindOrAdd(TileXY).FindOrAdd(Intent) = NewSnap;
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, const TArray<float>& Dist, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	TSharedPtr<FFieldSnapshot> NewSnap = MakeShared<FFieldSnapshot>();
	NewSnap->Dir = Dir;
	NewSnap->Dist = Dist;
	NewSnap->StaticCostEpoch = StaticCostEpoch;
	NewSnap->GoalsEpoch = GoalsEpoch;
	Storage.FindOrAdd(TileXY).FindOrAdd(Intent) = NewSnap;
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, TArray<FVector2D>&& Dir, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	TSharedPtr<FFieldSnapshot> NewSnap = MakeShared<FFieldSnapshot>();
	NewSnap->Dir = MoveTemp(Dir);
	NewSnap->StaticCostEpoch = StaticCostEpoch;
	NewSnap->GoalsEpoch = GoalsEpoch;
	Storage.FindOrAdd(TileXY).FindOrAdd(Intent) = NewSnap;
}

void UFlowFieldStorageSubsystem::Publish(const FIntPoint& TileXY, EFlowIntent Intent, TArray<FVector2D>&& Dir, TArray<float>&& Dist, int32 StaticCostEpoch, int32 GoalsEpoch)
{
	TSharedPtr<FFieldSnapshot> NewSnap = MakeShared<FFieldSnapshot>();
	NewSnap->Dir = MoveTemp(Dir);
	NewSnap->Dist = MoveTemp(Dist);
	NewSnap->StaticCostEpoch = StaticCostEpoch;
	NewSnap->GoalsEpoch = GoalsEpoch;
	Storage.FindOrAdd(TileXY).FindOrAdd(Intent) = NewSnap;
}
