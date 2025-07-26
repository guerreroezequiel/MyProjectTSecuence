// Fill out your copyright notice in the Description page of Project Settings.

#include "ZombiTransformProcessor.h"
#include "MassExecutionContext.h"
#include "Engine/Engine.h"

// Procesador especializado solo para transformaciones
// OPTIMIZADO para cache locality y paralelización
UZombiTransformProcessor::UZombiTransformProcessor()
{
    // Configuración para registro automático en UE5.5.4
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
    bRequiresGameThreadExecution = false;
    bAutoRegisterWithProcessingPhases = true;
}

void UZombiTransformProcessor::ConfigureQueries()
{
    // Query para entidades activas que necesitan actualización de transformación
    TransformQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    TransformQuery.AddRequirement<FZombiVelocityFragment>(EMassFragmentAccess::ReadOnly);
    TransformQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    TransformQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

void UZombiTransformProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Procesar transformaciones en paralelo
    TransformQuery.ForEachEntityChunk(EntityManager, Context, [DeltaTime](FMassExecutionContext &Context)
                                      {
		const TArrayView<FZombiTransformFragment> TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
		const TConstArrayView<FZombiVelocityFragment> VelocityFragments = Context.GetFragmentView<FZombiVelocityFragment>();

		const int32 NumEntities = Context.GetNumEntities();

		for (int32 i = 0; i < NumEntities; ++i)
		{
			FZombiTransformFragment& TransformFragment = TransformFragments[i];
			const FZombiVelocityFragment& VelocityFragment = VelocityFragments[i];

			// OPTIMIZACIÓN: Solo calcular transformación si hay movimiento
			if (VelocityFragment.MovementSpeed > 0.0f && !VelocityFragment.MovementDirection.IsNearlyZero())
			{
				// Calcular forward direction desde la rotación
				FVector ForwardDirection = TransformFragment.Rotation.Vector();
				
				// Aplicar movimiento
				FVector NewPosition = TransformFragment.Position + 
					ForwardDirection * VelocityFragment.MovementSpeed * DeltaTime;

				// Actualizar posición
				TransformFragment.Position = NewPosition;
			}
		} });
}