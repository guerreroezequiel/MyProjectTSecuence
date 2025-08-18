// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiStateFragment.generated.h"

/**
 * Fragmento optimizado para estados de zombies usando flags
 * OPTIMIZADO: 16 bytes para mejor cache locality
 * DOP: Flags en lugar de enums para evitar cache misses
 */
USTRUCT()
struct FZombiStateFragment : public FMassFragment
{
    GENERATED_BODY()

    // Flags de estado (8 bytes)
    UPROPERTY()
    uint8 StateFlags = 0; // Estados principales (bits 0-7)
    UPROPERTY()
    uint8 ActionFlags = 0; // Acciones activas (bits 0-7)
    UPROPERTY()
    uint8 ConditionFlags = 0; // Condiciones físicas (bits 0-7)
    UPROPERTY()
    uint8 HordeFlags = 0; // Comportamiento de horda (bits 0-7)

    // Timers y datos (8 bytes)
    UPROPERTY()
    uint16 StateTimer = 0; // Timer del estado actual (0-65535)
    UPROPERTY()
    uint16 ActionTimer = 0; // Timer de acciones (0-65535)
    UPROPERTY()
    uint32 StateData = 0; // Datos comprimidos del estado

    // Constructor por defecto
    FZombiStateFragment()
    {
        StateFlags = 0;
        ActionFlags = 0;
        ConditionFlags = 0;
        HordeFlags = 0;
        StateTimer = 0;
        ActionTimer = 0;
        StateData = 0;
    }

    // Constructor con estado inicial
    FZombiStateFragment(uint8 InStateFlags, uint8 InActionFlags = 0)
    {
        StateFlags = InStateFlags;
        ActionFlags = InActionFlags;
        ConditionFlags = FLAG_CONDITION_HEALTHY;
        HordeFlags = FLAG_HORDE_INDIVIDUAL;
        StateTimer = 0;
        ActionTimer = 0;
        StateData = 0;
    }

    // ===== FLAGS DE ESTADO (StateFlags) =====
    // Estados principales (bits 0-7)
    static constexpr uint8 FLAG_STATE_IDLE = 0x01;      // 0000 0001 - Inactivo
    static constexpr uint8 FLAG_STATE_WALKING = 0x02;   // 0000 0010 - Caminando
    static constexpr uint8 FLAG_STATE_CHASING = 0x04;   // 0000 0100 - Persiguiendo
    static constexpr uint8 FLAG_STATE_ATTACKING = 0x08; // 0000 1000 - Atacando
    static constexpr uint8 FLAG_STATE_DEAD = 0x10;      // 0001 0000 - Muerto
    static constexpr uint8 FLAG_STATE_STUNNED = 0x20;   // 0010 0000 - Aturdido
    static constexpr uint8 FLAG_STATE_FLEEING = 0x40;   // 0100 0000 - Huyendo
    static constexpr uint8 FLAG_STATE_RESERVED = 0x80;  // 1000 0000 - Reservado

    // ===== FLAGS DE ACCIÓN (ActionFlags) =====
    // Acciones individuales (bits 0-7)
    static constexpr uint8 FLAG_ACTION_NONE = 0x00;          // 0000 0000 - Sin acción
    static constexpr uint8 FLAG_ACTION_ROARING = 0x01;       // 0000 0001 - Rugiendo
    static constexpr uint8 FLAG_ACTION_EATING = 0x02;        // 0000 0010 - Comiendo
    static constexpr uint8 FLAG_ACTION_INVESTIGATING = 0x04; // 0000 0100 - Investigando
    static constexpr uint8 FLAG_ACTION_ALERTING = 0x08;      // 0000 1000 - Alertando
    static constexpr uint8 FLAG_ACTION_RECOVERING = 0x10;    // 0001 0000 - Recuperándose
    static constexpr uint8 FLAG_ACTION_SPAWNING = 0x20;      // 0010 0000 - Apareciendo
    static constexpr uint8 FLAG_ACTION_RESERVED = 0xC0;      // 1100 0000 - Reservado

