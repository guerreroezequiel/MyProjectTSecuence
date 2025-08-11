// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiCombatFragment.generated.h"

/**
 * Fragmento de combate optimizado para salud, daño y ataques
 * Separado del comportamiento para mejor cache locality
 */
USTRUCT()
struct FZombiCombatFragment : public FMassFragment
{
    GENERATED_BODY()

    // Salud (8 bytes)
    UPROPERTY()
    float Health = 100.0f;

    UPROPERTY()
    float MaxHealth = 100.0f;

    UPROPERTY()
    float HealthPercentage = 1.0f; // Calculado automáticamente

    // Combate (12 bytes)
    UPROPERTY()
    float AttackDamage = 25.0f;

    UPROPERTY()
    float AttackCooldown = 0.0f;

    UPROPERTY()
    float AttackCooldownMax = 2.0f;

    UPROPERTY()
    float AttackRange = 150.0f;

    // Daño y resistencia (8 bytes)
    UPROPERTY()
    float DamageCooldown = 0.0f;

    UPROPERTY()
    float DamageCooldownMax = 0.5f;

    UPROPERTY()
    float DamageResistance = 0.0f; // 0.0 = sin resistencia, 1.0 = inmune

    UPROPERTY()
    float LastDamageTime = 0.0f;

    // Constructor por defecto
    FZombiCombatFragment()
    {
        Health = 100.0f;
        MaxHealth = 100.0f;
        HealthPercentage = 1.0f;
        AttackDamage = 25.0f;
        AttackCooldown = 0.0f;
        AttackCooldownMax = 2.0f;
        AttackRange = 150.0f;
        DamageCooldown = 0.0f;
        DamageCooldownMax = 0.5f;
        DamageResistance = 0.0f;
        LastDamageTime = 0.0f;
    }

    // Constructor con parámetros
    FZombiCombatFragment(float InMaxHealth, float InAttackDamage, float InAttackRange = 150.0f)
    {
        MaxHealth = InMaxHealth;
        Health = MaxHealth;
        HealthPercentage = 1.0f;
        AttackDamage = InAttackDamage;
        AttackCooldown = 0.0f;
        AttackCooldownMax = 2.0f;
        AttackRange = InAttackRange;
        DamageCooldown = 0.0f;
        DamageCooldownMax = 0.5f;
        DamageResistance = 0.0f;
        LastDamageTime = 0.0f;
    }

    // Getters
    bool IsAlive() const { return Health > 0.0f; }
    bool IsDead() const { return Health <= 0.0f; }
    bool CanAttack() const { return AttackCooldown <= 0.0f; }
    bool CanTakeDamage() const { return DamageCooldown <= 0.0f; }
    bool IsAtFullHealth() const { return Health >= MaxHealth; }
    bool IsLowHealth() const { return HealthPercentage < 0.25f; }
    bool IsCriticalHealth() const { return HealthPercentage < 0.1f; }

    // Setters
    void SetHealth(float NewHealth)
    {
        Health = FMath::Clamp(NewHealth, 0.0f, MaxHealth);
        HealthPercentage = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    }

    void SetMaxHealth(float NewMaxHealth)
    {
        MaxHealth = FMath::Max(NewMaxHealth, 1.0f);
        Health = FMath::Min(Health, MaxHealth);
        HealthPercentage = MaxHealth > 0.0f ? Health / MaxHealth : 0.0f;
    }

    void AddHealth(float Amount)
    {
        SetHealth(Health + Amount);
    }

    void RemoveHealth(float Amount)
    {
        SetHealth(Health - (Amount * (1.0f - DamageResistance)));
        LastDamageTime = 0.0f; // Reset para animación de daño
    }

    void ResetHealth()
    {
        SetHealth(MaxHealth);
    }

    // Combate
    void StartAttackCooldown()
    {
        AttackCooldown = AttackCooldownMax;
    }

    void StartDamageCooldown()
    {
        DamageCooldown = DamageCooldownMax;
    }

    void UpdateCooldowns(float DeltaTime)
    {
        AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaTime);
        DamageCooldown = FMath::Max(0.0f, DamageCooldown - DeltaTime);
        LastDamageTime += DeltaTime;
    }

    // Utilidades
    float GetHealthPercentage() const { return HealthPercentage; }
    float GetRemainingHealth() const { return Health; }
    float GetMissingHealth() const { return MaxHealth - Health; }
};