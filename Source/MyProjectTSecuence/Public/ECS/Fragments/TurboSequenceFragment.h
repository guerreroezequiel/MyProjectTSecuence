#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequenceFragment.generated.h"

USTRUCT()
struct MYPROJECTTSECUENCE_API FTurboSequenceFragment : public FMassFragment
{
	GENERATED_BODY()

	FTurboSequence_MeshSpawnData_Lf SpawnData;
	TObjectPtr<UAnimSequence> Anim = nullptr;
	FTurboSequence_AnimPlaySettings_Lf AnimSettings;
	FTurboSequence_MinimalMeshData_Lf Instance;
	int32 UpdateGroupIndex = 0;
	bool bHasInstance = false;
};
