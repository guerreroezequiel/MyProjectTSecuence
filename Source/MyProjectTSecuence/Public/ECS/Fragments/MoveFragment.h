// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MoveFragment.generated.h"

/**
 * Fragmento que contiene los parámetros de movimiento de una entidad.
 * Define la velocidad de movimiento de la entidad.
 */
USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FMoveFragment : public FMassFragment
{
    GENERATED_BODY()

    FMoveFragment() = default;
    explicit FMoveFragment(float InSpeed) : Speed(InSpeed) {}

    /** Velocidad de movimiento en unidades por segundo */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float Speed = 100.0f; // Valor por defecto razonable
};
