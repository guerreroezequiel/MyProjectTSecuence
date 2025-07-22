// Fill out your copyright notice in the Description page of Project Settings.
#include "MassExecutionContext.h"
#include "ZombiStateSyncProcessor.h"
#include "FMassActorFragment.h"
#include "MyTurboSequenceAnimComponent.h"

// Constructor: define el grupo de ejecución para que se registre automáticamente
UZombiStateSyncProcessor::UZombiStateSyncProcessor()
{
    // Se ejecuta después de que Mass Entity actualice los fragmentos
    ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::SyncWorldToMass;
}

// Configura el query para requerir el fragmento de estado y el actor asociado
void UZombiStateSyncProcessor::ConfigureQueries()
{
    Query.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadOnly);
    Query.AddRequirement<FZombiActorFragment>(EMassFragmentAccess::ReadOnly);
}

// Ejecuta la sincronización de estado a animación
void UZombiStateSyncProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    Query.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext &Context)
                             {
        const TConstArrayView<FZombiStateFragment> StateFragments = Context.GetFragmentView<FZombiStateFragment>();
        const TConstArrayView<FZombiActorFragment> ActorFragments = Context.GetFragmentView<FZombiActorFragment>();

        for (int32 i = 0; i < Context.GetNumEntities(); ++i)
        {
            const FZombiStateFragment& State = StateFragments[i];
            const FZombiActorFragment& ActorFrag = ActorFragments[i];

            if (AActor* Actor = ActorFrag.Get())
            {
                if (UMyTurboSequenceAnimComponent* AnimComp = Actor->FindComponentByClass<UMyTurboSequenceAnimComponent>())
                {
                    // Sincroniza el estado lógico con la animación visual
                    AnimComp->SetState(State.State);
                }
            }
        } });
}