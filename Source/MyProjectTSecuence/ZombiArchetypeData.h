// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Animation/AnimSequence.h"
#include "ZombiArchetypeData.generated.h"

// Datos compartidos para archetypes de zombis
// OPTIMIZADO para reducir duplicación de datos entre entidades
USTRUCT(BlueprintType)
struct FZombiArchetypeData
{
    GENERATED_BODY()

    // Asset compartido para todas las entidades del archetype
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTurboSequence_MeshAsset_Lf> SharedMeshAsset = nullptr;

    // Animaciones compartidas para todas las entidades del archetype
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<TObjectPtr<UAnimSequence>> SharedAnimations;

    // Configuración base compartida
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseMovementSpeed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseRotationSpeed = 360.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BaseHealth = 100.0f;

    // Constructor
    FZombiArchetypeData()
    {
        SharedMeshAsset = nullptr;
        SharedAnimations.Empty();
        BaseMovementSpeed = 100.0f;
        BaseRotationSpeed = 360.0f;
        BaseHealth = 100.0f;
    }

    // Función para obtener animación por nombre
    UAnimSequence *GetAnimationByName(const FString &AnimationName) const
    {
        for (UAnimSequence *Animation : SharedAnimations)
        {
            if (Animation && Animation->GetName().Contains(AnimationName, ESearchCase::IgnoreCase))
            {
                return Animation;
            }
        }
        return nullptr;
    }

    // Función para obtener animación por índice
    UAnimSequence *GetAnimationByIndex(int32 Index) const
    {
        if (SharedAnimations.IsValidIndex(Index))
        {
            return SharedAnimations[Index];
        }
        return nullptr;
    }
};