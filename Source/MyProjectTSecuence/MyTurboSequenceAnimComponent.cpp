// Fill out your copyright notice in the Description page of Project Settings.

#include "MyTurboSequenceAnimComponent.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"

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

void UMyTurboSequenceAnimComponent::InitializeTurboSequence(UTurboSequence_MeshAsset_Lf *TSAsset, const FTransform &SpawnTransform)
{
    if (!TSAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("TSAsset no asignado"));
        return;
    }

    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TSAsset;

    MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        GetWorld());

    ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(0, MeshData);
}
