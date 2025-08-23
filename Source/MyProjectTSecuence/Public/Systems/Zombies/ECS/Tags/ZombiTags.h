// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiTags.generated.h"

// Tags especializados para filtrado inteligente de entidades
// OPTIMIZADO para queries eficientes y enfoque híbrido (Estados + Tags)

// Tag para entidades activas (no muertas) - Query base
USTRUCT()
struct FActiveTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades muertas (no procesar) - Query base
USTRUCT()
struct FDeadTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades persiguiendo al jugador - Query específica
USTRUCT()
struct FChasingTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades atacando - Query específica
USTRUCT()
struct FAttackingTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades visibles en frustum de cámara - Culling
USTRUCT()
struct FInFrustumTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades de alta prioridad (60 FPS) - LOD
USTRUCT()
struct FHighPriorityTag : public FMassTag
{
    GENERATED_BODY()
};