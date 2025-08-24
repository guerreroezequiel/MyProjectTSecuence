// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiLODProcessor.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"

UZombiLODProcessor::UZombiLODProcessor()
{
    // Configurar procesador para ejecutarse cada frame
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassLOD");        // Grupo propio
    ExecutionOrder.ExecuteBefore.Add(TEXT("MassBehavior")); // ANTES que Behavior
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;
}

void UZombiLODProcessor::ConfigureQueries()
{
    // Query para entidades con LOD - Solo entidades activas
    LODQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    LODQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Fragmentos requeridos
    LODQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadOnly);
    LODQuery.AddRequirement<FZombiBehaviorFragment>(EMassFragmentAccess::ReadOnly);
    LODQuery.AddRequirement<FZombiLODFragment>(EMassFragmentAccess::ReadWrite);

    // Registrar query
    LODQuery.RegisterWithProcessor(*this);
}

void UZombiLODProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Obtener tiempo actual
    CurrentTime = GetWorld()->GetTimeSeconds();

    // Obtener referencia al jugador (cached para performance)
    if (!PlayerPawn)
    {
        PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    }

    if (!PlayerPawn)
    {
        return; // No hay jugador, no procesar
    }

    // Procesar entidades en chunks para optimización
    LODQuery.ForEachEntityChunk(EntityManager, Context, [this, &EntityManager](FMassExecutionContext &Context)
                                {
        // Obtener arrays de fragmentos
        const TArrayView<const FZombiCoreFragment> CoreFragments = Context.GetFragmentView<FZombiCoreFragment>();
        const TArrayView<const FZombiBehaviorFragment> BehaviorFragments = Context.GetFragmentView<FZombiBehaviorFragment>();
        TArrayView<FZombiLODFragment> LODFragments = Context.GetMutableFragmentView<FZombiLODFragment>();
        
        // Procesar cada entidad en el chunk
        for (int32 EntityIndex = 0; EntityIndex < Context.GetNumEntities(); ++EntityIndex)
        {
            const FZombiCoreFragment& CoreFragment = CoreFragments[EntityIndex];
            const FZombiBehaviorFragment& BehaviorFragment = BehaviorFragments[EntityIndex];
            FZombiLODFragment& LODFragment = LODFragments[EntityIndex];
            
            // 1. Calcular distancia al jugador
            float DistanceToPlayer = 0.0f;
            CalculateDistanceToPlayer(CoreFragment.Position, DistanceToPlayer);
            LODFragment.SetDistanceToPlayer(DistanceToPlayer);
            
            // 2. Actualizar configuración de LOD
            UpdateLODSettings(LODFragment, BehaviorFragment, DistanceToPlayer);
            
            // 3. Obtener entidad para sincronización
            FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
            
            // 4. Sincronizar estado → tags de frecuencia
            SynchronizeStateToFrequency(EntityManager, Entity, BehaviorFragment);
            
            // 5. Aplicar frustum culling
            ApplyFrustumCulling(EntityManager, Entity, LODFragment, CoreFragment.Position);
        } });

    // Ejecutar todos los comandos diferidos DESPUÉS de la iteración
    ExecuteDeferredCommands(EntityManager);
}

void UZombiLODProcessor::CalculateDistanceToPlayer(const FVector &ZombieLocation, float &OutDistance)
{
    if (!PlayerPawn)
    {
        OutDistance = 0.0f;
        return;
    }

    // Calcular distancia 2D (isométrico)
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector DistanceVector = ZombieLocation - PlayerLocation;
    DistanceVector.Z = 0.0f; // Ignorar altura para isométrico
    OutDistance = DistanceVector.Size();
}

void UZombiLODProcessor::UpdateLODSettings(FZombiLODFragment &LODFragment, const FZombiBehaviorFragment &BehaviorFragment, float Distance)
{
    // 1. Actualizar PriorityLevel basado en estado
    switch (BehaviorFragment.GetState())
    {
    case EZombiState::Attack:
        LODFragment.SetAttackPriority();
        break;
    case EZombiState::TakeDamage:
        LODFragment.SetTakeDamagePriority();
        break;
    case EZombiState::Chase:
        LODFragment.SetChasePriority();
        break;
    case EZombiState::Seek:
        LODFragment.SetSeekPriority();
        break;
    case EZombiState::WalkAround:
        LODFragment.SetWalkAroundPriority();
        break;
    case EZombiState::Idle:
        LODFragment.SetIdlePriority();
        break;
    case EZombiState::Dead:
        LODFragment.SetDeadPriority();
        break;
    }

    // 2. Determinar LOD visual basado en distancia
    if (Distance < CriticalDistance)
    {
        LODFragment.SetFullDetail();
    }
    else if (Distance < HighDistance)
    {
        LODFragment.SetReducedDetail();
    }
    else if (Distance < NormalDistance)
    {
        LODFragment.SetSimpleDetail();
    }
    else
    {
        LODFragment.SetSpriteDetail();
    }
}

