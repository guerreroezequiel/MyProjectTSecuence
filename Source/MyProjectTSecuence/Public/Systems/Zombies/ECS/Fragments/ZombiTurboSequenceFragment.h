// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * ✅ PATRÓN OFICIAL: Fragmento que almacena datos lógicos para TurboSequence
 * Incluye datos pendientes para que el controlador principal aplique después del "big ECS loop"
 * Según documentación: ECS prepara datos, controlador aplica funciones TurboSequence
 */
USTRUCT()
struct FZombiTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	// Handle para la instancia visual de TurboSequence (16 bytes)
	UPROPERTY()
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// Asset de TurboSequence (8 bytes)
	UPROPERTY()
	TObjectPtr<UTurboSequence_MeshAsset_Lf> TurboSequenceAsset;

	// Animación actual (8 bytes)
	UPROPERTY()
	TObjectPtr<UAnimSequence> CurrentAnimation;

	// ✅ PATRÓN OFICIAL: Datos pendientes para aplicar en el controlador
	UPROPERTY()
	FTransform PendingTransform;

	UPROPERTY()
	FTurboSequence_AnimPlaySettings_Lf PendingAnimationSettings;

	// Flags de actualización
	UPROPERTY()
	uint8 bNeedsAnimationUpdate : 1;

	UPROPERTY()
	uint8 bNeedsTransformUpdate : 1;

	// Configuración de sombras optimizada (1 bit)
	UPROPERTY()
	uint8 ShadowQuality : 1; // 0=Off, 1=On (simplificado para optimización)

	// Constructor por defecto
	FZombiTurboSequenceFragment()
	{
		TurboSequenceAsset = nullptr;
		CurrentAnimation = nullptr;
		ShadowQuality = 1; // Low por defecto para optimización
		bNeedsAnimationUpdate = false;
		bNeedsTransformUpdate = false;
		PendingTransform = FTransform::Identity;
	}

	// Métodos de utilidad
	bool IsValid() const { return MeshData.IsMeshDataValid() && TurboSequenceAsset != nullptr; }
	void SetAnimation(UAnimSequence *NewAnimation) { CurrentAnimation = NewAnimation; }

	// Configuración de sombras
	void SetShadowQuality(uint8 Quality) { ShadowQuality = Quality > 0 ? 1 : 0; }
	uint8 GetShadowQuality() const { return ShadowQuality; }
	bool ShouldCastShadows() const { return ShadowQuality > 0; }
};
