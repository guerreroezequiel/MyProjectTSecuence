// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "MassEntityTypes.h"
#include "FMassActorFragment.generated.h"

// Fragmento que almacena una referencia al actor asociado a la entidad Mass
USTRUCT(BlueprintType)
struct FZombiActorFragment : public FMassFragment
{
	GENERATED_BODY()

	// Referencia al actor asociado a esta entidad Mass
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor *Actor = nullptr;

	// Constructor por defecto
	FZombiActorFragment() : Actor(nullptr) {}

	// Constructor con actor
	FZombiActorFragment(AActor *InActor) : Actor(InActor) {}

	// Función helper para obtener el actor de forma segura
	AActor *Get() const { return Actor; }
};
