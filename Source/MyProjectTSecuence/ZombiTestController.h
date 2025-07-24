// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZombiTestController.generated.h"

// Forward declarations
class UZombiSpawnerSubsystem;
class UTurboSequence_MeshAsset_Lf;

// Actor de prueba para controlar el spawning de zombis desde Blueprint
UCLASS(BlueprintType, Blueprintable)
class MYPROJECTTSECUENCE_API AZombiTestController : public AActor
{
	GENERATED_BODY()

public:
	AZombiTestController();

protected:
	virtual void BeginPlay() override;

public:
	// Spawna un lote de zombis (llamable desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Zombi Test")
	void SpawnZombiBatch(int32 Count = 100);

	// Spawna un zombi individual (llamable desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Zombi Test")
	void SpawnSingleZombi();

	// Limpia todos los zombis (llamable desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Zombi Test")
	void ClearAllZombis();

	// Obtiene el número de zombis activos (llamable desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Zombi Test")
	int32 GetActiveZombiCount();

	// Configura el asset de TurboSequence (llamable desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "Zombi Test")
	void SetTurboSequenceAsset(UTurboSequence_MeshAsset_Lf *Asset);

	// Asset de TurboSequence para los zombis
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombi Test")
	UTurboSequence_MeshAsset_Lf *ZombiTurboSequenceAsset;

	// Centro de spawning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombi Test")
	FVector SpawnCenter = FVector::ZeroVector;

	// Radio de spawning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombi Test")
	float SpawnRadius = 500.0f;

	// Número de zombis por lote
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zombi Test")
	int32 ZombisPerBatch = 100;

private:
	// Referencia al subsystem de spawning
	UPROPERTY()
	UZombiSpawnerSubsystem *SpawnerSubsystem;

	// Timer para spawn inicial
	FTimerHandle SpawnTimerHandle;

	// Función para spawn inicial con delay
	void DelayedSpawn();

	// Tick function para ejecutar procesadores manualmente
	virtual void Tick(float DeltaTime) override;
};
