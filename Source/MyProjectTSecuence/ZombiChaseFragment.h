// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "ZombiChaseFragment.generated.h"

// Fragmento para manejar el comportamiento de persecución al jugador
// OPTIMIZADO para cache locality y paralelización
USTRUCT()
struct FZombiChaseFragment : public FMassFragment
{
    GENERATED_BODY()

    // Posición del jugador objetivo
    UPROPERTY()
    FVector PlayerTargetPosition = FVector::ZeroVector;

    // Distancia máxima de persecución
    UPROPERTY()
    float ChaseDistance = 1000.0f;

    // Velocidad de persecución (más alta que movimiento normal)
    UPROPERTY()
    float ChaseSpeed = 200.0f;

    // Timer para el ciclo de persecución (10 segundos)
    UPROPERTY()
    float ChaseCycleTimer = 0.0f;

    // Duración de la persecución (5 segundos)
    UPROPERTY()
    float ChaseDuration = 5.0f;

    // Estado de persecución
    UPROPERTY()
    bool bIsChasing = false;

    // Constructor
    FZombiChaseFragment()
    {
        PlayerTargetPosition = FVector::ZeroVector;
        ChaseDistance = 1000.0f;
        ChaseSpeed = 200.0f;
        ChaseCycleTimer = 0.0f;
        ChaseDuration = 5.0f;
        bIsChasing = false;
    }
};