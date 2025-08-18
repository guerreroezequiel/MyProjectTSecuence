// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_Data_Lf.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStateFragment.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * Fragmento compartido (Shared) para configuración visual de zombies
 * Centraliza thresholds y punteros de animación comunes por arquetipo/asset
 */
USTRUCT()
struct FZombiVisualSharedFragment : public FMassSharedFragment
{
	GENERATED_BODY()

	// Punteros opcionales a animaciones base
	UPROPERTY()
	TObjectPtr<UAnimSequence> SharedIdleAnimation = nullptr;
	UPROPERTY()
	TObjectPtr<UAnimSequence> SharedWalkAnimation = nullptr;
	UPROPERTY()
	TObjectPtr<UAnimSequence> SharedRunAnimation = nullptr;

	// Thresholds de sincronización
	UPROPERTY()
	float SharedMinPositionDeltaForSync = 0.5f;
	UPROPERTY()
	float SharedMinYawDeltaForSync = 1.0f;
};

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

	// Sistema de transiciones suaves (usando flags)
	UPROPERTY()
	uint8 CurrentAnimationState = 0;
	UPROPERTY()
	uint8 TargetAnimationState = 0;

	// Control de animaciones individual por entidad
	UPROPERTY()
	bool bAnimationInitialized = false;
	UPROPERTY()
	float LastSpeed = -1.0f;
	UPROPERTY()
	float AnimationUpdateTimer = 0.0f;
	UPROPERTY()
	float LastAnimationUpdateTime = 0.0f;
	// Frecuencia de actualización eliminada para mantener orden de procesamiento

	// Cache de animaciones para optimizar búsquedas
	UPROPERTY()
	TObjectPtr<UAnimSequence> CachedIdleAnimation = nullptr;
	UPROPERTY()
	TObjectPtr<UAnimSequence> CachedWalkAnimation = nullptr;
	UPROPERTY()
	TObjectPtr<UAnimSequence> CachedRunAnimation = nullptr;
	UPROPERTY()
	bool bAnimationsCached = false;

	// Control de transiciones suaves
	UPROPERTY()
	TObjectPtr<UAnimSequence> CurrentAnimation = nullptr;
	UPROPERTY()
	TObjectPtr<UAnimSequence> TargetAnimation = nullptr;
	UPROPERTY()
	float TransitionProgress = 0.0f;
	UPROPERTY()
	float TransitionDuration = 0.3f; // Duración de transición en segundos
	UPROPERTY()
	bool bIsTransitioning = false;

	// Dirty flags y caches simples para reducir trabajo por frame
	UPROPERTY()
	FVector LastSyncedPosition = FVector::ZeroVector;
	UPROPERTY()
	float LastSyncedYaw = 0.0f;
	UPROPERTY()
	bool bTransformDirty = true;
	UPROPERTY()
	bool bAnimDirty = true;
	UPROPERTY()
	float MinPositionDeltaForSync = 0.5f; // umbral simple
	UPROPERTY()
	float MinYawDeltaForSync = 1.0f; // grados

	// Dirección de la animación (para sincronizar con movimiento) - TEMPORALMENTE COMENTADO
	// UPROPERTY()
	// FVector AnimationDirection = FVector::ForwardVector;
	// UPROPERTY()
	// float LastDirectionChangeTime = 0.0f;

	FZombiTurboSequenceFragment()
	{
		TurboSequenceAsset = nullptr;
		UpdateGroupIndex = 0;
	}
};