    // ===== FLAGS DE CONDICIÓN (ConditionFlags) =====
    // Condiciones físicas (bits 0-7)
    static constexpr uint8 FLAG_CONDITION_HEALTHY = 0x01;  // 0000 0001 - Saludable
    static constexpr uint8 FLAG_CONDITION_INJURED = 0x02;  // 0000 0010 - Herido
    static constexpr uint8 FLAG_CONDITION_CRITICAL = 0x04; // 0000 0100 - Crítico
    static constexpr uint8 FLAG_CONDITION_DYING = 0x08;    // 0000 1000 - Muriendo
    static constexpr uint8 FLAG_CONDITION_BLEEDING = 0x10; // 0001 0000 - Sangrando
    static constexpr uint8 FLAG_CONDITION_INFECTED = 0x20; // 0010 0000 - Infectado
    static constexpr uint8 FLAG_CONDITION_RESERVED = 0xC0; // 1100 0000 - Reservado

    // ===== FLAGS DE HORDA (HordeFlags) =====
    // Comportamiento de horda (bits 0-7)
    static constexpr uint8 FLAG_HORDE_INDIVIDUAL = 0x01; // 0000 0001 - Individual
    static constexpr uint8 FLAG_HORDE_FOLLOWING = 0x02;  // 0000 0010 - Siguiendo
    static constexpr uint8 FLAG_HORDE_SWARMING = 0x04;   // 0000 0100 - Enjambre
    static constexpr uint8 FLAG_HORDE_SCATTERED = 0x08;  // 0000 1000 - Disperso
    static constexpr uint8 FLAG_HORDE_LEADER = 0x10;     // 0001 0000 - Líder
    static constexpr uint8 FLAG_HORDE_SUPPORT = 0x20;    // 0010 0000 - Soporte
    static constexpr uint8 FLAG_HORDE_RESERVED = 0xC0;   // 1100 0000 - Reservado

    // ===== GETTERS DE ESTADO =====
    FORCEINLINE bool IsIdle() const { return (StateFlags & FLAG_STATE_IDLE) != 0; }
    FORCEINLINE bool IsWalking() const { return (StateFlags & FLAG_STATE_WALKING) != 0; }
    FORCEINLINE bool IsChasing() const { return (StateFlags & FLAG_STATE_CHASING) != 0; }
    FORCEINLINE bool IsAttacking() const { return (StateFlags & FLAG_STATE_ATTACKING) != 0; }
    FORCEINLINE bool IsDead() const { return (StateFlags & FLAG_STATE_DEAD) != 0; }
    FORCEINLINE bool IsStunned() const { return (StateFlags & FLAG_STATE_STUNNED) != 0; }
    FORCEINLINE bool IsFleeing() const { return (StateFlags & FLAG_STATE_FLEEING) != 0; }

    // ===== GETTERS DE ACCIÓN =====
    FORCEINLINE bool IsRoaring() const { return (ActionFlags & FLAG_ACTION_ROARING) != 0; }
    FORCEINLINE bool IsEating() const { return (ActionFlags & FLAG_ACTION_EATING) != 0; }
    FORCEINLINE bool IsInvestigating() const { return (ActionFlags & FLAG_ACTION_INVESTIGATING) != 0; }
    FORCEINLINE bool IsAlerting() const { return (ActionFlags & FLAG_ACTION_ALERTING) != 0; }
    FORCEINLINE bool IsRecovering() const { return (ActionFlags & FLAG_ACTION_RECOVERING) != 0; }
    FORCEINLINE bool IsSpawning() const { return (ActionFlags & FLAG_ACTION_SPAWNING) != 0; }

    // ===== GETTERS DE CONDICIÓN =====
    FORCEINLINE bool IsHealthy() const { return (ConditionFlags & FLAG_CONDITION_HEALTHY) != 0; }
    FORCEINLINE bool IsInjured() const { return (ConditionFlags & FLAG_CONDITION_INJURED) != 0; }
    FORCEINLINE bool IsCritical() const { return (ConditionFlags & FLAG_CONDITION_CRITICAL) != 0; }
    FORCEINLINE bool IsDying() const { return (ConditionFlags & FLAG_CONDITION_DYING) != 0; }
    FORCEINLINE bool IsBleeding() const { return (ConditionFlags & FLAG_CONDITION_BLEEDING) != 0; }
    FORCEINLINE bool IsInfected() const { return (ConditionFlags & FLAG_CONDITION_INFECTED) != 0; }

