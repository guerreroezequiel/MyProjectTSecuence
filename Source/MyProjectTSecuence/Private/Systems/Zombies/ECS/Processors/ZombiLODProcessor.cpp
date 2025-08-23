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
            
            // 2. Calcular intensidad de estímulos
            float StimulusIntensity = CalculateStimulusIntensity(BehaviorFragment);
            LODFragment.SetStimulusIntensity(StimulusIntensity);
            
            // 3. Actualizar configuración de LOD
            UpdateLODSettings(LODFragment, BehaviorFragment, DistanceToPlayer);
            
            // 4. Aplicar frustum culling
            ApplyFrustumCulling(LODFragment, CoreFragment.Position);
            
            // 5. Sincronizar tags con estado
            FMassEntityHandle Entity = Context.GetEntity(EntityIndex);
            SynchronizeTagsWithState(EntityManager, Entity, BehaviorFragment);
            
            // 6. Actualizar tiempo de último update
            LODFragment.SetLastUpdateTime(CurrentTime);
        } });
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

    // 2. Determinar UpdateFrequency basado en prioridad y distancia
    bool bIsHighPriority = LODFragment.IsHighPriority();
    bool bIsMediumPriority = LODFragment.IsMediumPriority();

    if (bIsHighPriority || Distance < CriticalDistance)
    {
        // CRÍTICO: Attack, TakeDamage, o muy cercanos
        LODFragment.SetCriticalUpdate();
        LODFragment.SetFullDetail();
    }
    else if (bIsMediumPriority || Distance < HighDistance)
    {
        // ALTO: Chase, Seek, o medios con estímulos
        LODFragment.SetHighUpdate();
        LODFragment.SetReducedDetail();
    }
    else if (Distance < NormalDistance)
    {
        // NORMAL: WalkAround, o lejanos con estímulos
        LODFragment.SetNormalUpdate();
        LODFragment.SetSimpleDetail();
    }
    else
    {
        // BAJO: Idle lejanos sin estímulos
        LODFragment.SetLowUpdate();
        LODFragment.SetSpriteDetail();
    }

    // 3. Marcar para update si debe procesarse este frame
    bool bShouldProcess = LODFragment.ShouldProcessThisFrame(CurrentTime);
    LODFragment.SetNeedsUpdate(bShouldProcess);
}

void UZombiLODProcessor::SynchronizeTagsWithState(FMassEntityManager &EntityManager, FMassEntityHandle Entity, const FZombiBehaviorFragment &BehaviorFragment)
{
    // TODO: Implementar tag management usando FMassCommandBuffer
    // Por ahora, funcionalidad deshabilitada hasta encontrar la API correcta
    /*
    // Limpiar tags existentes primero
    EntityManager.RemoveTag<FChasingTag>(Entity);
    EntityManager.RemoveTag<FAttackingTag>(Entity);
    EntityManager.RemoveTag<FHighPriorityTag>(Entity);

    // Agregar tags según estado
    switch (BehaviorFragment.GetState())
    {
        case EZombiState::Dead:
            EntityManager.AddTag<FDeadTag>(Entity);
            EntityManager.RemoveTag<FActiveTag>(Entity);
            break;

        case EZombiState::Attack:
            EntityManager.AddTag<FAttackingTag>(Entity);
            EntityManager.AddTag<FHighPriorityTag>(Entity);
            break;

        case EZombiState::TakeDamage:
            EntityManager.AddTag<FHighPriorityTag>(Entity);
            break;

        case EZombiState::Chase:
            EntityManager.AddTag<FChasingTag>(Entity);
            EntityManager.AddTag<FHighPriorityTag>(Entity);
            break;

        case EZombiState::Seek:
            EntityManager.AddTag<FChasingTag>(Entity);
            break;

        case EZombiState::WalkAround:
        case EZombiState::Idle:
            // Estados básicos, solo FActiveTag
            break;
    }
    */
}

void UZombiLODProcessor::ApplyFrustumCulling(FZombiLODFragment &LODFragment, const FVector &ZombieLocation)
{
    bool bInFrustum = IsInFrustum(ZombieLocation);
    LODFragment.SetInFrustum(bInFrustum);

    // Si no está en frustum, reducir LOD visual
    if (!bInFrustum)
    {
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

float UZombiLODProcessor::CalculateStimulusIntensity(const FZombiBehaviorFragment &BehaviorFragment) const
{
    // Calcular intensidad de estímulos basada en estado y datos
    float Intensity = 0.0f;

    switch (BehaviorFragment.GetState())
    {
    case EZombiState::Attack:
        Intensity = 100.0f; // Máxima intensidad
        break;
    case EZombiState::TakeDamage:
        Intensity = 95.0f; // Alta intensidad
        break;
    case EZombiState::Chase:
        Intensity = 80.0f; // Intensidad alta
        break;
    case EZombiState::Seek:
        Intensity = 60.0f; // Intensidad media
        break;
    case EZombiState::WalkAround:
        Intensity = 30.0f; // Intensidad baja
        break;
    case EZombiState::Idle:
        Intensity = 10.0f; // Intensidad mínima
        break;
    case EZombiState::Dead:
        Intensity = 0.0f; // Sin intensidad
        break;
    }

    // Ajustar por datos específicos del estado
    if (BehaviorFragment.HasAnyAction())
    {
        Intensity += 20.0f; // Bonus por acciones
    }

    return FMath::Clamp(Intensity, 0.0f, 100.0f);
}
