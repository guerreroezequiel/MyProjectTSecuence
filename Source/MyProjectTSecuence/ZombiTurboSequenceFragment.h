// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "ZombiTurboSequenceFragment.generated.h"

// Fragmento que almacena la referencia a la instancia visual de TurboSequence
// Solo contiene datos visuales, sin lógica de juego
USTRUCT(BlueprintType)
struct FZombiTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	// Referencia a la instancia visual de TurboSequence
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// Indica si la instancia visual está inicializada
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsVisualInstanceValid = false;

	// ID del grupo de actualización para optimización
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 UpdateGroupIndex = 0;

	// Asset de TurboSequence usado para esta instancia
	// Las animaciones se manejan por nombre desde este asset
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UTurboSequence_MeshAsset_Lf *TurboSequenceAsset = nullptr;
};
