#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TurboSequenceWorldSubsystem.generated.h"

UCLASS()
class MYPROJECTTSECUENCE_API UTurboSequenceWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	static UTurboSequenceWorldSubsystem* Get(const UWorld* World)
	{
		return World ? World->GetSubsystem<UTurboSequenceWorldSubsystem>() : nullptr;
	}

	virtual void Initialize(FSubsystemCollectionBase& Collection) override
	{
		Super::Initialize(Collection);
		LastSolvedFrame = MAX_uint64; // invalid at start
	}

	virtual void Deinitialize() override
	{
		Super::Deinitialize();
	}

	bool ShouldSolveThisFrame(uint64 CurrentFrame)
	{
		if (LastSolvedFrame == CurrentFrame)
		{
			return false;
		}
		LastSolvedFrame = CurrentFrame;
		return true;
	}

private:
	uint64 LastSolvedFrame = MAX_uint64;
};
