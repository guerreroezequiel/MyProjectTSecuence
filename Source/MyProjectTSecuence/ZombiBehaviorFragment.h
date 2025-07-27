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
    Attack UMETA(DisplayName = "Attack"),
    TakeDamage UMETA(DisplayName = "Take Damage"),
    Dead UMETA(DisplayName = "Dead")
};

/**
 * Acciones individuales (pueden combinarse con estados)
 */
UENUM(BlueprintType)
enum class EZombiAction : uint8
{
    None UMETA(DisplayName = "None"),
    Attacking UMETA(DisplayName = "Attacking"),
    Damaged UMETA(DisplayName = "Damaged"),
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

    // Estados y comportamientos (8 bytes)
    UPROPERTY()
    uint32 StateFlags = 0; // Estado principal (3 bits)

    UPROPERTY()
    uint32 ActionFlags = 0; // Acciones individuales (4 bits)

    UPROPERTY()
    uint32 ConditionFlags = 0; // Condiciones físicas (2 bits)

    UPROPERTY()
    uint32 HordeFlags = 0; // Comportamiento de horda (2 bits)

    // Timers de comportamiento (16 bytes)
    UPROPERTY()
    float StateTimer = 0.0f; // Timer del estado actual

    UPROPERTY()
    float ActionTimer = 0.0f; // Timer de la acción actual

    UPROPERTY()
    float BehaviorTimer = 0.0f; // Timer general de comportamiento

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

    UPROPERTY()
    float AttackRange = 150.0f;

    // Configuración de persecución periódica
    UPROPERTY()
    float PeriodicChaseInterval = 10.0f; // Intervalo entre persecuciones (10 segundos)

    UPROPERTY()
    float PeriodicChaseDuration = 5.0f; // Duración de persecución (5 segundos)

    UPROPERTY()
    float PeriodicChaseDistance = 500.0f; // Distancia para activar persecución periódica (aumentada de 100 a 500)

    // Referencias de horda (8 bytes)
    UPROPERTY()
    int32 HordeLeaderIndex = -1; // Índice del líder de la horda

    UPROPERTY()
    float HordeInfluenceRadius = 300.0f; // Radio de influencia de la horda

    // Constructor por defecto
    FZombiBehaviorFragment()
    {
        StateFlags = 0;
        ActionFlags = 0;
        ConditionFlags = 0;
        HordeFlags = 0;
        StateTimer = 0.0f;
        ActionTimer = 0.0f;
        BehaviorTimer = 0.0f;
        DirectionChangeTimer = 0.0f;
        ChasePeriodicTimer = 0.0f;
        ChaseDurationTimer = 0.0f;
        DirectionChangeInterval = 3.0f;
        ChaseDistance = 1000.0f;
        ChaseSpeed = 200.0f;
        AttackRange = 150.0f;
        PeriodicChaseInterval = 10.0f;
        PeriodicChaseDuration = 5.0f;
        PeriodicChaseDistance = 500.0f;
        HordeLeaderIndex = -1;
        HordeInfluenceRadius = 300.0f;
    }

    // Getters para Estados
    EZombiState GetState() const { return static_cast<EZombiState>(StateFlags & 0x07); }
    bool IsStanding() const { return GetState() == EZombiState::Stand; }
    bool IsWalking() const { return GetState() == EZombiState::WalkAround; }
    bool IsChasing() const { return GetState() == EZombiState::Chase; }
    bool IsAttacking() const { return GetState() == EZombiState::Attack; }
    bool IsTakingDamage() const { return GetState() == EZombiState::TakeDamage; }
    bool IsDead() const { return GetState() == EZombiState::Dead; }

    // Getters para Acciones
    bool IsAttackingAction() const { return (ActionFlags & 0x01) != 0; }
    bool IsDamagedAction() const { return (ActionFlags & 0x02) != 0; }
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

    // Setters para Estados
    void SetState(EZombiState NewState)
    {
        StateFlags = (StateFlags & 0xF8) | static_cast<uint32>(NewState);
        StateTimer = 0.0f; // Reset timer al cambiar estado
    }

    // Setters para Acciones
    void SetAttackingAction(bool bAttacking) { ActionFlags = bAttacking ? (ActionFlags | 0x01) : (ActionFlags & ~0x01); }
    void SetDamagedAction(bool bDamaged) { ActionFlags = bDamaged ? (ActionFlags | 0x02) : (ActionFlags & ~0x02); }
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
    bool IsInPeriodicChaseDistance(float DistanceToPlayer) const { return DistanceToPlayer <= PeriodicChaseDistance; }
    bool IsPeriodicChaseTime() const { return ChasePeriodicTimer >= PeriodicChaseInterval; }
    bool IsPeriodicChaseActive() const { return ChaseDurationTimer > 0.0f; }
    void StartPeriodicChase() { ChaseDurationTimer = PeriodicChaseDuration; }
    void ResetPeriodicChaseTimer() { ChasePeriodicTimer = 0.0f; }
};