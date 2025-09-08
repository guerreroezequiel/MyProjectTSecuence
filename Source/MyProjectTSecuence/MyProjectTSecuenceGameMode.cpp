// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectTSecuenceGameMode.h"
#include "MyProjectTSecuenceCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/Engine.h"
#include "MassEntitySubsystem.h"
#include "Systems/Zombies/ECS/Subsystems/ZombiSpawnerSubsystem.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "TurboSequence_MinimalData_Lf.h"

AMyProjectTSecuenceGameMode::AMyProjectTSecuenceGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}

void AMyProjectTSecuenceGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Obtiene el ZombiMassSubsystem
	ZombiMassSubsystem = GetWorld()->GetSubsystem<UZombiMassSubsystem>();

	if (ZombiMassSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("ZombiMassSubsystem inicializado correctamente"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener ZombiMassSubsystem"));
	}

	// Obtiene el ZombiSpawnerSubsystem
	ZombiSpawnerSubsystem = GetWorld()->GetSubsystem<UZombiSpawnerSubsystem>();

	if (ZombiSpawnerSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("ZombiSpawnerSubsystem inicializado correctamente"));

		// 🚀 CONFIGURAR NUEVO ASSET DE TURBOSEQUENCE
		// Cargar el nuevo mesh asset de zombie
		UTurboSequence_MeshAsset_Lf *ZombieTurboSequenceAsset = LoadObject<UTurboSequence_MeshAsset_Lf>(
			nullptr,
			TEXT("/Game/Characters/Mannequins/TurboSequence/TS_Zombie_MeshAsset"));

		if (ZombieTurboSequenceAsset)
		{
			// 🔍 DIAGNÓSTICO: Verificar asset cargado
			UE_LOG(LogTemp, Log, TEXT("✅ Asset cargado: %s"), *ZombieTurboSequenceAsset->GetName());

			if (ZombieTurboSequenceAsset->AnimationLibrary)
			{
				UE_LOG(LogTemp, Log, TEXT("🔍 AnimationLibrary encontrada con %d animaciones"),
					   ZombieTurboSequenceAsset->AnimationLibrary->Animations.Num());

				// Listar animaciones en el GameMode
				for (int32 i = 0; i < ZombieTurboSequenceAsset->AnimationLibrary->Animations.Num(); ++i)
				{
					const FAnimationLibraryItem_Lf &AnimItem = ZombieTurboSequenceAsset->AnimationLibrary->Animations[i];
					if (AnimItem.Animation)
					{
						UE_LOG(LogTemp, Log, TEXT("🔍 Animación en GameMode %d: %s"), i, *AnimItem.Animation->GetName());
					}
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("🔍 AnimationLibrary es NULL en GameMode"));
			}

			// Configurar el asset en el spawner subsystem
			ZombiSpawnerSubsystem->SetZombiTurboSequenceAsset(ZombieTurboSequenceAsset);
			UE_LOG(LogTemp, Log, TEXT("✅ Nuevo asset de TurboSequence configurado: %s"), *ZombieTurboSequenceAsset->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No se pudo cargar el asset TS_Zombie_MeshAsset. Verificar la ruta."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No se pudo obtener ZombiSpawnerSubsystem"));
	}
}
