#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequenceInstanceFragment.generated.h"

USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FTurboSequenceInstanceFragment : public FMassFragment
{
	GENERATED_BODY()

	FTurboSequenceInstanceFragment()
		: MeshData()
		, MeshAsset(nullptr)
		, bInstanceCreated(false)
	{
	}

	// TS minimal mesh data (ID/handle)
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	FTurboSequence_MinimalMeshData_Lf MeshData;

	// TS mesh asset to use when creating the visual instance
	UPROPERTY(EditAnywhere, Category="TurboSequence")
	TObjectPtr<UTurboSequence_MeshAsset_Lf> MeshAsset;

	// Internal flag to indicate instance has been created in TS manager
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	bool bInstanceCreated;

	// ---- Sync gating state (HOT/WARM/COLD + dirty checks) ----

	// Última posición sincronizada con TS (para delta pos)
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	FVector LastSyncedLocation = FVector::ZeroVector;

	// Último yaw sincronizado (para delta yaw en grados)
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	float LastSyncedYaw = 0.0f;

	// Frame en el que se realizó el último sync (para periodos WARM)
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	uint64 LastSyncedFrame = 0;

	// Contador para periodos WARM (sync cada N frames)
	UPROPERTY(VisibleAnywhere, Category="TurboSequence")
	int32 WarmCounter = 0;
};