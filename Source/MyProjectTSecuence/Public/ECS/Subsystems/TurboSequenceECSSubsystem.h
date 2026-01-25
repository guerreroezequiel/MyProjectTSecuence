#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TurboSequence_Data_Lf.h"
#include "TurboSequenceECSSubsystem.generated.h"

class ATurboSequence_Manager_Lf;

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceECSSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	static UTurboSequenceECSSubsystem* Get(const UWorld* World);

	ATurboSequence_Manager_Lf* EnsureManager_GameThread();
	void SolveGroup_GameThread(float DeltaTime, int32 GroupIndex);

private:
	TWeakObjectPtr<ATurboSequence_Manager_Lf> CachedManager;
};
