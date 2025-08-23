// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "ZombiLODFragment.generated.h"

/**
 * Fragmento LOD para optimización inteligente
 * Maneja Level of Detail basado en estado + distancia + estímulos
 * Optimizado para enfoque híbrido (Estados + Tags)
 */
USTRUCT()
struct FZombiLODFragment : public FMassFragment
{
    GENERATED_BODY()

    // Frecuencia de actualización - 0=60fps, 1=30fps, 2=15fps, 3=5fps
    UPROPERTY()
    uint8 UpdateFrequency;

    // LOD visual - 0=Full, 1=Reduced, 2=Simple, 3=Sprite
    UPROPERTY()
    uint8 VisualLOD;

    // Flags de optimización
    UPROPERTY()
    uint8 bInFrustum; // Visible en cámara
    UPROPERTY()
    uint8 bNeedsUpdate; // Debe procesarse este frame

    // Nivel de prioridad - 0=Idle, 1=WalkAround, 2=Seek, 3=Chase, 4=TakeDamage, 5=Attack, 6=Dead
    UPROPERTY()
    uint8 PriorityLevel;

    // Datos de distancia y tiempo (12 bytes)
    UPROPERTY()
    float DistanceToPlayer; // Distancia al jugador
    UPROPERTY()
    float LastUpdateTime; // Último tiempo de update
    UPROPERTY()
    float StimulusIntensity; // Intensidad del estímulo (daño, etc.)

    // Constructor por defecto
    FZombiLODFragment()
    {
        UpdateFrequency = 0; // 60 FPS por defecto
        VisualLOD = 0;       // Full detail por defecto
        bInFrustum = 1;      // Visible por defecto
        bNeedsUpdate = 1;    // Necesita update por defecto
        PriorityLevel = 0;   // Idle por defecto
        DistanceToPlayer = 0.0f;
        LastUpdateTime = 0.0f;
        StimulusIntensity = 0.0f;
    }

    // Enums para UpdateFrequency
    enum class EUpdateFrequency : uint8
    {
        Critical = 0, // 60 FPS - Máxima prioridad
        High = 1,     // 30 FPS - Alta prioridad
        Normal = 2,   // 15 FPS - Prioridad normal
        Low = 3       // 5 FPS - Baja prioridad
    };

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

    // Getters para UpdateFrequency
    EUpdateFrequency GetUpdateFrequency() const { return static_cast<EUpdateFrequency>(UpdateFrequency); }
    bool IsCriticalUpdate() const { return UpdateFrequency == 0; }
    bool IsHighUpdate() const { return UpdateFrequency == 1; }
    bool IsNormalUpdate() const { return UpdateFrequency == 2; }
    bool IsLowUpdate() const { return UpdateFrequency == 3; }

    // Getters para VisualLOD
    EVisualLOD GetVisualLOD() const { return static_cast<EVisualLOD>(VisualLOD); }
    bool IsFullDetail() const { return VisualLOD == 0; }
    bool IsReducedDetail() const { return VisualLOD == 1; }
    bool IsSimpleDetail() const { return VisualLOD == 2; }
    bool IsSpriteDetail() const { return VisualLOD == 3; }

    // Getters para flags
    bool IsInFrustum() const { return bInFrustum != 0; }
    bool NeedsUpdate() const { return bNeedsUpdate != 0; }

    // Getters para PriorityLevel
    EPriorityLevel GetPriorityLevel() const { return static_cast<EPriorityLevel>(PriorityLevel); }
    bool IsHighPriority() const { return PriorityLevel >= 4; }                         // TakeDamage, Attack
    bool IsMediumPriority() const { return PriorityLevel >= 2 && PriorityLevel <= 3; } // Seek, Chase
    bool IsLowPriority() const { return PriorityLevel <= 1; }                          // Idle, WalkAround

    // Getters para datos
    float GetDistanceToPlayer() const { return DistanceToPlayer; }
    float GetLastUpdateTime() const { return LastUpdateTime; }
    float GetStimulusIntensity() const { return StimulusIntensity; }

