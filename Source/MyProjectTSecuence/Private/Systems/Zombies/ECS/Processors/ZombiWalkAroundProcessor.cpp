// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiWalkAroundProcessor.h"
#include "MassExecutionContext.h"

UZombiWalkAroundProcessor::UZombiWalkAroundProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    // ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Behavior); // Removido - no existe en UE5.5
}

void UZombiWalkAroundProcessor::ConfigureQueries()
{
    // Query especializado para entidades caminando
    WalkAroundQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    WalkAroundQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    WalkAroundQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);

    // Tags para filtrado DOP
    WalkAroundQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    WalkAroundQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

    WalkAroundQuery.RegisterWithProcessor(*this);
}

void UZombiWalkAroundProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Solo procesar entidades que están en estado de walkaround
    WalkAroundQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                       {
        const int32 NumEntities = Context.GetNumEntities();
        auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
        auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
        auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();

        for (int32 i = 0; i < NumEntities; ++i)
        {
            FZombiStateFragment& StateFragment = StateFragments[i];
            FZombiTransformFragment& TransformFragment = TransformFragments[i];
            FZombiMovementFragment& MovementFragment = MovementFragments[i];

            // Solo procesar si realmente está caminando
            if (StateFragment.IsWalking())
            {
                ProcessWalkAroundLogic(StateFragment, TransformFragment, MovementFragment, DeltaTime);
            }
        } });
}

void UZombiWalkAroundProcessor::ProcessWalkAroundLogic(FZombiStateFragment &StateFragment,
                                                       FZombiTransformFragment &TransformFragment,
                                                       FZombiMovementFragment &MovementFragment,
                                                       float DeltaTime)
{
    // Verificar si debe cambiar de dirección
    if (ShouldChangeDirection(StateFragment))
    {
        // Generar nueva dirección aleatoria
        FVector NewDirection = GenerateRandomDirection();
        uint8 NewSpeed = CalculateWalkSpeed();

        MovementFragment.StartMoving(NewDirection, NewSpeed);

        // Resetear timer para el próximo cambio de dirección
        StateFragment.SetStateTimerSeconds(0.0f);

        // DEBUG: Log cambio de dirección
        static int32 DebugCounter = 0;
        if (++DebugCounter % 30 == 0) // Cada ~30 cambios
        {
            UE_LOG(LogTemp, Log, TEXT("🚶 WalkAroundProcessor: Nueva dirección - Dir: %s, Speed: %d"),
                   *NewDirection.ToString(), NewSpeed);
        }
    }

    // SIN colisiones, SIN límites de área - mantener simple
}

FVector UZombiWalkAroundProcessor::GenerateRandomDirection()
{
    // Generar dirección horizontal aleatoria (evitar movimiento vertical excesivo)
    float RandomAngle = FMath::RandRange(0.0f, 2.0f * PI);
    FVector RandomDirection = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);

    return RandomDirection.GetSafeNormal();
}

uint8 UZombiWalkAroundProcessor::CalculateWalkSpeed()
{
    // Velocidad aleatoria entre MIN y MAX
    return static_cast<uint8>(FMath::RandRange(MIN_WALK_SPEED, MAX_WALK_SPEED));
}

bool UZombiWalkAroundProcessor::ShouldChangeDirection(const FZombiStateFragment &StateFragment)
{
    float TimeSinceLastChange = StateFragment.GetStateTimerSeconds();
    float RandomChangeTime = FMath::RandRange(DIRECTION_CHANGE_MIN_TIME, DIRECTION_CHANGE_MAX_TIME);

    return TimeSinceLastChange >= RandomChangeTime;
}
