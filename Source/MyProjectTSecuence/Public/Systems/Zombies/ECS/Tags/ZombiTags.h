// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "CoreMinimal.h"
#include "ZombiTags.generated.h"

/**
 * Tags optimizados para el sistema ECS de zombies
 * DOP: Tags para filtrado rápido en queries
 * Optimizado para 10,000 entidades
 */

// ===== TAGS DE ESTADO PRINCIPAL =====

/**
 * Tag para entidades en estado de persecución
 * Usado para procesamiento prioritario (60 FPS)
 */
USTRUCT()
struct FChasingTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades caminando
 * Usado para procesamiento de alta prioridad (30 FPS)
 */
USTRUCT()
struct FWalkingTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades inactivas
 * Usado para procesamiento de prioridad normal (15 FPS)
 */
USTRUCT()
struct FIdleTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades muertas
 * Usado para procesamiento de baja prioridad (5 FPS) o exclusión
 */
USTRUCT()
struct FDeadTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades atacando
 * Usado para procesamiento prioritario (60 FPS)
 */
USTRUCT()
struct FAttackingTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades huyendo
 * Usado para procesamiento de alta prioridad (30 FPS)
 */
USTRUCT()
struct FFleeingTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE ACCIÓN =====

/**
 * Tag para entidades rugiendo
 * Usado para efectos de sonido y animación
 */
USTRUCT()
struct FRoaringTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades investigando
 * Usado para comportamiento de IA
 */
USTRUCT()
struct FInvestigatingTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades alertando
 * Usado para propagación de información
 */
USTRUCT()
struct FAlertingTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE CONDICIÓN =====

/**
 * Tag para entidades heridas
 * Usado para efectos visuales y comportamiento
 */
USTRUCT()
struct FInjuredTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades en estado crítico
 * Usado para efectos visuales y comportamiento
 */
USTRUCT()
struct FCriticalTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades sangrando
 * Usado para efectos de partículas
 */
USTRUCT()
struct FBleedingTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE HORDA =====

/**
 * Tag para líderes de horda
 * Usado para comportamiento grupal
 */
USTRUCT()
struct FHordeLeaderTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades siguiendo
 * Usado para comportamiento grupal
 */
USTRUCT()
struct FHordeFollowingTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades en enjambre
 * Usado para comportamiento grupal
 */
USTRUCT()
struct FHordeSwarmingTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE LOD (Level of Detail) =====

/**
 * Tag para entidades de LOD crítico (muy cercanas)
 * Procesamiento a 60 FPS
 */
USTRUCT()
struct FLODCriticalTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades de LOD alto (visibles)
 * Procesamiento a 30 FPS
 */
USTRUCT()
struct FLODHighTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades de LOD normal (de fondo)
 * Procesamiento a 15 FPS
 */
USTRUCT()
struct FLODNormalTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades de LOD bajo (lejanas)
 * Procesamiento a 5 FPS
 */
USTRUCT()
struct FLODLowTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE BATCH PROCESSING =====

/**
 * Tag para entidades en batch crítico
 * Tamaño de lote: 32 entidades
 */
USTRUCT()
struct FBatchCriticalTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades en batch alto
 * Tamaño de lote: 64 entidades
 */
USTRUCT()
struct FBatchHighTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades en batch normal
 * Tamaño de lote: 128 entidades
 */
USTRUCT()
struct FBatchNormalTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades en batch bajo
 * Tamaño de lote: 256 entidades
 */
USTRUCT()
struct FBatchLowTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE SISTEMA =====

/**
 * Tag para entidades activas
 * Usado para filtrar entidades que deben ser procesadas
 */
USTRUCT()
struct FActiveTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades inicializadas
 * Usado para filtrar entidades completamente inicializadas
 */
USTRUCT()
struct FInitializedTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades que necesitan update
 * Usado para control de frecuencia de update
 */
USTRUCT()
struct FNeedsUpdateTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades que necesitan sincronización visual
 * Usado para TurboSequence
 */
USTRUCT()
struct FNeedsVisualSyncTag : public FMassTag
{
    GENERATED_BODY()
};

// ===== TAGS DE DEBUG =====

/**
 * Tag para entidades en modo debug
 * Usado para mostrar información de debug
 */
USTRUCT()
struct FDebugTag : public FMassTag
{
    GENERATED_BODY()
};

/**
 * Tag para entidades seleccionadas en debug
 * Usado para mostrar información detallada
 */
USTRUCT()
struct FDebugSelectedTag : public FMassTag
{
    GENERATED_BODY()
};