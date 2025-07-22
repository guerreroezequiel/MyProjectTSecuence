// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "MyTurboSequenceAnimComponent.h" // Para usar EZombiState
#include "ZombiStateFragment.generated.h"

// Fragmento que almacena el estado lógico del zombi para Mass Entity
USTRUCT(BlueprintType)
struct FZombiStateFragment : public FMassFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EZombiState State = EZombiState::Idle;
};
