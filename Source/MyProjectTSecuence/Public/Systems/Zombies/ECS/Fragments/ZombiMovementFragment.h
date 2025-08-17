// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiMovementFragment.generated.h"

/**
 * Fragmento optimizado para movimiento de zombies
 * OPTIMIZADO: 16 bytes para mejor cache locality
 * DOP: Solo datos de movimiento, sin lógica de comportamiento
 */
USTRUCT()
struct FZombiMovementFragment : public FMassFragment
{
    GENERATED_BODY()

    // Dirección de movimiento (12 bytes)
    UPROPERTY()
    FVector Direction = FVector::ForwardVector;

    // Configuración de movimiento (4 bytes)
    UPROPERTY()
    uint8 MovementSpeed = 0;     // 0-255 (velocidad de movimiento)
    UPROPERTY()
    uint8 MovementFlags = 0;     // Flags de estado de movimiento
    UPROPERTY()
    uint8 BatchGroup = 0;        // Grupo de batch para optimización
    UPROPERTY()
    uint8 Reserved = 0;          // Padding para alineación de 16 bytes

    // Constructor por defecto
    FZombiMovementFragment()
    {
        Direction = FVector::ForwardVector;
        MovementSpeed = 0;
        MovementFlags = 0;
        BatchGroup = 0;
        Reserved = 0;
    }

    // Constructor con parámetros
    FZombiMovementFragment(const FVector& InDirection, uint8 InSpeed, uint8 InFlags = 0)
    {
        Direction = InDirection.GetSafeNormal();
        MovementSpeed = InSpeed;
        MovementFlags = InFlags;
        BatchGroup = 0;
        Reserved = 0;
    }

    // Getters optimizados
    FORCEINLINE FVector GetDirection() const { return Direction; }
    FORCEINLINE float GetSpeed() const { return static_cast<float>(MovementSpeed); }
    FORCEINLINE uint8 GetFlags() const { return MovementFlags; }
    FORCEINLINE uint8 GetBatchGroup() const { return BatchGroup; }

    // Setters optimizados
    FORCEINLINE void SetDirection(const FVector& InDirection) { Direction = InDirection.GetSafeNormal(); }
    FORCEINLINE void SetSpeed(uint8 InSpeed) { MovementSpeed = InSpeed; }
    FORCEINLINE void SetSpeed(float InSpeed) { MovementSpeed = static_cast<uint8>(FMath::Clamp(InSpeed, 0.0f, 255.0f)); }
    FORCEINLINE void SetFlags(uint8 InFlags) { MovementFlags = InFlags; }
    FORCEINLINE void SetBatchGroup(uint8 InGroup) { BatchGroup = InGroup; }

    // Flags de movimiento (bits 0-7)
    static constexpr uint8 FLAG_MOVING = 0x01;        // 0000 0001 - En movimiento
    static constexpr uint8 FLAG_CHASING = 0x02;       // 0000 0010 - Persiguiendo
    static constexpr uint8 FLAG_FLEEING = 0x04;       // 0000 0100 - Huyendo
    static constexpr uint8 FLAG_STUNNED = 0x08;       // 0000 1000 - Aturdido
    static constexpr uint8 FLAG_ACCELERATING = 0x10;  // 0001 0000 - Acelerando
    static constexpr uint8 FLAG_DECELERATING = 0x20;  // 0010 0000 - Desacelerando
    static constexpr uint8 FLAG_COLLIDING = 0x40;     // 0100 0000 - Colisionando
    static constexpr uint8 FLAG_RESERVED = 0x80;      // 1000 0000 - Reservado

    // Getters de flags
    FORCEINLINE bool IsMoving() const { return (MovementFlags & FLAG_MOVING) != 0; }
    FORCEINLINE bool IsChasing() const { return (MovementFlags & FLAG_CHASING) != 0; }
    FORCEINLINE bool IsFleeing() const { return (MovementFlags & FLAG_FLEEING) != 0; }
    FORCEINLINE bool IsStunned() const { return (MovementFlags & FLAG_STUNNED) != 0; }
    FORCEINLINE bool IsAccelerating() const { return (MovementFlags & FLAG_ACCELERATING) != 0; }
    FORCEINLINE bool IsDecelerating() const { return (MovementFlags & FLAG_DECELERATING) != 0; }
    FORCEINLINE bool IsColliding() const { return (MovementFlags & FLAG_COLLIDING) != 0; }

    // Setters de flags
    FORCEINLINE void SetMoving(bool bMoving) { SetFlag(FLAG_MOVING, bMoving); }
    FORCEINLINE void SetChasing(bool bChasing) { SetFlag(FLAG_CHASING, bChasing); }
    FORCEINLINE void SetFleeing(bool bFleeing) { SetFlag(FLAG_FLEEING, bFleeing); }
    FORCEINLINE void SetStunned(bool bStunned) { SetFlag(FLAG_STUNNED, bStunned); }
    FORCEINLINE void SetAccelerating(bool bAccelerating) { SetFlag(FLAG_ACCELERATING, bAccelerating); }
    FORCEINLINE void SetDecelerating(bool bDecelerating) { SetFlag(FLAG_DECELERATING, bDecelerating); }
    FORCEINLINE void SetColliding(bool bColliding) { SetFlag(FLAG_COLLIDING, bColliding); }

    // Utilidades de movimiento
    FORCEINLINE void Stop() 
    { 
        MovementSpeed = 0; 
        MovementFlags &= ~(FLAG_MOVING | FLAG_CHASING | FLAG_FLEEING | FLAG_ACCELERATING | FLAG_DECELERATING);
    }

    FORCEINLINE void StartMoving(const FVector& InDirection, uint8 InSpeed)
    {
        SetDirection(InDirection);
        SetSpeed(InSpeed);
        SetMoving(true);
    }

    FORCEINLINE void StartChasing(const FVector& InDirection, uint8 InSpeed)
    {
        SetDirection(InDirection);
        SetSpeed(InSpeed);
        SetMoving(true);
        SetChasing(true);
        SetFleeing(false);
    }

    FORCEINLINE void StartFleeing(const FVector& InDirection, uint8 InSpeed)
    {
        SetDirection(InDirection);
        SetSpeed(InSpeed);
        SetMoving(true);
        SetFleeing(true);
        SetChasing(false);
    }

    // Interpolación de dirección
    FORCEINLINE void InterpolateDirection(const FVector& TargetDirection, float Alpha)
    {
        Direction = FMath::Lerp(Direction, TargetDirection.GetSafeNormal(), Alpha).GetSafeNormal();
    }

    // Cálculo de velocidad efectiva
    FORCEINLINE float GetEffectiveSpeed() const
    {
        if (IsStunned()) return 0.0f;
        if (IsColliding()) return GetSpeed() * 0.5f;
        return GetSpeed();
    }

    // Validación
    FORCEINLINE bool IsValid() const
    {
        return !Direction.ContainsNaN() && Direction.SizeSquared() > 0.0f && MovementSpeed <= 255;
    }

private:
    // Helper para setear flags
    FORCEINLINE void SetFlag(uint8 Flag, bool bSet)
    {
        if (bSet)
            MovementFlags |= Flag;
        else
            MovementFlags &= ~Flag;
    }
};
