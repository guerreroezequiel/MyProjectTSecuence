// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiBehaviorFragment.generated.h"

/**
 * Estados principales del zombi
 */
UENUM(BlueprintType)
enum class EZombiState : uint8
{
    Stand UMETA(DisplayName = "Stand"),
    WalkAround UMETA(DisplayName = "Walk Around"),
    Chase UMETA(DisplayName = "Chase"),
    Dead UMETA(DisplayName = "Dead")
};

/**
 * Acciones individuales (pueden combinarse con estados)
 */
UENUM(BlueprintType)
enum class EZombiAction : uint8
{
    None UMETA(DisplayName = "None"),

    Stunned UMETA(DisplayName = "Stunned"),
    Roaring UMETA(DisplayName = "Roaring")
};

/**
 * Condiciones físicas
 */
UENUM(BlueprintType)
enum class EZombiCondition : uint8
{
    Healthy UMETA(DisplayName = "Healthy"),
    Injured UMETA(DisplayName = "Injured"),
    Critical UMETA(DisplayName = "Critical"),
    Dying UMETA(DisplayName = "Dying")
};

/**
 * Comportamiento de horda
 */
UENUM(BlueprintType)
enum class EZombiHordeBehavior : uint8
{
    Individual UMETA(DisplayName = "Individual"),
    Following UMETA(DisplayName = "Following"),
    Swarming UMETA(DisplayName = "Swarming"),
    Scattered UMETA(DisplayName = "Scattered")
};

/**
 * Fragmento de comportamiento optimizado para IA y estados complejos
 * Maneja estados principales, acciones individuales, condiciones y comportamiento de horda
 */
USTRUCT()
struct FZombiBehaviorFragment : public FMassFragment
{
    GENERATED_BODY()

    // Estados y comportamientos (8 bytes) - COMPATIBLE CON DOP
    UPROPERTY()
    uint8 CurrentState = 0; // Estado actual (Chase, WalkAround, Attack, etc.)

    UPROPERTY()
    uint8 StateTimer = 0; // Timer del estado actual (0-255)

    UPROPERTY()
    uint8 StateFlags = 0; // Flags específicos del estado

    UPROPERTY()
    float StateData = 0.0f; // Datos específicos del estado (distancia, dirección, etc.)

    UPROPERTY()
    uint8 ActionFlags = 0; // Acciones individuales (4 bits)

    UPROPERTY()
    uint8 ConditionFlags = 0; // Condiciones físicas (2 bits)

    UPROPERTY()
    uint8 HordeFlags = 0; // Comportamiento de horda (2 bits)

    // Timers de comportamiento (16 bytes)
    UPROPERTY()
    float ActionTimer = 0.0f; // Timer de la acción actual

    UPROPERTY()
    float DirectionChangeTimer = 0.0f; // Timer para cambio de dirección

    // Timers de persecución periódica (8 bytes)
    UPROPERTY()
    float ChasePeriodicTimer = 0.0f; // Timer para persecución periódica (10 segundos)

    UPROPERTY()
    float ChaseDurationTimer = 0.0f; // Timer de duración de persecución (5 segundos)

    // Configuración de comportamiento (16 bytes)
    UPROPERTY()
    float DirectionChangeInterval = 3.0f;

    UPROPERTY()
    float ChaseDistance = 1000.0f;

    UPROPERTY()
    float ChaseSpeed = 200.0f;

    // Referencias de horda (8 bytes)
    UPROPERTY()
    int32 HordeLeaderIndex = -1; // Índice del líder de la horda

    UPROPERTY()
    float HordeInfluenceRadius = 300.0f; // Radio de influencia de la horda

    // Constructor por defecto - COMPATIBLE CON DOP
    FZombiBehaviorFragment()
    {
        CurrentState = static_cast<uint8>(EZombiState::WalkAround); // Estado inicial
        StateTimer = 0;
        StateFlags = 0;
        StateData = 0.0f;
        ActionFlags = 0;
        ConditionFlags = 0;
        HordeFlags = 0;
        ActionTimer = 0.0f;
        DirectionChangeTimer = 0.0f;
        DirectionChangeInterval = 3.0f;
        ChaseDistance = 1000.0f;
        ChaseSpeed = 200.0f;
        HordeLeaderIndex = -1;
        HordeInfluenceRadius = 300.0f;
    }