    // ===== GETTERS DE HORDA =====
    FORCEINLINE bool IsIndividual() const { return (HordeFlags & FLAG_HORDE_INDIVIDUAL) != 0; }
    FORCEINLINE bool IsFollowing() const { return (HordeFlags & FLAG_HORDE_FOLLOWING) != 0; }
    FORCEINLINE bool IsSwarming() const { return (HordeFlags & FLAG_HORDE_SWARMING) != 0; }
    FORCEINLINE bool IsScattered() const { return (HordeFlags & FLAG_HORDE_SCATTERED) != 0; }
    FORCEINLINE bool IsLeader() const { return (HordeFlags & FLAG_HORDE_LEADER) != 0; }
    FORCEINLINE bool IsSupport() const { return (HordeFlags & FLAG_HORDE_SUPPORT) != 0; }

    // ===== SETTERS DE ESTADO =====
    FORCEINLINE void SetIdle(bool bIdle) { SetStateFlag(FLAG_STATE_IDLE, bIdle); }
    FORCEINLINE void SetWalking(bool bWalking) { SetStateFlag(FLAG_STATE_WALKING, bWalking); }
    FORCEINLINE void SetChasing(bool bChasing) { SetStateFlag(FLAG_STATE_CHASING, bChasing); }
    FORCEINLINE void SetAttacking(bool bAttacking) { SetStateFlag(FLAG_STATE_ATTACKING, bAttacking); }
    FORCEINLINE void SetDead(bool bDead) { SetStateFlag(FLAG_STATE_DEAD, bDead); }
    FORCEINLINE void SetStunned(bool bStunned) { SetStateFlag(FLAG_STATE_STUNNED, bStunned); }
    FORCEINLINE void SetFleeing(bool bFleeing) { SetStateFlag(FLAG_STATE_FLEEING, bFleeing); }

    // ===== SETTERS DE ACCIÓN =====
    FORCEINLINE void SetRoaring(bool bRoaring) { SetActionFlag(FLAG_ACTION_ROARING, bRoaring); }
    FORCEINLINE void SetEating(bool bEating) { SetActionFlag(FLAG_ACTION_EATING, bEating); }
    FORCEINLINE void SetInvestigating(bool bInvestigating) { SetActionFlag(FLAG_ACTION_INVESTIGATING, bInvestigating); }
    FORCEINLINE void SetAlerting(bool bAlerting) { SetActionFlag(FLAG_ACTION_ALERTING, bAlerting); }
    FORCEINLINE void SetRecovering(bool bRecovering) { SetActionFlag(FLAG_ACTION_RECOVERING, bRecovering); }
    FORCEINLINE void SetSpawning(bool bSpawning) { SetActionFlag(FLAG_ACTION_SPAWNING, bSpawning); }

    // ===== SETTERS DE CONDICIÓN =====
    FORCEINLINE void SetHealthy(bool bHealthy) { SetConditionFlag(FLAG_CONDITION_HEALTHY, bHealthy); }
    FORCEINLINE void SetInjured(bool bInjured) { SetConditionFlag(FLAG_CONDITION_INJURED, bInjured); }
    FORCEINLINE void SetCritical(bool bCritical) { SetConditionFlag(FLAG_CONDITION_CRITICAL, bCritical); }
    FORCEINLINE void SetDying(bool bDying) { SetConditionFlag(FLAG_CONDITION_DYING, bDying); }
    FORCEINLINE void SetBleeding(bool bBleeding) { SetConditionFlag(FLAG_CONDITION_BLEEDING, bBleeding); }
    FORCEINLINE void SetInfected(bool bInfected) { SetConditionFlag(FLAG_CONDITION_INFECTED, bInfected); }

