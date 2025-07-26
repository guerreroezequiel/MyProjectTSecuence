// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiBehaviorFragment.generated.h"

// Fragmento especializado para comportamiento y timers
// OPTIMIZADO para cache locality - datos de comportamiento juntos
USTRUCT(BlueprintType)
struct FZombiBehaviorFragment : public FMassFragment
{
    GENERATED_BODY()

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

    // Total: 24 bytes, optimizado para cache lines de 32 bytes
};