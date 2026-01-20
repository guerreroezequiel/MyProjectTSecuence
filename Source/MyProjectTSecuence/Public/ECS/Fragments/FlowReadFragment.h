// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "FlowReadFragment.generated.h"

/**
 * Fragmento que almacena la información de lectura del FlowField.
 * Contiene la dirección de movimiento y la época en que se leyó el campo.
 */
USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FFlowReadFragment : public FMassFragment
{
    GENERATED_BODY()

    FFlowReadFragment() = default;
    
    explicit FFlowReadFragment(const FVector& InDirWS, int32 InEpochSeen = -1, bool bInValid = false)
        : DirWS(InDirWS)
        , EpochSeen(InEpochSeen)
        , bValid(bInValid)
    {
    }

    /** Dirección de movimiento en espacio mundo */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowField|Read")
    FVector DirWS = FVector::ZeroVector;

    /** Época del campo de flujo cuando se leyó */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowField|Read")
    int32 EpochSeen = -1;

    /** Indica si la lectura es válida */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowField|Read")
    bool bValid = false;

    /** Dirección válida más reciente para fallback cuando no hay snapshot o es cero */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FlowField|Read")
    FVector CachedLastValidDirWS = FVector::ZeroVector;

    /** Cuántos frames podemos seguir usando la dirección en cache si no hay lectura válida */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FlowField|Read")
    int32 FallbackFramesLeft = 0;
};
