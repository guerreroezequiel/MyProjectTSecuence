// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiCoreFragment.generated.h"

/**
 * Fragmento core unificado para transformación y movimiento
 * Optimizado para cache locality y rendimiento con 10,000+ entidades
 */
USTRUCT()
struct FZombiCoreFragment : public FMassFragment
{
    GENERATED_BODY()

    // Transform (16 bytes)
    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator;

    // Velocidad (16 bytes)
    UPROPERTY()
    FVector MovementDirection = FVector::ForwardVector;

    UPROPERTY()
    float MovementSpeed = 0.0f;

    UPROPERTY()
    float RotationSpeed = 180.0f;

    // Comportamiento básico (12 bytes)
    UPROPERTY()
    float BehaviorTimer = 0.0f;

    UPROPERTY()
    float DirectionChangeInterval = 3.0f;

    UPROPERTY()
    FVector MovementCenter = FVector::ZeroVector;

    UPROPERTY()
    float MovementRadius = 500.0f;

    // Constructor con valores por defecto
    FZombiCoreFragment()
    {
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        MovementDirection = FVector::ForwardVector;
        MovementSpeed = 0.0f;
        RotationSpeed = 180.0f;
        BehaviorTimer = 0.0f;
        DirectionChangeInterval = 3.0f;
        MovementCenter = FVector::ZeroVector;
        MovementRadius = 500.0f;
    }

    // Constructor con parámetros
    FZombiCoreFragment(const FVector &InPosition, const FRotator &InRotation, float InMovementRadius = 500.0f)
    {
        Position = InPosition;
        Rotation = InRotation;
        MovementDirection = InRotation.Vector();
        MovementSpeed = 0.0f;
        RotationSpeed = 180.0f;
        BehaviorTimer = 0.0f;
        DirectionChangeInterval = 3.0f;
        MovementCenter = InPosition;
        MovementRadius = InMovementRadius;
    }
};