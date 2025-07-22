// Fill out your copyright notice in the Description page of Project Settings.

#include "MyTurboSequenceAnimComponent.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"

// Constructor: inicializa el componente (no hace nada especial aquí)
UMyTurboSequenceAnimComponent::UMyTurboSequenceAnimComponent()
{
    PrimaryComponentTick.bCanEverTick = false; // Cambia a true si necesitas tick
}

// Se llama automáticamente cuando el juego comienza o el actor aparece en el mundo
void UMyTurboSequenceAnimComponent::BeginPlay()
{
    Super::BeginPlay();
    // Aquí podrías inicializar cosas si lo necesitas
}

// Cambia la animación de la instancia visual de TurboSequence
void UMyTurboSequenceAnimComponent::SetTurboSequenceAnimation(UAnimSequence *NewAnimation)
{
    if (!MeshData.IsMeshDataValid() || !NewAnimation)
    {
        UE_LOG(LogTemp, Warning, TEXT("No hay instancia de TurboSequence o animación no asignada"));
        return;
    }

    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, NewAnimation, PlaySettings);
}

// Crea la instancia visual de TurboSequence en el mundo
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

// Cambia el estado lógico y actualiza la animación correspondiente
void UMyTurboSequenceAnimComponent::SetState(EZombiState NewState)
{
    CurrentState = NewState;

    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;

    if (!MeshData.IsMeshDataValid())
        return;

    switch (NewState)
    {
    case EZombiState::Idle:
        if (IdleAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, IdleAnim, PlaySettings);
        break;
    case EZombiState::Walk:
        if (WalkAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, WalkAnim, PlaySettings);
        break;
    case EZombiState::Chase:
        if (ChaseAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, ChaseAnim, PlaySettings);
        break;
    case EZombiState::Attack:
        if (AttackAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, AttackAnim, PlaySettings);
        break;
    case EZombiState::Hit:
        if (HitAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, HitAnim, PlaySettings);
        break;
    case EZombiState::Death:
        if (DeathAnim)
            ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(MeshData, DeathAnim, PlaySettings);
        break;
    }
}
