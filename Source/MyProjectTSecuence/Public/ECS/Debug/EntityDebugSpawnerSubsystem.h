#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Engine.h"
#include "MassEntityTypes.h"
#include "MassEntityView.h"
#include "MassCommonFragments.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/MoveFragment.h"
#include "ECS/Tags/ZombiTag.h"
#include "EntityDebugSpawnerSubsystem.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UEntityDebugSpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "EntityDebugSpawner")
	void SpawnEntities(int32 Count = 10, float Radius = 1000.0f, FVector Origin = FVector::ZeroVector);

	UFUNCTION(BlueprintCallable, Category = "EntityDebugSpawner")
	void ClearAllEntities();

	UFUNCTION(BlueprintCallable, Category = "EntityDebugSpawner|Debug")
	void SetDebugVisualization(bool bEnable);

	// In case initialization order prevents auto registration, expose a manual hook
	UFUNCTION(BlueprintCallable, Category = "EntityDebugSpawner|Debug")
	void ForceRegisterConsoleCommands();

private:
    // Console registration
    static TArray<FAutoConsoleCommand*> ConsoleCommands;
    static void RegisterConsoleCommands();
    static void UnregisterConsoleCommands();
    static void ExecuteSpawnEntities(const TArray<FString>& Args);
    static void ExecuteClearEntities(const TArray<FString>& Args);
    static void ExecuteToggleDebug(const TArray<FString>& Args);
    static void ExecuteReRegister(const TArray<FString>& Args);

    // NUEVOS: declarar handlers de consola
    static void ExecuteMarkTSForCleanup(const TArray<FString>& Args);
    static void ExecuteClearEntitiesSafe(const TArray<FString>& Args);

    static UWorld* ResolveActiveWorld();

	// Internal helpers
	void SetupArchetype();
	void UpdateDebugVisualization();

	// Cached Mass subsystem
	UPROPERTY(Transient)
	TObjectPtr<UMassEntitySubsystem> MassEntitySubsystem;

	FMassArchetypeHandle Archetype;
	TArray<FMassEntityHandle> SpawnedEntities;
	bool bDebugVisualizationEnabled = true;
	float DebugSphereRadius = 50.0f;
	FColor DebugSphereColor = FColor::Red;
	FTimerHandle DebugTimerHandle;
};