void UZombiLODProcessor::SynchronizeStateToFrequency(FMassEntityManager &EntityManager, FMassEntityHandle Entity, const FZombiBehaviorFragment &BehaviorFragment)
{
    // Limpiar todos los tags de frecuencia usando comandos diferidos
    AddDeferredTagCommand(Entity, FUpdate60FPS::StaticStruct(), false);
    AddDeferredTagCommand(Entity, FUpdate30FPS::StaticStruct(), false);
    AddDeferredTagCommand(Entity, FUpdate15FPS::StaticStruct(), false);
    AddDeferredTagCommand(Entity, FUpdate5FPS::StaticStruct(), false);

    // Agregar tag según estado usando comandos diferidos
    switch (BehaviorFragment.GetState())
    {
    case EZombiState::Dead:
        AddDeferredTagCommand(Entity, FDeadTag::StaticStruct(), true);
        AddDeferredTagCommand(Entity, FUpdate5FPS::StaticStruct(), true);
        break;

    case EZombiState::Attack:
        AddDeferredTagCommand(Entity, FUpdate60FPS::StaticStruct(), true);
        break;

    case EZombiState::TakeDamage:
        AddDeferredTagCommand(Entity, FUpdate60FPS::StaticStruct(), true);
        break;

    case EZombiState::Chase:
        AddDeferredTagCommand(Entity, FUpdate30FPS::StaticStruct(), true);
        break;

    case EZombiState::Seek:
        AddDeferredTagCommand(Entity, FUpdate15FPS::StaticStruct(), true);
        break;

    case EZombiState::WalkAround:
        AddDeferredTagCommand(Entity, FUpdate15FPS::StaticStruct(), true);
        break;

    case EZombiState::Idle:
        AddDeferredTagCommand(Entity, FUpdate5FPS::StaticStruct(), true);
        break;
    }
}

void UZombiLODProcessor::ApplyFrustumCulling(FMassEntityManager &EntityManager, FMassEntityHandle Entity, FZombiLODFragment &LODFragment, const FVector &ZombieLocation)
{
    bool bInFrustum = IsInFrustum(ZombieLocation);

    // Aplicar FInFrustumTag usando comandos diferidos
    if (bInFrustum)
    {
        AddDeferredTagCommand(Entity, FInFrustumTag::StaticStruct(), true);
    }
    else
    {
        AddDeferredTagCommand(Entity, FInFrustumTag::StaticStruct(), false);
        // Si no está en frustum, reducir LOD visual
        LODFragment.SetSpriteDetail();
    }
}

void UZombiLODProcessor::SetZombieState(FMassEntityManager &EntityManager, FMassEntityHandle Entity, EZombiState NewState)
{
    // Este método será usado por otros procesadores para cambios de estado
    // La sincronización de tags se hace automáticamente en SynchronizeTagsWithState
}

bool UZombiLODProcessor::IsInFrustum(const FVector &Location) const
{
    // Implementación básica de frustum culling para isométrico
    // En una implementación completa, esto calcularía el frustum real de la cámara

    if (!PlayerPawn)
    {
        return true; // Si no hay jugador, asumir visible
    }

    // Culling simple basado en distancia máxima
    FVector PlayerLocation = PlayerPawn->GetActorLocation();
    FVector DistanceVector = Location - PlayerLocation;
    DistanceVector.Z = 0.0f;

    float Distance = DistanceVector.Size();
    float MaxViewDistance = 2000.0f; // Distancia máxima de vista

    return Distance <= MaxViewDistance;
}

void UZombiLODProcessor::AddDeferredTagCommand(FMassEntityHandle Entity, const UScriptStruct *TagType, bool bAddTag)
{
    // Agregar comando al array de comandos diferidos
    DeferredCommands.Emplace(Entity, TagType, bAddTag);
}

void UZombiLODProcessor::ExecuteDeferredCommands(FMassEntityManager &EntityManager)
{
    // Ejecutar todos los comandos diferidos
    for (const FDeferredTagCommand &Command : DeferredCommands)
    {
        if (Command.bAddTag)
        {
            EntityManager.AddTagToEntity(Command.Entity, Command.TagType);
        }
        else
        {
            EntityManager.RemoveTagFromEntity(Command.Entity, Command.TagType);
        }
    }

    // Limpiar array de comandos
    DeferredCommands.Reset();
}
