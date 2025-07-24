// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiMovementFragment.generated.h"

// Fragmento que maneja el movimiento y rotación del zombi para Mass Entity
// Optimizado para rendimiento con muchos zombis
USTRUCT(BlueprintType)
struct FZombiMovementFragment : public FMassFragment
{
	GENERATED_BODY()

	// Posición actual del zombi
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector;

	// Rotación actual del zombi
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator;

	// Velocidad de movimiento (unidades por segundo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 100.0f;

	// Velocidad de rotación (grados por segundo)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSpeed = 90.0f;

	// Dirección de movimiento actual
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MovementDirection = FVector::ForwardVector;

	// Tiempo para cambiar dirección (para movimiento aleatorio)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DirectionChangeTimer = 0.0f;

	// Intervalo para cambiar dirección
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DirectionChangeInterval = 3.0f;

	// Radio de movimiento (para mantener zombis en área)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementRadius = 500.0f;

	// Centro del área de movimiento
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MovementCenter = FVector::ZeroVector;
};
