// Copyright 2024 MyProjectTSec

#include "ECS/Processors/MoveIntegrateProcessor.h"
#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "MassCommonFragments.h"
#include "MassMovementFragments.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/MoveFragment.h"
#include "ECS/Fragments/ZombiCoreFragment.h"
#include "ECS/Tags/ZombiTag.h"

UMoveIntegrateProcessor::UMoveIntegrateProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UMoveIntegrateProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FZombiCoreFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FFlowReadFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FMoveFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
    EntityQuery.RegisterWithProcessor(*this);
}

void UMoveIntegrateProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();
    
    EntityQuery.ForEachEntityChunk(EntityManager, Context, [DeltaTime](FMassExecutionContext& Context)
    {
        const TArrayView<FTransformFragment> TransformFragments = Context.GetMutableFragmentView<FTransformFragment>();
        const TArrayView<FZombiCoreFragment> CoreFragments = Context.GetMutableFragmentView<FZombiCoreFragment>();
        const TConstArrayView<FFlowReadFragment> FlowReadFragments = Context.GetFragmentView<FFlowReadFragment>();
        const TConstArrayView<FMoveFragment> MoveFragments = Context.GetFragmentView<FMoveFragment>();
        
        const int32 NumEntities = Context.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i)
        {
            FTransformFragment& Transform = TransformFragments[i];
            FZombiCoreFragment& Core = CoreFragments[i];
            const FFlowReadFragment& FlowRead = FlowReadFragments[i];
            const FMoveFragment& Move = MoveFragments[i];
            
            if (FlowRead.bValid)
            {
                // Calcular desplazamiento basado en la dirección y velocidad
                const FVector Displacement = FlowRead.DirWS * Move.Speed * DeltaTime;
                
                // Actualizar posición en el transform
                FTransform NewTransform = Transform.GetTransform();
                NewTransform.AddToTranslation(Displacement);
                
                // Actualizar rotación para que mire en la dirección del movimiento
                if (!FlowRead.DirWS.IsNearlyZero())
                {
                    const FRotator NewRotation = FlowRead.DirWS.Rotation();
                    NewTransform.SetRotation(NewRotation.Quaternion());
                }
                
                Transform.SetTransform(NewTransform);
            }
        }
    });
}