    // Getters para Estados - COMPATIBLE CON DOP
    EZombiState GetState() const { return static_cast<EZombiState>(CurrentState); }
    bool IsStanding() const { return GetState() == EZombiState::Stand; }
    bool IsWalking() const { return GetState() == EZombiState::WalkAround; }
    bool IsChasing() const { return GetState() == EZombiState::Chase; }
    bool IsDead() const { return GetState() == EZombiState::Dead; }

    // Getters para datos del estado
    float GetStateTimer() const { return static_cast<float>(StateTimer) / 100.0f; } // Convertir de 0-255 a segundos
    uint8 GetStateFlags() const { return StateFlags; }
    float GetStateData() const { return StateData; }

    // Getters para Acciones
    bool IsStunnedAction() const { return (ActionFlags & 0x04) != 0; }
    bool IsRoaringAction() const { return (ActionFlags & 0x08) != 0; }

    // Getters para Condiciones
    EZombiCondition GetCondition() const { return static_cast<EZombiCondition>((ConditionFlags & 0x03)); }
    bool IsHealthy() const { return GetCondition() == EZombiCondition::Healthy; }
    bool IsInjured() const { return GetCondition() == EZombiCondition::Injured; }
    bool IsCritical() const { return GetCondition() == EZombiCondition::Critical; }
    bool IsDying() const { return GetCondition() == EZombiCondition::Dying; }

    // Getters para Horda
    EZombiHordeBehavior GetHordeBehavior() const { return static_cast<EZombiHordeBehavior>((HordeFlags & 0x03)); }
    bool IsIndividual() const { return GetHordeBehavior() == EZombiHordeBehavior::Individual; }
    bool IsFollowing() const { return GetHordeBehavior() == EZombiHordeBehavior::Following; }
    bool IsSwarming() const { return GetHordeBehavior() == EZombiHordeBehavior::Swarming; }
    bool IsScattered() const { return GetHordeBehavior() == EZombiHordeBehavior::Scattered; }

    // Setters para Estados - COMPATIBLE CON DOP
    void SetState(EZombiState NewState)
    {
        CurrentState = static_cast<uint8>(NewState);
        StateTimer = 0; // Reset timer al cambiar estado
    }

    void SetStateTimer(float Timer) { StateTimer = static_cast<uint8>(Timer * 100.0f); } // Convertir segundos a 0-255
    void SetStateFlags(uint8 Flags) { StateFlags = Flags; }
    void SetStateData(float Data) { StateData = Data; }

    // Setters para Acciones
    void SetStunnedAction(bool bStunned) { ActionFlags = bStunned ? (ActionFlags | 0x04) : (ActionFlags & ~0x04); }
    void SetRoaringAction(bool bRoaring) { ActionFlags = bRoaring ? (ActionFlags | 0x08) : (ActionFlags & ~0x08); }

    // Setters para Condiciones
    void SetCondition(EZombiCondition NewCondition) { ConditionFlags = (ConditionFlags & 0xFC) | static_cast<uint32>(NewCondition); }

    // Setters para Horda
    void SetHordeBehavior(EZombiHordeBehavior NewBehavior) { HordeFlags = (HordeFlags & 0xFC) | static_cast<uint32>(NewBehavior); }
    void SetHordeLeader(int32 LeaderIndex) { HordeLeaderIndex = LeaderIndex; }

    // Utilidades
    void ClearAllActions()
    {
        ActionFlags = 0;
        ActionTimer = 0.0f;
    }
    bool HasAnyAction() const { return ActionFlags != 0; }
    bool IsInHorde() const { return HordeLeaderIndex >= 0; }

    // Utilidades para persecución periódica
};