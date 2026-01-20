#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "FlowFieldStorageSubsystem.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UFlowFieldStorageSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UFlowFieldStorageSubsystem* Get(const UWorld* World);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	Grid::Flow::FFieldView TryGetFieldView(const FIntPoint& TileXY, EFlowIntent Intent) const;

	// Publicación de snapshot inmutable del DirField para un (TileXY, Intent)
	void Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, int32 StaticCostEpoch, int32 GoalsEpoch);

	// Overload opcional: publicar también Dist (cost-to-go) si está disponible
	void Publish(const FIntPoint& TileXY, EFlowIntent Intent, const TArray<FVector2D>& Dir, const TArray<float>& Dist, int32 StaticCostEpoch, int32 GoalsEpoch);

	// Overloads con move para evitar copias
	void Publish(const FIntPoint& TileXY, EFlowIntent Intent, TArray<FVector2D>&& Dir, int32 StaticCostEpoch, int32 GoalsEpoch);
	void Publish(const FIntPoint& TileXY, EFlowIntent Intent, TArray<FVector2D>&& Dir, TArray<float>&& Dist, int32 StaticCostEpoch, int32 GoalsEpoch);

private:
	// Snapshot inmutable por (TileXY, Intent) dentro de este World
	struct FFieldSnapshot
	{
		TArray<FVector2D> Dir;
		TArray<float> Dist;
		int32 StaticCostEpoch = -1;
		int32 GoalsEpoch = -1;
	};

	// Storage por World: TileXY -> (Intent -> Snapshot inmutable por puntero)
	TMap<FIntPoint, TMap<EFlowIntent, TSharedPtr<const FFieldSnapshot>>> Storage;
};
