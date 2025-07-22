// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyTurboSequenceAnimComponent.h"
#include "ZombiTestActor.generated.h"

// Actor de prueba que demuestra la integración de TurboSequence + Mass Entity
UCLASS()
class MYPROJECTTSECUENCE_API AZombiTestActor : public AActor
{
	GENERATED_BODY()

public:
	AZombiTestActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	// Componente de animación TurboSequence
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UMyTurboSequenceAnimComponent *AnimComponent;

	// Asset de TurboSequence para este zombi
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UTurboSequence_MeshAsset_Lf *TSAsset;

	// Intervalo de tiempo para cambiar estados automáticamente
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test")
	float StateChangeInterval = 3.0f;

	// Cambia a un estado aleatorio (para testing)
	UFUNCTION(BlueprintCallable, Category = "Test")
	void ChangeToRandomState();

	// Inicializa el zombi con TurboSequence
	UFUNCTION(BlueprintCallable, Category = "Test")
	void InitializeZombi();

	// Registra este actor en el sistema Mass
	UFUNCTION(BlueprintCallable, Category = "Mass")
	void RegisterInMassSystem();

private:
	// Timer para cambiar estados automáticamente
	float StateTimer;

	// Lista de todos los estados disponibles
	TArray<EZombiState> AllStates;
	int32 CurrentStateIndex;

	// Indica si ya está registrado en Mass
	bool bRegisteredInMass;
};
