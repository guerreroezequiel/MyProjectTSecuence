// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiTransformProcessor.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"

UZombiTransformProcessor::UZombiTransformProcessor()
{
    // Configurar el procesador para ejecutarse después del comportamiento
    ExecutionOrder.ExecuteAfter.Add(TEXT("MassBehavior"));
    ExecutionOrder.ExecuteBefore.Add(TEXT("MassMovement"));
}

void UZombiTransformProcessor::ConfigureQueries()
{
    // Query principal para todas las entidades con transformación
    TransformQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    TransformQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    TransformQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
    TransformQuery.RegisterWithProcessor(*this);
}

void UZombiTransformProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Procesar todas las entidades activas
    TransformQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                                      {
        const int32 NumEntities = Context.GetNumEntities();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            
            // Validar y corregir transformaciones inválidas
            if (!ValidateTransform(TransformFragment))
            {
                FixInvalidTransform(TransformFragment);
            }
        } });
}

bool UZombiTransformProcessor::ValidateTransform(const FZombiTransformFragment &TransformFragment) const
{
    if (TransformFragment.GetPosition().ContainsNaN())
    {
        return false;
    }

    float Yaw = TransformFragment.GetYaw();
    if (FMath::IsNaN(Yaw) || !FMath::IsFinite(Yaw))
    {
        return false;
    }

    if (TransformFragment.GetPosition().Size() > 10000.0f)
    {
        return false;
    }

    return true;
}

void UZombiTransformProcessor::FixInvalidTransform(FZombiTransformFragment &TransformFragment) const
{
    if (TransformFragment.GetPosition().ContainsNaN())
    {
        TransformFragment.SetPosition(FVector::ZeroVector);
    }

    float Yaw = TransformFragment.GetYaw();
    if (FMath::IsNaN(Yaw) || !FMath::IsFinite(Yaw))
    {
        TransformFragment.SetYaw(0.0f);
    }

    if (TransformFragment.GetPosition().Size() > 10000.0f)
    {
        TransformFragment.SetPosition(FVector::ZeroVector);
    }
}