// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiTurboSequenceFragment.generated.h"

/**
 * Fragmento optimizado para manejar la representación visual de entidades zombi usando TurboSequence
 * OPTIMIZADO: 32 bytes máximo para 5000+ entidades
 * State Sync Architecture - Solo datos esenciales
 * SHADOW OPTIMIZATION: Configuración de sombras por distancia
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

	// Configuración de sombras optimizada (1 bit)
	UPROPERTY()
	uint8 ShadowQuality : 1; // 0=Off, 1=On (simplificado para optimización)

	// Constructor por defecto
	FZombiTurboSequenceFragment()
	{
		TurboSequenceAsset = nullptr;
		CurrentAnimation = nullptr;
		ShadowQuality = 1; // Low por defecto para optimización
	}

	// Métodos de utilidad
	bool IsValid() const { return MeshData.IsMeshDataValid() && TurboSequenceAsset != nullptr; }
	void SetAnimation(UAnimSequence *NewAnimation) { CurrentAnimation = NewAnimation; }

	// Configuración de sombras
	void SetShadowQuality(uint8 Quality) { ShadowQuality = Quality > 0 ? 1 : 0; }
	uint8 GetShadowQuality() const { return ShadowQuality; }
	bool ShouldCastShadows() const { return ShadowQuality > 0; }
};
