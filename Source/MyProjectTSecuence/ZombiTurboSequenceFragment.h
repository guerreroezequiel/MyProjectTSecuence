// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Data_Lf.h"
#include "MyTurboSequenceAnimComponent.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * Fragmento para manejar la representación visual de entidades zombi usando TurboSequence
 * Enfoque: State Sync Architecture - Blend Space para animaciones
 */
USTRUCT()
struct FZombiTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	// Handle para la instancia visual de TurboSequence
	UPROPERTY()
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// Asset de TurboSequence que contiene el mesh y animaciones
	UPROPERTY()
	TObjectPtr<UTurboSequence_MeshAsset_Lf> TurboSequenceAsset;

	// Índice del grupo de actualización para distribución de carga
	UPROPERTY()
	int32 UpdateGroupIndex = 0;

	// Blend Space data para animaciones (enfoque principal)
	UPROPERTY()
	FTurboSequence_AnimMinimalBlendSpaceCollection_Lf BlendSpaceData;

	// Sistema de transiciones suaves
	UPROPERTY()
	EZombiState CurrentAnimationState = EZombiState::Idle;
	UPROPERTY()
	EZombiState TargetAnimationState = EZombiState::Idle;
	UPROPERTY()
	float TransitionProgress = 0.0f;
	UPROPERTY()
	float TransitionDuration = 0.5f; // Duración de transición en segundos

	FZombiTurboSequenceFragment()
	{
		TurboSequenceAsset = nullptr;
		UpdateGroupIndex = 0;
	}
};
