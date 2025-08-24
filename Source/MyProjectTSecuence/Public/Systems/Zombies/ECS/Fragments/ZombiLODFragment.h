// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "ZombiLODFragment.generated.h"

/**
 * Fragmento LOD simplificado para optimización
 * Solo datos estables - Frecuencia manejada por tags
 * Optimizado para enfoque Tags por Frecuencia
 */
USTRUCT()
struct FZombiLODFragment : public FMassFragment
{
    GENERATED_BODY()

    // LOD visual - 0=Full, 1=Reduced, 2=Simple, 3=Sprite
    UPROPERTY()
    uint8 VisualLOD;

    // Nivel de prioridad - 0=Idle, 1=WalkAround, 2=Seek, 3=Chase, 4=TakeDamage, 5=Attack, 6=Dead
    UPROPERTY()
    uint8 PriorityLevel;

    // Padding para alineación
    UPROPERTY()
    uint8 Padding;

    // Distancia al jugador (necesario para cálculos)
    UPROPERTY()
    float DistanceToPlayer;

    // Constructor por defecto
    FZombiLODFragment()
    {
        VisualLOD = 0;     // Full detail por defecto
        PriorityLevel = 0; // Idle por defecto
        Padding = 0;       // Alineación
        DistanceToPlayer = 0.0f;
    }

    // Enums para VisualLOD
    enum class EVisualLOD : uint8
    {
        Full = 0,    // Detalle completo
        Reduced = 1, // Detalle reducido
        Simple = 2,  // Detalle simple
        Sprite = 3   // Sprite 2D
    };

    // Enums para PriorityLevel
    enum class EPriorityLevel : uint8
    {
        Idle = 0,
        WalkAround = 1,
        Seek = 2,
        Chase = 3,
        TakeDamage = 4,
        Attack = 5,
        Dead = 6
    };

    // Getters para VisualLOD
    EVisualLOD GetVisualLOD() const { return static_cast<EVisualLOD>(VisualLOD); }
    bool IsFullDetail() const { return VisualLOD == 0; }
    bool IsReducedDetail() const { return VisualLOD == 1; }
    bool IsSimpleDetail() const { return VisualLOD == 2; }
    bool IsSpriteDetail() const { return VisualLOD == 3; }

    // Getters para PriorityLevel
    EPriorityLevel GetPriorityLevel() const { return static_cast<EPriorityLevel>(PriorityLevel); }
    bool IsHighPriority() const { return PriorityLevel >= 4; }                         // TakeDamage, Attack
    bool IsMediumPriority() const { return PriorityLevel >= 2 && PriorityLevel <= 3; } // Seek, Chase
    bool IsLowPriority() const { return PriorityLevel <= 1; }                          // Idle, WalkAround

    // Getters para datos
    float GetDistanceToPlayer() const { return DistanceToPlayer; }

    // Setters para VisualLOD
    void SetVisualLOD(EVisualLOD LOD) { VisualLOD = static_cast<uint8>(LOD); }
    void SetFullDetail() { VisualLOD = 0; }
    void SetReducedDetail() { VisualLOD = 1; }
    void SetSimpleDetail() { VisualLOD = 2; }
    void SetSpriteDetail() { VisualLOD = 3; }

    // Setters para PriorityLevel
    void SetPriorityLevel(EPriorityLevel Priority) { PriorityLevel = static_cast<uint8>(Priority); }
    void SetIdlePriority() { PriorityLevel = 0; }
    void SetWalkAroundPriority() { PriorityLevel = 1; }
    void SetSeekPriority() { PriorityLevel = 2; }
    void SetChasePriority() { PriorityLevel = 3; }
    void SetTakeDamagePriority() { PriorityLevel = 4; }
    void SetAttackPriority() { PriorityLevel = 5; }
    void SetDeadPriority() { PriorityLevel = 6; }

    // Setters para datos
    void SetDistanceToPlayer(float Distance) { DistanceToPlayer = Distance; }
};
