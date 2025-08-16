// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntitySubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUltraConsolidatedFragment.h"
#include "ZombiSpawnerSubsystem.generated.h"

// Subsystem optimizado para spawning masivo de zombis usando Mass Entity + TurboSequence
// Diseñado para manejar miles de entidades con máximo rendimiento
UCLASS()
class MYPROJECTTSECUENCE_API UZombiSpawnerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UZombiSpawnerSubsystem();

	// Inicialización del subsystem
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;
	virtual void Deinitialize() override;

	// Spawna un lote de zombis de manera optimizada
	UFUNCTION(BlueprintCallable, Category = "Zombi Spawner")
	void SpawnZombiBatch(int32 Count, const FVector &CenterLocation, float SpawnRadius);

	// Spawna un zombi individual
	UFUNCTION(BlueprintCallable, Category = "Zombi Spawner")
	void SpawnSingleZombi(const FVector &Location);

	// Configura el asset de TurboSequence para los zombis
	UFUNCTION(BlueprintCallable, Category = "Zombi Spawner")
	void SetZombiTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset);

	// Obtiene estadísticas de rendimiento
	UFUNCTION(BlueprintCallable, Category = "Zombi Spawner")
	int32 GetActiveZombiCount() const { return ActiveZombiCount; }

	// Limpia todos los zombis
	UFUNCTION(BlueprintCallable, Category = "Zombi Spawner")
	void ClearAllZombis();

	// Procesa reintentos de instancias visuales pendientes
	void ProcessPendingVisualInstances(float DeltaTime);

	// Asset de TurboSequence para los zombis (público para acceso desde procesadores)
	UPROPERTY()
	TObjectPtr<UTurboSequence_MeshAsset_Lf> ZombiTurboSequenceAsset;

private:

	// Referencia al MassEntitySubsystem
	UPROPERTY()
	TObjectPtr<UMassEntitySubsystem> MassEntitySubsystem;

	// Lista de entidades spawnadas
	TArray<FMassEntityHandle> SpawnedEntities;

	// Contador de zombis activos
	int32 ActiveZombiCount = 0;

	// Sistema de reintentos para instancias visuales
	TArray<FMassEntityHandle> PendingVisualInstances;
	float RetryTimer = 0.0f;
	const float RetryInterval = 0.1f; // Reintentar cada 0.1 segundos

	// Mapa de instancias visuales de TurboSequence (EntityHandle -> MeshData)
	TMap<FMassEntityHandle, FTurboSequence_MinimalMeshData_Lf> VisualInstances;

	// Crea una entidad Mass con todos los fragmentos necesarios
	FMassEntityHandle CreateZombiMassEntity(const FVector &SpawnLocation);

	// Configura el Blend Space para una entidad
	void ConfigureBlendSpaceForEntity(FZombiUltraConsolidatedFragment &UltraFragment);

	// Genera una ubicación de spawn aleatoria
	FVector GenerateRandomSpawnLocation(const FVector &Center, float Radius) const;

	// Genera una rotación aleatoria
	FRotator GenerateRandomRotation() const;

	// Crea la instancia visual de TurboSequence para una entidad
	void CreateTurboSequenceVisualInstance(FMassEntityHandle EntityHandle, const FVector &SpawnLocation);
};
