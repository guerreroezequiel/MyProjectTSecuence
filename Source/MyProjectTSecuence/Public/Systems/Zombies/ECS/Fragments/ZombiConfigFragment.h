// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "Engine/Engine.h"
#include "ZombiConfigFragment.generated.h"

/**
 * Fragmento de configuración global para zombies
 * OPTIMIZACIÓN: Datos compartidos para evitar duplicación en cada entidad
 * Se usa como Shared Fragment para reducir memoria y mejorar cache locality
 */
USTRUCT()
struct FZombiConfigFragment : public FMassSharedFragment
{
    GENERATED_BODY()

    // Configuración de movimiento (16 bytes)
    UPROPERTY()
    float DefaultMovementSpeed = 50.0f;

    UPROPERTY()
    float ChaseSpeed = 200.0f;

    UPROPERTY()
    float RotationSpeed = 90.0f;

    UPROPERTY()
    float DirectionChangeInterval = 3.0f;

    // Configuración de comportamiento (16 bytes)
    UPROPERTY()
    float ChaseDistance = 1000.0f;

    UPROPERTY()
    float MovementRadius = 500.0f;

    UPROPERTY()
    float StimulusDecayTime = 5.0f;

    UPROPERTY()
    float ActionTimeout = 2.0f;

    // Configuración de optimización (8 bytes)
    UPROPERTY()
    uint8 DefaultUpdateFrequency = 60;

    UPROPERTY()
    uint8 MaxBatchSize = 64;

    UPROPERTY()
    uint8 LODDistance1 = 255; // Distancia para LOD 1 (máximo 255 para uint8)

    UPROPERTY()
    uint8 LODDistance2 = 255; // Distancia para LOD 2 (máximo 255 para uint8)

    UPROPERTY()
    uint8 LODDistance3 = 255; // Distancia para LOD 3 (máximo 255 para uint8)

    UPROPERTY()
    uint8 Reserved1 = 0;

    UPROPERTY()
    uint8 Reserved2 = 0;

    UPROPERTY()
    uint8 Reserved3 = 0;

    // Constructor con valores por defecto optimizados
    FZombiConfigFragment()
    {
        DefaultMovementSpeed = 50.0f;
        ChaseSpeed = 200.0f;
        RotationSpeed = 90.0f;
        DirectionChangeInterval = 2.0f; // Acelerar para testing (2 segundos)
        ChaseDistance = 1000.0f;
        MovementRadius = 500.0f;
        StimulusDecayTime = 5.0f;
        ActionTimeout = 2.0f;
        DefaultUpdateFrequency = 60;
        MaxBatchSize = 64;
        LODDistance1 = 255;
        LODDistance2 = 255;
        LODDistance3 = 255;
    }

    // Funciones de utilidad para LOD
    uint8 GetLODLevel(float Distance) const
    {
        if (Distance <= LODDistance1)
            return 0; // LOD 0 - Máxima calidad
        if (Distance <= LODDistance2)
            return 1; // LOD 1 - Calidad media
        if (Distance <= LODDistance3)
            return 2; // LOD 2 - Calidad baja
        return 3;     // LOD 3 - Mínima calidad
    }

    uint8 GetUpdateFrequencyForLOD(uint8 LODLevel) const
    {
        switch (LODLevel)
        {
        case 0:
            return 60; // 60 FPS
        case 1:
            return 30; // 30 FPS
        case 2:
            return 15; // 15 FPS
        case 3:
            return 5; // 5 FPS
        default:
            return DefaultUpdateFrequency;
        }
    }

    // Funciones de utilidad para batch processing
    uint8 GetOptimalBatchSize() const
    {
        return MaxBatchSize;
    }
};
