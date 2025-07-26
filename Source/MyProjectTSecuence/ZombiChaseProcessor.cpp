// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiChaseProcessor.h"
#include "ZombiTransformFragment.h"
#include "ZombiVelocityFragment.h"
#include "MassExecutionContext.h"
#include "Engine/Engine.h"
#include "MyProjectTSecuenceCharacter.h"
#include "Kismet/GameplayStatics.h"

// Procesador especializado para persecución al jugador
// OPTIMIZADO para cache locality y paralelización
UZombiChaseProcessor::UZombiChaseProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    ExecutionOrder.ExecuteAfter.Add(TEXT("ZombiTransformProcessor"));
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;
}

void UZombiChaseProcessor::ConfigureQueries()
{
    // Query para entidades que pueden perseguir (todas las activas)
    ChaseQuery.AddRequirement<FZombiChaseFragment>(EMassFragmentAccess::ReadWrite);
    ChaseQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    ChaseQuery.AddRequirement<FZombiVelocityFragment>(EMassFragmentAccess::ReadWrite);
    ChaseQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    ChaseQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    ChaseQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    // Query para entidades que están persiguiendo
    ChasingQuery.AddRequirement<FZombiChaseFragment>(EMassFragmentAccess::ReadWrite);
    ChasingQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    ChasingQuery.AddRequirement<FZombiVelocityFragment>(EMassFragmentAccess::ReadWrite);
    ChasingQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    ChasingQuery.AddTagRequirement<FChasingTag>(EMassFragmentPresence::All);
    ChasingQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    ChasingQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

void UZombiChaseProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Obtener referencia al jugador (solo una vez por frame)
    if (!PlayerCharacter)
    {
        PlayerCharacter = Cast<AMyProjectTSecuenceCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    }

    // Actualizar posición del jugador
    if (PlayerCharacter)
    {
        CachedPlayerPosition = PlayerCharacter->GetActorLocation();
    }

    // Actualizar timer global de persecución
    GlobalChaseTimer += DeltaTime;
    const float ChaseCycleInterval = 10.0f; // 10 segundos entre persecuciones

    // Verificar si debe iniciar persecución global
    if (GlobalChaseTimer >= ChaseCycleInterval && !bGlobalChaseActive)
    {
        bGlobalChaseActive = true;
        GlobalChaseTimer = 0.0f;

        // Log de persecución iniciada
        UE_LOG(LogTemp, Warning, TEXT("🎮 PERSECUCIÓN INICIADA - Todos los zombis persiguen al jugador por 5 segundos!"));
    }

    // Verificar si debe terminar persecución global
    if (bGlobalChaseActive && GlobalChaseTimer >= 5.0f) // 5 segundos de persecución
    {
        bGlobalChaseActive = false;
        GlobalChaseTimer = 0.0f;

        // Log de persecución terminada
        UE_LOG(LogTemp, Warning, TEXT("🎮 PERSECUCIÓN TERMINADA - Los zombis vuelven a comportamiento normal"));
    }

    // Procesar entidades que están persiguiendo
    if (bGlobalChaseActive)
    {
        ChasingQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime, &EntityManager](FMassExecutionContext &Context)
                                        {
			const TArrayView<FZombiChaseFragment> ChaseFragments = Context.GetMutableFragmentView<FZombiChaseFragment>();
			const TConstArrayView<FZombiTransformFragment> TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
			const TArrayView<FZombiVelocityFragment> VelocityFragments = Context.GetMutableFragmentView<FZombiVelocityFragment>();
			const TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();

			const int32 NumEntities = Context.GetNumEntities();

			for (int32 i = 0; i < NumEntities; ++i)
			{
				FZombiChaseFragment& ChaseFragment = ChaseFragments[i];
				const FZombiTransformFragment& TransformFragment = TransformFragments[i];
				FZombiVelocityFragment& VelocityFragment = VelocityFragments[i];
				FZombiStateFragment& StateFragment = StateFragments[i];

				// Actualizar posición del jugador en el fragmento
				ChaseFragment.PlayerTargetPosition = CachedPlayerPosition;

				// Calcular dirección hacia el jugador
				FVector DirectionToPlayer = (CachedPlayerPosition - TransformFragment.Position).GetSafeNormal();

				// Verificar si el jugador está dentro del rango de persecución
				float DistanceToPlayer = FVector::Dist(TransformFragment.Position, CachedPlayerPosition);
				
				if (DistanceToPlayer <= ChaseFragment.ChaseDistance)
				{
					// Configurar persecución
					VelocityFragment.MovementDirection = DirectionToPlayer;
					VelocityFragment.MovementSpeed = ChaseFragment.ChaseSpeed;
					StateFragment.State = EZombiState::Chase;

					// Marcar como persiguiendo
					ChaseFragment.bIsChasing = true;
					
					// Agregar tag de persecución
					EntityManager.AddTagToEntity(Context.GetEntity(i), FChasingTag::StaticStruct());
				}
				else
				{
					// Jugador fuera de rango, volver a comportamiento normal
					ChaseFragment.bIsChasing = false;
					StateFragment.State = EZombiState::Walk;
					
					// Quitar tag de persecución
					EntityManager.RemoveTagFromEntity(Context.GetEntity(i), FChasingTag::StaticStruct());
				}
			} });
    }
    else
    {
        // Procesar entidades para preparar persecución futura
        ChaseQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime, &EntityManager](FMassExecutionContext &Context)
                                      {
			const TArrayView<FZombiChaseFragment> ChaseFragments = Context.GetMutableFragmentView<FZombiChaseFragment>();
			const TArrayView<FZombiStateFragment> StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();

			const int32 NumEntities = Context.GetNumEntities();

			for (int32 i = 0; i < NumEntities; ++i)
			{
				FZombiChaseFragment& ChaseFragment = ChaseFragments[i];
				FZombiStateFragment& StateFragment = StateFragments[i];

				// Si estaba persiguiendo, volver a comportamiento normal
				if (ChaseFragment.bIsChasing)
				{
					ChaseFragment.bIsChasing = false;
					StateFragment.State = EZombiState::Walk;
					
					// Quitar tag de persecución
					EntityManager.RemoveTagFromEntity(Context.GetEntity(i), FChasingTag::StaticStruct());
				}
			} });
    }
}