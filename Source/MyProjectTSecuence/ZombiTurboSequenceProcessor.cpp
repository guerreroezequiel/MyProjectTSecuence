// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTurboSequenceProcessor.h"
#include "MassExecutionContext.h"
#include "TurboSequence_Manager_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

UZombiTurboSequenceProcessor::UZombiTurboSequenceProcessor()
{
    // Configuración correcta para procesadores Mass Entity en UE5.5.4
    ExecutionFlags = (int32)(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;

    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Constructor llamado - Procesador creado"));
}

// Configura los queries para requerir los fragmentos necesarios
void UZombiTurboSequenceProcessor::ConfigureQueries()
{
    // Query para entidades que necesitan instancias visuales creadas
    VisualInstanceQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadWrite);
    VisualInstanceQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);
    VisualInstanceQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);

    // Query para entidades que necesitan sincronización de transformación
    TransformSyncQuery.AddRequirement<FZombiTurboSequenceFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadOnly);
    TransformSyncQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);

    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Queries configurados correctamente (registro automático)"));
}

void UZombiTurboSequenceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesa entidades que necesitan sincronización de transformación y animación
    TransformSyncQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                          {
        const TConstArrayView<FZombiTurboSequenceFragment> TurboSequenceFragments = Context.GetFragmentView<FZombiTurboSequenceFragment>();
        const TConstArrayView<FZombiMovementFragment> MovementFragments = Context.GetFragmentView<FZombiMovementFragment>();
        const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            const FZombiTurboSequenceFragment& TurboSequenceFragment = TurboSequenceFragments[i];
            const FZombiMovementFragment& MovementFragment = MovementFragments[i];
            const FZombiStateFragment& StateFragment = StateFragments[i];

            // Actualiza transformación si la instancia visual es válida
            if (TurboSequenceFragment.bIsVisualInstanceValid)
            {
                UpdateTurboSequenceTransform(TurboSequenceFragment, MovementFragment);
                UpdateTurboSequenceAnimation(TurboSequenceFragment, StateFragment);
            }
        } });
}

void UZombiTurboSequenceProcessor::CreateTurboSequenceInstance(FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                               const FZombiMovementFragment &MovementFragment,
                                                               UWorld *World)
{
    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        UE_LOG(LogTemp, Warning, TEXT("TurboSequence Asset no asignado para crear instancia visual"));
        return;
    }

    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("World es null al crear instancia visual de TurboSequence"));
        return;
    }

    if (!World->IsValidLowLevel() || World->IsUnreachable())
    {
        UE_LOG(LogTemp, Error, TEXT("World no es válido al crear instancia visual de TurboSequence"));
        return;
    }

    // Crea la transformación de spawn
    FTransform SpawnTransform(MovementFragment.Rotation, MovementFragment.Position, FVector::OneVector);

    // Crea la instancia visual usando las mejores prácticas de TurboSequence
    FTurboSequence_MeshSpawnData_Lf SpawnData;
    SpawnData.RootMotionMesh.Mesh = TurboSequenceFragment.TurboSequenceAsset;

    // Verifica que el asset sea válido antes de crear la instancia
    if (!TurboSequenceFragment.TurboSequenceAsset->IsValidLowLevel())
    {
        UE_LOG(LogTemp, Error, TEXT("TurboSequence Asset no es válido"));
        return;
    }

    TurboSequenceFragment.MeshData = ATurboSequence_Manager_Lf::AddSkinnedMeshInstance_GameThread(
        SpawnData,
        SpawnTransform,
        World);

    // Agrega a un grupo de actualización para optimización
    if (TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        ATurboSequence_Manager_Lf::AddInstanceToUpdateGroup_Concurrent(
            TurboSequenceFragment.UpdateGroupIndex,
            TurboSequenceFragment.MeshData);

        TurboSequenceFragment.bIsVisualInstanceValid = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Error al crear instancia visual de TurboSequence"));
    }
}

void UZombiTurboSequenceProcessor::UpdateTurboSequenceTransform(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                                const FZombiMovementFragment &MovementFragment)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Actualiza la transformación de la instancia visual
    FTransform NewTransform(MovementFragment.Rotation, MovementFragment.Position, FVector::OneVector);

    ATurboSequence_Manager_Lf::SetMeshWorldSpaceTransform_Concurrent(
        TurboSequenceFragment.MeshData,
        NewTransform,
        false // No forzar actualización
    );
}

void UZombiTurboSequenceProcessor::DestroyTurboSequenceInstance(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                                UWorld *World)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Remueve del grupo de actualización
    ATurboSequence_Manager_Lf::RemoveInstanceFromUpdateGroup_Concurrent(
        TurboSequenceFragment.UpdateGroupIndex,
        TurboSequenceFragment.MeshData);

    // Destruye la instancia visual
    ATurboSequence_Manager_Lf::RemoveSkinnedMeshInstance_GameThread(
        TurboSequenceFragment.MeshData,
        World);
}

