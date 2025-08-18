// Fill out your copyright notice in the Description page of Project Settings.

#include "Systems/Zombies/ECS/Processors/ZombiBasicBehaviorProcessor.h"
#include "MassExecutionContext.h"

UZombiBasicBehaviorProcessor::UZombiBasicBehaviorProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ExecutionFlags = static_cast<int32>(EProcessorExecutionFlags::All);
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionOrder.ExecuteInGroup = TEXT("MassBasicBehavior");
    ExecutionOrder.ExecuteAfter.Add(TEXT("MassBehavior")); // Después del BehaviorProcessor principal
}

void UZombiBasicBehaviorProcessor::ConfigureQueries()
{
    UE_LOG(LogTemp, Log, TEXT("🧠 BasicBehaviorProcessor: ConfigureQueries llamado"));

    // Query para entidades en estado Idle
    IdleBehaviorQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    IdleBehaviorQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadWrite);
    IdleBehaviorQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    IdleBehaviorQuery.AddTagRequirement<FIdleTag>(EMassFragmentPresence::All);
    IdleBehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    IdleBehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
    IdleBehaviorQuery.RegisterWithProcessor(*this);

    // Query para entidades en estado WalkAround
    WalkAroundBehaviorQuery.AddRequirement<FZombiStateFragment>(EMassFragmentAccess::ReadWrite);
    WalkAroundBehaviorQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    WalkAroundBehaviorQuery.AddRequirement<FZombiMovementFragment>(EMassFragmentAccess::ReadWrite);
    WalkAroundBehaviorQuery.AddTagRequirement<FWalkingTag>(EMassFragmentPresence::All);
    WalkAroundBehaviorQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    WalkAroundBehaviorQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
    WalkAroundBehaviorQuery.RegisterWithProcessor(*this);

    UE_LOG(LogTemp, Log, TEXT("🧠 BasicBehaviorProcessor: Queries configuradas - Idle y WalkAround"));
}

void UZombiBasicBehaviorProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Solo ejecutar durante el juego (PIE), no en el editor
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !GetWorld()->HasBegunPlay())
    {
        return;
    }

    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Debug: Verificar ejecución
    static int32 ExecuteCounter = 0;
    if (++ExecuteCounter % 600 == 0) // Cada 10 segundos aprox
    {
        UE_LOG(LogTemp, Log, TEXT("🧠 BasicBehaviorProcessor: EJECUTÁNDOSE | Execute count: %d"), ExecuteCounter);
    }

    // Procesar entidades Idle (baja frecuencia - cada 4 frames)
    static int32 IdleFrameCounter = 0;
    if (++IdleFrameCounter % 4 == 0)
    {
        IdleBehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                             {
            const int32 NumEntities = Context.GetNumEntities();
            if (NumEntities > 0)
            {
                static int32 IdleChunkCounter = 0;
                if (++IdleChunkCounter % 150 == 0) // Debug cada 2.5 segundos aprox
                {
                    UE_LOG(LogTemp, Log, TEXT("🧠 BasicBehavior IdleChunk: %d entidades"), NumEntities);
                }
            }

            auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
            auto TransformFragments = Context.GetMutableFragmentView<FZombiTransformFragment>();
            auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();

            for (int32 i = 0; i < NumEntities; ++i)
            {
                ProcessIdleBehavior(StateFragments[i], TransformFragments[i], MovementFragments[i], DeltaTime * 4);
            } });
    }

    // Procesar entidades WalkAround (frecuencia media - cada 2 frames)
    static int32 WalkFrameCounter = 0;
    if (++WalkFrameCounter % 2 == 0)
    {
        WalkAroundBehaviorQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &Context)
                                                   {
            const int32 NumEntities = Context.GetNumEntities();
            if (NumEntities > 0)
            {
                static int32 WalkChunkCounter = 0;
                if (++WalkChunkCounter % 300 == 0) // Debug cada 5 segundos aprox
                {
                    UE_LOG(LogTemp, Log, TEXT("🧠 BasicBehavior WalkChunk: %d entidades"), NumEntities);
                }
            }

            auto StateFragments = Context.GetMutableFragmentView<FZombiStateFragment>();
            auto TransformFragments = Context.GetFragmentView<FZombiTransformFragment>();
            auto MovementFragments = Context.GetMutableFragmentView<FZombiMovementFragment>();

            for (int32 i = 0; i < NumEntities; ++i)
            {
                ProcessWalkAroundBehavior(StateFragments[i], TransformFragments[i], MovementFragments[i], DeltaTime * 2);
            } });
    }
}

void UZombiBasicBehaviorProcessor::ProcessIdleBehavior(FZombiStateFragment &StateFragment,
                                                       FZombiTransformFragment &TransformFragment,
                                                       FZombiMovementFragment &MovementFragment,
                                                       float DeltaTime)
{
    // SIMPLE: Solo asegurar que esté parado
    if (MovementFragment.GetSpeed() > 0)
    {
        MovementFragment.Stop();
    }

    // SIMPLE: Solo rotar un poco hacia los costados cada tanto
    if (ShouldPlayIdleAnimation(StateFragment))
    {
        // Rotar un poco hacia la izquierda o derecha
        float RandomRotation = FMath::RandRange(-30.0f, 30.0f); // ±30 grados
        float CurrentYaw = TransformFragment.GetYaw();
        float NewYaw = CurrentYaw + RandomRotation;
        TransformFragment.SetYaw(NewYaw);

        // Resetear timer
        StateFragment.SetStateTimerSeconds(0.0f);
    }
}

void UZombiBasicBehaviorProcessor::ProcessWalkAroundBehavior(FZombiStateFragment &StateFragment,
                                                             const FZombiTransformFragment &TransformFragment,
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
    }
}

FVector UZombiBasicBehaviorProcessor::GenerateRandomDirection()
{
    // Generar dirección horizontal aleatoria (evitar movimiento vertical excesivo)
    float RandomAngle = FMath::RandRange(0.0f, 2.0f * PI);
    FVector RandomDirection = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);
    return RandomDirection.GetSafeNormal();
}

uint8 UZombiBasicBehaviorProcessor::CalculateWalkSpeed()
{
    // Velocidad aleatoria entre MIN y MAX
    return static_cast<uint8>(FMath::RandRange(MIN_WALK_SPEED, MAX_WALK_SPEED));
}

bool UZombiBasicBehaviorProcessor::ShouldChangeDirection(const FZombiStateFragment &StateFragment)
{
    float TimeSinceLastChange = StateFragment.GetStateTimerSeconds();
    float RandomChangeTime = FMath::RandRange(DIRECTION_CHANGE_MIN_TIME, DIRECTION_CHANGE_MAX_TIME);
    return TimeSinceLastChange >= RandomChangeTime;
}

bool UZombiBasicBehaviorProcessor::ShouldPlayIdleAnimation(const FZombiStateFragment &StateFragment)
{
    float TimeSinceLastAnimation = StateFragment.GetStateTimerSeconds();
    float RandomAnimationTime = FMath::RandRange(IDLE_ANIMATION_MIN_TIME, IDLE_ANIMATION_MAX_TIME);
    return TimeSinceLastAnimation >= RandomAnimationTime;
}
