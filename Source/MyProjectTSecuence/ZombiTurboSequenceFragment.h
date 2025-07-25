// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * Fragmento que contiene datos para instancias visuales de TurboSequence
 * Implementa State Sync: separa lógica de visualización
 */
USTRUCT()
struct MYPROJECTTSECUENCE_API FZombiTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	FZombiTurboSequenceFragment()
	{
		bIsVisualInstanceValid = false;
		UpdateGroupIndex = 0;
	}

	// Datos de la instancia visual de TurboSequence
	UPROPERTY()
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// Blend Space para animaciones (State Sync)
	UPROPERTY()
	FTurboSequence_AnimMinimalBlendSpaceCollection_Lf BlendSpaceData;

	// Asset de TurboSequence (referencia)
	UPROPERTY()
	class UTurboSequence_MeshAsset_Lf *TurboSequenceAsset = nullptr;

	// Indica si la instancia visual es válida
	UPROPERTY()
	bool bIsVisualInstanceValid = false;

	// Índice del grupo de actualización para optimización
	UPROPERTY()
	int32 UpdateGroupIndex = 0;
};
