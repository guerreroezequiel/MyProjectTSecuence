// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiTransformFragment.generated.h"

// Fragmento especializado para transformaciones (posición y rotación)
// OPTIMIZADO para cache locality - datos accedidos juntos frecuentemente
USTRUCT(BlueprintType)
struct FZombiTransformFragment : public FMassFragment
{
    GENERATED_BODY()

    // Datos de transformación (accedidos juntos frecuentemente)
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector; // 12 bytes

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator Rotation = FRotator::ZeroRotator; // 12 bytes

    // Total: 24 bytes, optimizado para cache lines de 32 bytes
};