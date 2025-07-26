// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiMovementFragment.generated.h"

// Fragmento que maneja el movimiento y rotación del zombi para Mass Entity
// OPTIMIZADO para cache locality y rendimiento con miles de entidades
USTRUCT(BlueprintType)
struct FZombiMovementFragment : public FMassFragment
{
	GENERATED_BODY()

	// Datos de transformación (accedidos juntos frecuentemente)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector Position = FVector::ZeroVector; // 12 bytes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator Rotation = FRotator::ZeroRotator; // 12 bytes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MovementDirection = FVector::ForwardVector; // 12 bytes

	// Datos de velocidad (accedidos juntos frecuentemente)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementSpeed = 100.0f; // 4 bytes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float RotationSpeed = 360.0f; // 4 bytes

	// Datos de comportamiento (accedidos juntos frecuentemente)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DirectionChangeTimer = 0.0f; // 4 bytes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DirectionChangeInterval = 3.0f; // 4 bytes

	// Datos de área (accedidos juntos frecuentemente)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MovementRadius = 500.0f; // 4 bytes

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MovementCenter = FVector::ZeroVector; // 12 bytes

	// Total: 68 bytes, optimizado para cache lines de 64 bytes
};
