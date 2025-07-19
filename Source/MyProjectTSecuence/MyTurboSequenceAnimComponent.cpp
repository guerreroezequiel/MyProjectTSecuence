// Fill out your copyright notice in the Description page of Project Settings.

#include "MyTurboSequenceAnimComponent.h"

UMyTurboSequenceAnimComponent::UMyTurboSequenceAnimComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Cambia a true si necesitas tick
}

void UMyTurboSequenceAnimComponent::BeginPlay()
{
    Super::BeginPlay();

    // Aquí inicializarás referencias a TurboSequence más adelante
}

void UMyTurboSequenceAnimComponent::SetTurboSequenceAnimation(UAnimSequence *NewAnimation)
{
    // Implementación pendiente (puedes dejarlo vacío por ahora)
}
