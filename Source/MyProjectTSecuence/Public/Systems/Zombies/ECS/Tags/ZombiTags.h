// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "ZombiTags.generated.h"

// Tags especializados para filtrado inteligente de entidades
// OPTIMIZADO para queries eficientes y paralelización

// Tag para entidades activas (no muertas)
USTRUCT()
struct FActiveTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades en movimiento
USTRUCT()
struct FMovingTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades muertas
USTRUCT()
struct FDeadTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades visibles (en frustum de cámara)
USTRUCT()
struct FVisibleTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades que necesitan actualización de animación
USTRUCT()
struct FNeedsAnimationUpdateTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades que necesitan sincronización visual
USTRUCT()
struct FNeedsVisualSyncTag : public FMassTag
{
    GENERATED_BODY()
};

// Tag para entidades que están persiguiendo al jugador
USTRUCT()
struct FChasingTag : public FMassTag
{
    GENERATED_BODY()
};