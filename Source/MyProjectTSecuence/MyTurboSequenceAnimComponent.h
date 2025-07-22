// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "MyTurboSequenceAnimComponent.generated.h"

// Enum que representa los posibles estados lógicos del zombi
UENUM(BlueprintType)
enum class EZombiState : uint8
{
	Idle,	// Quieto
	Walk,	// Caminando
	Chase,	// Persiguiendo
	Attack, // Atacando
	Hit,	// Recibiendo golpe
	Death	// Muerto
};

// Componente que administra la instancia visual de TurboSequence y su animación
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UMyTurboSequenceAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Constructor estándar
	UMyTurboSequenceAnimComponent();

	// Crea la instancia visual de TurboSequence en el mundo
	void InitializeTurboSequence(class UTurboSequence_MeshAsset_Lf *TSAsset, const FTransform &SpawnTransform);

	// Cambia la animación de la instancia visual (puedes llamarla desde Blueprint)
	UFUNCTION(BlueprintCallable, Category = "TurboSequence")
	void SetTurboSequenceAnimation(class UAnimSequence *NewAnimation);

	// Cambia el estado lógico y actualiza la animación correspondiente
	UFUNCTION(BlueprintCallable, Category = "TurboSequence")
	void SetState(EZombiState NewState);

	// Estado actual del zombi (Idle, Walk, etc.)
	UPROPERTY(BlueprintReadOnly, Category = "TurboSequence")
	EZombiState CurrentState = EZombiState::Idle;

	// Animaciones para cada estado (asignar en el editor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *IdleAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *WalkAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *ChaseAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *AttackAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *HitAnim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TurboSequence")
	UAnimSequence *DeathAnim;

protected:
	// Se llama automáticamente cuando el juego comienza o el actor aparece en el mundo
	virtual void BeginPlay() override;

private:
	// Referencia interna al manager de TurboSequence (no se usa directamente en este ejemplo)
	class ATurboSequence_Manager_Lf *TurboSequenceManager = nullptr;
	// Identificador de la instancia visual de TurboSequence
	FTurboSequence_MinimalMeshData_Lf MeshData;
};