// Actualiza la animación de la instancia visual según el estado lógico
void UZombiTurboSequenceProcessor::UpdateTurboSequenceAnimation(const FZombiTurboSequenceFragment &TurboSequenceFragment,
                                                                const FZombiStateFragment &StateFragment)
{
    if (!TurboSequenceFragment.MeshData.IsMeshDataValid())
    {
        return;
    }

    // Validación adicional para evitar crashes
    if (!TurboSequenceFragment.TurboSequenceAsset)
    {
        return;
    }

    // Configuración de reproducción de animación usando la API correcta de TurboSequence
    FTurboSequence_AnimPlaySettings_Lf PlaySettings;
    PlaySettings.AnimationSpeed = 1.0f;
    PlaySettings.AnimationWeight = 1.0f;
    PlaySettings.AnimationPlayTimeInSeconds = 0.0f;
    PlaySettings.StartTransitionTimeInSeconds = 0.1f;
    PlaySettings.EndTransitionTimeInSeconds = 0.1f;
    PlaySettings.RootMotionMode = ETurboSequence_RootMotionMode_Lf::None;
    PlaySettings.AnimationManagementMode = ETurboSequence_ManagementMode_Lf::Auto;

    FString AnimationName;
    switch (StateFragment.State)
    {
    case EZombiState::Idle:
        AnimationName = TEXT("MM_Idle");
        break;
    case EZombiState::Walk:
        AnimationName = TEXT("MM_Walk_Fwd");
        break;
    case EZombiState::Chase:
        AnimationName = TEXT("MM_Walk_Fwd");
        break; // Usar Walk_Fwd para Chase por ahora
    case EZombiState::Attack:
        AnimationName = TEXT("MM_Land");
        break; // Usar Land para Attack por ahora
    case EZombiState::Hit:
        AnimationName = TEXT("MM_Land");
        break; // Usar Land para Hit por ahora
    case EZombiState::Death:
        AnimationName = TEXT("MM_Land");
        break; // Usar Land para Death por ahora
    default:
        AnimationName = TEXT("MM_Idle");
        break;
    }

    UAnimSequence *AnimationToPlay = FindAnimationByName(TurboSequenceFragment.TurboSequenceAsset, AnimationName);

    if (AnimationToPlay)
    {
        // Validación adicional antes de reproducir
        if (AnimationToPlay->GetPlayLength() > 0.0f)
        {
            // Usar la API correcta de TurboSequence
            try
            {
                // Usar PlayAnimation_Concurrent con la configuración correcta
                FTurboSequence_AnimMinimalCollection_Lf AnimationCollection =
                    ATurboSequence_Manager_Lf::PlayAnimation_Concurrent(
                        TurboSequenceFragment.MeshData,
                        AnimationToPlay,
                        PlaySettings);

                static float LogTimer = 0.0f;
                static float LastTime = 0.0f;
                float CurrentTime = FPlatformTime::Seconds();
                LogTimer += (CurrentTime - LastTime);
                LastTime = CurrentTime;

                if (LogTimer >= 10.0f)
                {
                    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Reproduciendo animación '%s' para estado %d (Collection válida: %s)"),
                           *AnimationName, (int32)StateFragment.State,
                           AnimationCollection.IsAnimCollectionValid() ? TEXT("Sí") : TEXT("No"));
                    LogTimer = 0.0f;
                }
            }
            catch (...)
            {
                UE_LOG(LogTemp, Error, TEXT("ZombiTurboSequenceProcessor: Error al reproducir animación '%s'"), *AnimationName);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiTurboSequenceProcessor: Animación '%s' tiene duración 0"), *AnimationName);
        }
    }
    else
    {
        static float LogTimer = 0.0f;
        static float LastTime = 0.0f;
        float CurrentTime = FPlatformTime::Seconds();
        LogTimer += (CurrentTime - LastTime);
        LastTime = CurrentTime;

        if (LogTimer >= 15.0f)
        {
            UE_LOG(LogTemp, Warning, TEXT("ZombiTurboSequenceProcessor: No se encontró animación '%s' para estado %d"),
                   *AnimationName, (int32)StateFragment.State);
            LogTimer = 0.0f;
        }
    }
}

// Busca una animación por nombre en el asset de TurboSequence
UAnimSequence *UZombiTurboSequenceProcessor::FindAnimationByName(UTurboSequence_MeshAsset_Lf *Asset, const FString &AnimationName)
{
    if (!Asset)
    {
        return nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Buscando animación '%s' en asset '%s'"),
           *AnimationName, *Asset->GetName());

    // Buscar en AnimationLibrary con validaciones estrictas
    if (Asset->AnimationLibrary && IsValid(Asset->AnimationLibrary))
    {
        // Validación adicional para evitar crash
        if (Asset->AnimationLibrary->Animations.Num() > 0)
        {
            for (const FAnimationLibraryItem_Lf &AnimItem : Asset->AnimationLibrary->Animations)
            {
                if (AnimItem.Animation && IsValid(AnimItem.Animation) && AnimItem.Animation->GetName().Contains(AnimationName))
                {
                    UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Encontrada animación '%s' en AnimationLibrary"),
                           *AnimationName);
                    return AnimItem.Animation;
                }
            }
        }
    }

    // Buscar en OverrideDefaultAnimation
    if (Asset->OverrideDefaultAnimation && IsValid(Asset->OverrideDefaultAnimation))
    {
        UE_LOG(LogTemp, Log, TEXT("ZombiTurboSequenceProcessor: Usando OverrideDefaultAnimation para '%s'"),
               *AnimationName);
        return Asset->OverrideDefaultAnimation;
    }

    // Si no encontramos la animación específica, usar la primera disponible como fallback
    if (Asset->AnimationLibrary && IsValid(Asset->AnimationLibrary) && Asset->AnimationLibrary->Animations.Num() > 0)
    {
        for (const FAnimationLibraryItem_Lf &AnimItem : Asset->AnimationLibrary->Animations)
        {
            if (AnimItem.Animation && IsValid(AnimItem.Animation))
            {
                UE_LOG(LogTemp, Warning, TEXT("ZombiTurboSequenceProcessor: Usando animación fallback '%s' para '%s'"),
                       *AnimItem.Animation->GetName(), *AnimationName);
                return AnimItem.Animation;
            }
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("ZombiTurboSequenceProcessor: No se encontró animación '%s' en asset '%s'"),
           *AnimationName, *Asset->GetName());

    return nullptr;
}