    // ===== SETTERS DE HORDA =====
    FORCEINLINE void SetIndividual(bool bIndividual) { SetHordeFlag(FLAG_HORDE_INDIVIDUAL, bIndividual); }
    FORCEINLINE void SetFollowing(bool bFollowing) { SetHordeFlag(FLAG_HORDE_FOLLOWING, bFollowing); }
    FORCEINLINE void SetSwarming(bool bSwarming) { SetHordeFlag(FLAG_HORDE_SWARMING, bSwarming); }
    FORCEINLINE void SetScattered(bool bScattered) { SetHordeFlag(FLAG_HORDE_SCATTERED, bScattered); }
    FORCEINLINE void SetLeader(bool bLeader) { SetHordeFlag(FLAG_HORDE_LEADER, bLeader); }
    FORCEINLINE void SetSupport(bool bSupport) { SetHordeFlag(FLAG_HORDE_SUPPORT, bSupport); }

    // ===== UTILIDADES DE ESTADO =====
    FORCEINLINE void ClearAllStates() { StateFlags = 0; }
    FORCEINLINE void ClearAllActions() { ActionFlags = 0; }
    FORCEINLINE void ClearAllConditions() { ConditionFlags = 0; }
    FORCEINLINE void ClearAllHordeFlags() { HordeFlags = 0; }

    FORCEINLINE void SetToIdle()
    {
        ClearAllStates();
        SetIdle(true);
        StateTimer = 0;
        // Tag se gestionará automáticamente en BehaviorProcessor
    }

    FORCEINLINE void SetToWalking()
    {
        ClearAllStates();
        SetWalking(true);
        StateTimer = 0;
        // Tag se gestionará automáticamente en BehaviorProcessor
    }

    FORCEINLINE void SetToChasing()
    {
        ClearAllStates();
        SetChasing(true);
        StateTimer = 0;
        // Tag se gestionará automáticamente en BehaviorProcessor
    }

    FORCEINLINE void SetToDead()
    {
        ClearAllStates();
        SetDead(true);
        StateTimer = 0;
        ActionTimer = 0;
    }

    // ===== UTILIDADES DE TIMER =====
    FORCEINLINE float GetStateTimerSeconds() const { return static_cast<float>(StateTimer) / 60.0f; }
    FORCEINLINE float GetActionTimerSeconds() const { return static_cast<float>(ActionTimer) / 60.0f; }

    FORCEINLINE void SetStateTimerSeconds(float Seconds)
    {
        StateTimer = static_cast<uint16>(FMath::Clamp(Seconds * 60.0f, 0.0f, 65535.0f));
    }

    FORCEINLINE void SetActionTimerSeconds(float Seconds)
    {
        ActionTimer = static_cast<uint16>(FMath::Clamp(Seconds * 60.0f, 0.0f, 65535.0f));
    }

    // ===== UTILIDADES DE DATOS COMPRIMIDOS =====
    FORCEINLINE void SetStateData(uint32 Data) { StateData = Data; }
    FORCEINLINE uint32 GetStateData() const { return StateData; }

    // Validación
    FORCEINLINE bool IsValid() const
    {
        // Verificar que solo un estado principal esté activo
        uint8 ActiveStates = StateFlags & (FLAG_STATE_IDLE | FLAG_STATE_WALKING | FLAG_STATE_CHASING |
                                           FLAG_STATE_ATTACKING | FLAG_STATE_DEAD | FLAG_STATE_STUNNED |
                                           FLAG_STATE_FLEEING);
        return FMath::CountBits(ActiveStates) <= 1; // Solo un estado activo
    }

private:
    // Helpers para setear flags
    FORCEINLINE void SetStateFlag(uint8 Flag, bool bSet)
    {
        if (bSet)
            StateFlags |= Flag;
        else
            StateFlags &= ~Flag;
    }

    FORCEINLINE void SetActionFlag(uint8 Flag, bool bSet)
    {
        if (bSet)
            ActionFlags |= Flag;
        else
            ActionFlags &= ~Flag;
    }

    FORCEINLINE void SetConditionFlag(uint8 Flag, bool bSet)
    {
        if (bSet)
            ConditionFlags |= Flag;
        else
            ConditionFlags &= ~Flag;
    }

    FORCEINLINE void SetHordeFlag(uint8 Flag, bool bSet)
    {
        if (bSet)
            HordeFlags |= Flag;
        else
            HordeFlags &= ~Flag;
    }
};
