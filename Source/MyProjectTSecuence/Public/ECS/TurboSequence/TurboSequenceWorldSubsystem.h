#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "EngineUtils.h"
#include "TurboSequence_Lf/Public/TurboSequence_Manager_Lf.h"
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
		// Si el singleton global apunta a nuestro Manager, liberarlo para evitar puntero colgante en PIE/multiworld.
		if (ATurboSequence_Manager_Lf::Instance != nullptr && Manager.IsValid() && ATurboSequence_Manager_Lf::Instance == Manager.Get())
		{
			ATurboSequence_Manager_Lf::Instance = nullptr;
			UE_LOG(LogTemp, Log, TEXT("TS WorldSubsystem: Cleared TurboSequence_Manager_Lf::Instance on Deinitialize()"));
		}
		Manager = nullptr;
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

	ATurboSequence_Manager_Lf* GetOrFindManager(UWorld* World)
	{
		if (Manager.IsValid())
		{
			return Manager.Get();
		}
		if (!World)
		{
			return nullptr;
		}
		for (TActorIterator<ATurboSequence_Manager_Lf> It(World); It; ++It)
		{
			Manager = *It;
			// Asignar singleton global solo si está vacío o ya apunta al mismo objeto (mitiga multiworld/PIE).
			if (ATurboSequence_Manager_Lf::Instance == nullptr || ATurboSequence_Manager_Lf::Instance == Manager.Get())
			{
				ATurboSequence_Manager_Lf::Instance = Manager.Get();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("TS WorldSubsystem: TurboSequence_Manager_Lf::Instance ya estaba seteado a otro World. Evitando overwrite."));
			}
			UE_LOG(LogTemp, Log, TEXT("TS WorldSubsystem: Found TurboSequence Manager in World"));
			return Manager.Get();
		}
		if (!bLoggedManagerMissingOnce)
		{
			bLoggedManagerMissingOnce = true;
			UE_LOG(LogTemp, Warning, TEXT("TS WorldSubsystem: TurboSequence Manager not found in World (ensure BP exists and BeginPlay ran)"));
		}
		return nullptr;
	}

private:
	uint64 LastSolvedFrame = MAX_uint64;
	TWeakObjectPtr<ATurboSequence_Manager_Lf> Manager;
	bool bLoggedManagerMissingOnce = false;
};
