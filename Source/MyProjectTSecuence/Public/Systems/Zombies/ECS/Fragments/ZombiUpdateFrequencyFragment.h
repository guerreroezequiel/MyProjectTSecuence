#pragma once

#include "MassEntityTypes.h"
#include "Engine/Engine.h"
#include "ZombiUpdateFrequencyFragment.generated.h"

/**
 * @brief Prioridades de update para optimizar rendimiento
 *
 * Con cámara isométrica fija y distancia máxima de 1500 unidades,
 * podemos optimizar significativamente el procesamiento
 */
UENUM(BlueprintType)
enum class EZombiUpdatePriority : uint8
{
    Critical, // 60 FPS - zombies muy cercanos (< 300 unidades) - AI completa
    High,     // 30 FPS - zombies visibles (< 600 unidades) - AI básica
    Normal,   // 15 FPS - zombies de fondo (< 1000 unidades) - movimiento simple
    Low       // 5 FPS - zombies lejanos (< 1500 unidades) - solo posición
};

/**
 * @brief Fragmento para controlar la frecuencia de update de zombies
 *
 * Este fragmento permite procesar zombies a diferentes frecuencias
 * basado en su distancia al jugador, optimizando significativamente
 * el rendimiento para 1000+ entidades
 */
USTRUCT()
struct FZombiUpdateFrequencyFragment : public FMassFragment
{
    GENERATED_BODY()

    // Intervalo de update en segundos (calculado dinámicamente)
    UPROPERTY()
    float UpdateInterval = 1.0f / 60.0f;

    // Último tiempo en que se procesó esta entidad
    UPROPERTY()
    float LastUpdateTime = 0.0f;

    // Prioridad actual de update
    UPROPERTY()
    EZombiUpdatePriority Priority = EZombiUpdatePriority::Normal;

    // Distancia al jugador (calculada cada frame)
    UPROPERTY()
    float DistanceToPlayer = 0.0f;

    // ¿Necesita ser procesado en este frame?
    UPROPERTY()
    bool bNeedsUpdate = true;

    // ¿Está dentro del rango de procesamiento? (máximo 1500 unidades)
    UPROPERTY()
    bool bIsInRange = true;

    // Constructor por defecto
    FZombiUpdateFrequencyFragment()
    {
        UpdateInterval = 1.0f / 60.0f;
        LastUpdateTime = 0.0f;
        Priority = EZombiUpdatePriority::Normal;
        DistanceToPlayer = 0.0f;
        bNeedsUpdate = true;
        bIsInRange = true;
    }

    /**
     * @brief Calcula el intervalo de update basado en la distancia al jugador
     * @param DistanceToPlayer Distancia en unidades al jugador
     * @return Intervalo de update en segundos
     */
    static float CalculateUpdateInterval(float DistanceToPlayer)
    {
        // Con cámara isométrica fija, podemos optimizar más agresivamente
        if (DistanceToPlayer < 300.0f)
            return 1.0f / 60.0f; // 60 FPS - Critical
        if (DistanceToPlayer < 600.0f)
            return 1.0f / 30.0f; // 30 FPS - High
        if (DistanceToPlayer < 1000.0f)
            return 1.0f / 15.0f; // 15 FPS - Normal
        if (DistanceToPlayer < 1500.0f)
            return 1.0f / 5.0f; // 5 FPS - Low
        return 1.0f / 1.0f;     // 1 FPS - Muy lejos, casi no procesar
    }

    /**
     * @brief Calcula la prioridad basada en la distancia al jugador
     * @param DistanceToPlayer Distancia en unidades al jugador
     * @return Prioridad de update
     */
    static EZombiUpdatePriority CalculatePriority(float DistanceToPlayer)
    {
        if (DistanceToPlayer < 300.0f)
            return EZombiUpdatePriority::Critical;
        if (DistanceToPlayer < 600.0f)
            return EZombiUpdatePriority::High;
        if (DistanceToPlayer < 1000.0f)
            return EZombiUpdatePriority::Normal;
        if (DistanceToPlayer < 1500.0f)
            return EZombiUpdatePriority::Low;
        return EZombiUpdatePriority::Low; // Por defecto
    }

    /**
     * @brief Verifica si la entidad está dentro del rango de procesamiento
     * @param DistanceToPlayer Distancia en unidades al jugador
     * @return true si está en rango (máximo 1500 unidades)
     */
    static bool IsInRange(float DistanceToPlayer)
    {
        return DistanceToPlayer <= 1500.0f;
    }

    /**
     * @brief Verifica si es tiempo de procesar esta entidad
     * @param CurrentTime Tiempo actual del juego
     * @return true si necesita ser procesado
     */
    bool ShouldUpdate(float CurrentTime) const
    {
        return bIsInRange && (CurrentTime - LastUpdateTime >= UpdateInterval);
    }

    /**
     * @brief Actualiza la información de frecuencia basada en la distancia
     * @param NewDistanceToPlayer Nueva distancia al jugador
     * @param CurrentTime Tiempo actual del juego
     */
    void UpdateFrequencyInfo(float NewDistanceToPlayer, float CurrentTime)
    {
        this->DistanceToPlayer = NewDistanceToPlayer;
        this->bIsInRange = IsInRange(NewDistanceToPlayer);
        this->UpdateInterval = CalculateUpdateInterval(NewDistanceToPlayer);
        this->Priority = CalculatePriority(NewDistanceToPlayer);
        this->LastUpdateTime = CurrentTime;
    }
};