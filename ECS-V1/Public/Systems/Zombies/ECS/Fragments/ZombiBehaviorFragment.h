// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiBehaviorFragment.generated.h"

/**
 * Estados principales del zombi - Simplificados para enfoque híbrido
 */
UENUM(BlueprintType)
enum class EZombiState : uint8
{
    Idle UMETA(DisplayName = "Idle"),
    WalkAround UMETA(DisplayName = "Walk Around"),
    Seek UMETA(DisplayName = "Seek"),
    Chase UMETA(DisplayName = "Chase"),
    TakeDamage UMETA(DisplayName = "Take Damage"),
    Attack UMETA(DisplayName = "Attack"),
    Dead UMETA(DisplayName = "Dead")
};

/**
 * Fragmento de comportamiento simplificado para enfoque híbrido
 * Optimizado para ECS - Solo datos esenciales
 */
USTRUCT()
struct FZombiBehaviorFragment : public FMassFragment
{
    GENERATED_BODY()

    // Estado principal (1 byte)
    UPROPERTY()
    EZombiState CurrentState = EZombiState::Idle;

    // Timers esenciales (8 bytes)
    UPROPERTY()
    float StateTimer = 0.0f; // Tiempo en estado actual

    UPROPERTY()
    float ActionTimer = 0.0f; // Timer para acciones específicas

    // Datos del estado actual (4 bytes)
    UPROPERTY()
    float StateData = 0.0f; // Datos específicos (distancia, dirección, etc.)

    // Configuración básica (12 bytes)
    UPROPERTY()
    float DirectionChangeInterval = 3.0f;

    UPROPERTY()
    float ChaseDistance = 1000.0f;

    UPROPERTY()
    float ChaseSpeed = 200.0f;

    // Constructor por defecto
    FZombiBehaviorFragment()
    {
        CurrentState = EZombiState::Idle;
        StateTimer = 0.0f;
        ActionTimer = 0.0f;
        StateData = 0.0f;
        DirectionChangeInterval = 3.0f;
        ChaseDistance = 1000.0f;
        ChaseSpeed = 200.0f;
    }

    // Getters para Estados - Simplificados
    EZombiState GetState() const { return CurrentState; }
    bool IsIdle() const { return CurrentState == EZombiState::Idle; }
    bool IsWalking() const { return CurrentState == EZombiState::WalkAround; }
    bool IsSeeking() const { return CurrentState == EZombiState::Seek; }
    bool IsChasing() const { return CurrentState == EZombiState::Chase; }
    bool IsTakingDamage() const { return CurrentState == EZombiState::TakeDamage; }
    bool IsAttacking() const { return CurrentState == EZombiState::Attack; }
    bool IsDead() const { return CurrentState == EZombiState::Dead; }

    // Getters para timers
    float GetStateTimer() const { return StateTimer; }
    float GetActionTimer() const { return ActionTimer; }
    float GetStateData() const { return StateData; }

    // Setters para Estados - Simplificados
    void SetState(EZombiState NewState)
    {
        CurrentState = NewState;
        StateTimer = 0.0f; // Reset timer al cambiar estado
    }

    void SetStateTimer(float Timer) { StateTimer = Timer; }
    void SetActionTimer(float Timer) { ActionTimer = Timer; }
    void SetStateData(float Data) { StateData = Data; }

    // Utilidades
    void ClearAllActions()
    {
        ActionTimer = 0.0f;
        StateData = 0.0f;
    }

    bool HasAnyAction() const { return ActionTimer > 0.0f; }

    // Verificar si debe cambiar de dirección (para WalkAround)
    bool ShouldChangeDirection() const
    {
        return CurrentState == EZombiState::WalkAround && StateTimer >= DirectionChangeInterval;
    }
};