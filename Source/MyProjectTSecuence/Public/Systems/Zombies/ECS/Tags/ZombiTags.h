// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiTags.generated.h"

// Tags especializados para LOD y optimización
// OPTIMIZADO para queries eficientes - Tags por Frecuencia

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

// Tag para entidades visibles en frustum de cámara - Culling
USTRUCT()
struct FInFrustumTag : public FMassTag
{
    GENERATED_BODY()
};

// Tags por frecuencia de actualización - LOD Dinámico
USTRUCT()
struct FUpdate60FPS : public FMassTag
{
    GENERATED_BODY()
};

USTRUCT()
struct FUpdate30FPS : public FMassTag
{
    GENERATED_BODY()
};

USTRUCT()
struct FUpdate15FPS : public FMassTag
{
    GENERATED_BODY()
};

USTRUCT()
struct FUpdate5FPS : public FMassTag
{
    GENERATED_BODY()
};