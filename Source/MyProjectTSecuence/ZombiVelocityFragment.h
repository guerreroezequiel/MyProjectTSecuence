// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiVelocityFragment.generated.h"

// Fragmento especializado para velocidad y dirección de movimiento
// OPTIMIZADO para cache locality - datos de movimiento juntos
USTRUCT(BlueprintType)
struct FZombiVelocityFragment : public FMassFragment
{
    GENERATED_BODY()

    // Datos de velocidad (accedidos juntos frecuentemente)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MovementSpeed = 100.0f; // 4 bytes

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RotationSpeed = 360.0f; // 4 bytes

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector MovementDirection = FVector::ForwardVector; // 12 bytes

    // Total: 20 bytes, optimizado para cache lines de 32 bytes
};