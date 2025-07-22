// Fill out your copyright notice in the Description page of Project Settings.
#include "ZombiStateSyncProcessor.h"
#include "MassExecutionContext.h"
#include "FMassActorFragment.h"
#include "MyTurboSequenceAnimComponent.h"

// Constructor: sin grupo de ejecución específico
UZombiStateSyncProcessor::UZombiStateSyncProcessor()
{
    // Se ejecutará en el grupo por defecto automáticamente
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