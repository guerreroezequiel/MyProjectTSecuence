// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * ✅ PATRÓN OFICIAL TURBOSEQUENCE: Fragment que almacena datos lógicos
 * ECS prepara datos → Controller aplica a TurboSequence después del "big loop"
 * Optimizado para 10k+ entidades con memory layout eficiente
 */
USTRUCT()
struct FZombiTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	// ═══ CORE TURBOSEQUENCE DATA ═══
	// Handle para la instancia visual (16 bytes)
	UPROPERTY()
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// Asset de TurboSequence (8 bytes)
	UPROPERTY()
	TObjectPtr<UTurboSequence_MeshAsset_Lf> TurboSequenceAsset;

	// ═══ ANIMATION STATE ═══
	// Animación actual (8 bytes)
	UPROPERTY()
	TObjectPtr<UAnimSequence> CurrentAnimation;

	// ═══ PENDING OPERATIONS (PATRÓN OFICIAL) ═══
	// Transform pendiente para aplicar en controller
	UPROPERTY()
	FTransform PendingTransform;

	// Settings de animación pendientes
	UPROPERTY()
	FTurboSequence_AnimPlaySettings_Lf PendingAnimationSettings;

	// ═══ UPDATE FLAGS (OPTIMIZADO) ═══
	// Flags de actualización compactos (3 bits total)
	UPROPERTY()
	uint8 bNeedsAnimationUpdate : 1;

	UPROPERTY()
	uint8 bNeedsTransformUpdate : 1;

	UPROPERTY()
	uint8 ShadowQuality : 1; // 0=Off, 1=On

	// ═══ CONSTRUCTOR OPTIMIZADO ═══
	FZombiTurboSequenceFragment()
		: TurboSequenceAsset(nullptr), CurrentAnimation(nullptr), PendingTransform(FTransform::Identity), bNeedsAnimationUpdate(false), bNeedsTransformUpdate(false), ShadowQuality(1) // High quality por defecto
	{
		// PendingAnimationSettings se inicializa automáticamente
	}

	// ═══ UTILITY METHODS ═══
	FORCEINLINE bool IsValid() const
	{
		return MeshData.IsMeshDataValid() && TurboSequenceAsset != nullptr;
	}

	FORCEINLINE void SetAnimation(UAnimSequence *NewAnimation)
	{
		CurrentAnimation = NewAnimation;
	}

	// ═══ SHADOW OPTIMIZATION ═══
	FORCEINLINE void SetShadowQuality(uint8 Quality)
	{
		ShadowQuality = Quality > 0 ? 1 : 0;
	}

	FORCEINLINE bool ShouldCastShadows() const
	{
		return ShadowQuality > 0;
	}
};