    // Setters para UpdateFrequency
    void SetUpdateFrequency(EUpdateFrequency Frequency) { UpdateFrequency = static_cast<uint8>(Frequency); }
    void SetCriticalUpdate() { UpdateFrequency = 0; }
    void SetHighUpdate() { UpdateFrequency = 1; }
    void SetNormalUpdate() { UpdateFrequency = 2; }
    void SetLowUpdate() { UpdateFrequency = 3; }

    // Setters para VisualLOD
    void SetVisualLOD(EVisualLOD LOD) { VisualLOD = static_cast<uint8>(LOD); }
    void SetFullDetail() { VisualLOD = 0; }
    void SetReducedDetail() { VisualLOD = 1; }
    void SetSimpleDetail() { VisualLOD = 2; }
    void SetSpriteDetail() { VisualLOD = 3; }

    // Setters para flags
    void SetInFrustum(bool bInFrustumValue) { bInFrustum = bInFrustumValue ? 1 : 0; }
    void SetNeedsUpdate(bool bNeedsUpdateValue) { bNeedsUpdate = bNeedsUpdateValue ? 1 : 0; }

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
    void SetLastUpdateTime(float Time) { LastUpdateTime = Time; }
    void SetStimulusIntensity(float Intensity) { StimulusIntensity = Intensity; }

    // Utilidades
    void ResetUpdateFlags()
    {
        bNeedsUpdate = 0;
    }

    void MarkForUpdate()
    {
        bNeedsUpdate = 1;
    }

    // Calcular si debe procesarse este frame basado en frecuencia
    bool ShouldProcessThisFrame(float CurrentTime) const
    {
        if (!bNeedsUpdate)
            return false;

        float TimeSinceLastUpdate = CurrentTime - LastUpdateTime;
        float RequiredInterval = GetUpdateInterval();

        return TimeSinceLastUpdate >= RequiredInterval;
    }

    // Obtener intervalo de update en segundos
    float GetUpdateInterval() const
    {
        switch (UpdateFrequency)
        {
        case 0:
            return 1.0f / 60.0f; // 60 FPS
        case 1:
            return 1.0f / 30.0f; // 30 FPS
        case 2:
            return 1.0f / 15.0f; // 15 FPS
        case 3:
            return 1.0f / 5.0f; // 5 FPS
        default:
            return 1.0f / 60.0f;
        }
    }

    // Calcular score de prioridad combinada (Estado + Distancia + Estímulos)
    float CalculatePriorityScore() const
    {
        // Estado (StatePriority)
        float StateWeight = 0.6f;
        float StatePriority = 0.0f;

        switch (PriorityLevel)
        {
        case 5:
            StatePriority = 100.0f;
            break; // Attack
        case 4:
            StatePriority = 95.0f;
            break; // TakeDamage
        case 3:
            StatePriority = 80.0f;
            break; // Chase
        case 2:
            StatePriority = 60.0f;
            break; // Seek
        case 1:
            StatePriority = 30.0f;
            break; // WalkAround
        case 0:
            StatePriority = 10.0f;
            break; // Idle
        case 6:
            StatePriority = 0.0f;
            break; // Dead
        }

        // Distancia (DistanceFactor)
        float DistanceWeight = 0.3f;
        float DistanceFactor = 0.0f;

        if (DistanceToPlayer < 200.0f)
            DistanceFactor = 1.0f;
        else if (DistanceToPlayer < 500.0f)
            DistanceFactor = 0.7f;
        else if (DistanceToPlayer < 800.0f)
            DistanceFactor = 0.4f;
        else
            DistanceFactor = 0.1f;

        // Estímulos (StimulusIntensity)
        float StimulusWeight = 0.1f;
        float StimulusFactor = FMath::Clamp(StimulusIntensity / 100.0f, 0.0f, 1.0f);

        // Score combinado
        return (StateWeight * StatePriority) +
               (DistanceWeight * DistanceFactor * 100.0f) +
               (StimulusWeight * StimulusFactor * 100.0f);
    }
};
