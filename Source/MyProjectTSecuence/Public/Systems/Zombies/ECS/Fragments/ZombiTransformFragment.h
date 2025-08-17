// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiTransformFragment.generated.h"

/**
 * Fragmento optimizado para transformación de zombies
 * OPTIMIZADO: 16 bytes para mejor cache locality
 * DOP: Solo datos de transformación, sin lógica de comportamiento
 */
USTRUCT()
struct FZombiTransformFragment : public FMassFragment
{
    GENERATED_BODY()

    // Posición en el mundo (12 bytes)
    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    // Rotación comprimida (4 bytes)
    UPROPERTY()
    uint16 Yaw = 0; // 0-65535 (0-360 grados) - 16 bits de precisión
    UPROPERTY()
    uint16 Reserved = 0; // Padding para alineación de 16 bytes

    // Constructor por defecto
    FZombiTransformFragment()
    {
        Position = FVector::ZeroVector;
        Yaw = 0;
        Reserved = 0;
    }

    // Constructor con parámetros
    FZombiTransformFragment(const FVector &InPosition, const FRotator &InRotation)
    {
        Position = InPosition;
        Yaw = static_cast<uint16>(FMath::RoundToInt(InRotation.Yaw) & 0xFFFF);
        Reserved = 0;
    }

    // Constructor con parámetros (sobrecarga para compatibilidad)
    FZombiTransformFragment(const FVector &InPosition, float InYaw)
    {
        Position = InPosition;
        Yaw = static_cast<uint16>(FMath::RoundToInt(InYaw) & 0xFFFF);
        Reserved = 0;
    }

    // Getters optimizados
    FORCEINLINE FVector GetPosition() const { return Position; }
    FORCEINLINE float GetYaw() const { return static_cast<float>(Yaw); }
    FORCEINLINE FRotator GetRotation() const { return FRotator(0.0f, GetYaw(), 0.0f); }
    FORCEINLINE FVector GetForwardVector() const { return GetRotation().Vector(); }

    // Setters optimizados
    FORCEINLINE void SetPosition(const FVector &InPosition) { Position = InPosition; }
    FORCEINLINE void SetYaw(float InYaw) { Yaw = static_cast<uint16>(FMath::RoundToInt(InYaw) & 0xFFFF); }
    FORCEINLINE void SetRotation(const FRotator &InRotation) { SetYaw(InRotation.Yaw); }

    // Utilidades para transformación
    FORCEINLINE void AddPosition(const FVector &Delta) { Position += Delta; }
    FORCEINLINE void AddYaw(float DeltaYaw) { SetYaw(GetYaw() + DeltaYaw); }

    // Interpolación optimizada
    FORCEINLINE void InterpolateTo(const FVector &TargetPosition, float Alpha)
    {
        Position = FMath::Lerp(Position, TargetPosition, Alpha);
    }

    FORCEINLINE void InterpolateYawTo(float TargetYaw, float Alpha)
    {
        float CurrentYaw = GetYaw();
        float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrentYaw, TargetYaw);
        SetYaw(CurrentYaw + DeltaYaw * Alpha);
    }

    // Validación
    FORCEINLINE bool IsValid() const
    {
        return !Position.ContainsNaN() && Position.Size() < 10000.0f;
    }
};
