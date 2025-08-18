#include "Systems/Zombies/ECS/Processors/ZombiStimulusProcessor.h"
#include "Systems/StimulusSubsystem/StimulusSubsystem.h"
#include "Systems/Zombies/ECS/Fragments/ZombiTransformFragment.h"
#include "Systems/Zombies/ECS/Fragments/ZombiStimuliFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "Engine/Engine.h"

UZombiStimulusProcessor::UZombiStimulusProcessor()
{
    // Configuración del processor
    StimulusDetectionRange = 1000.0f; // Rango de detección de estímulos
    StimulusUpdateInterval = 0.1f;    // Intervalo de actualización de estímulos

    // Configurar orden de ejecución (CRÍTICO)
    ExecutionOrder.ExecuteBefore.Add(TEXT("MassBehavior"));

    // Ejecutar en PrePhysics para que BehaviorProcessor pueda usar los estímulos
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UZombiStimulusProcessor::ConfigureQueries()
{
    // Query para zombis vivos que pueden recibir estímulos
    StimulusQuery.RegisterWithProcessor(*this);
    StimulusQuery.AddRequirement<FZombiTransformFragment>(EMassFragmentAccess::ReadOnly);
    StimulusQuery.AddRequirement<FZombiStimuliFragment>(EMassFragmentAccess::ReadWrite);
    StimulusQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
    StimulusQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);
}

void UZombiStimulusProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context)
{
    // Obtener delta time
    const float DeltaTime = Context.GetDeltaTimeSeconds();

    // Actualizar cache de estímulos
    UpdateStimulusCache(DeltaTime);

    // Procesar estímulos para cada zombie
    StimulusQuery.ForEachEntityChunk(EntityManager, Context, [this, DeltaTime](FMassExecutionContext &ChunkContext)
                                     {
        // Obtener arrays de fragmentos
        const TConstArrayView<FZombiTransformFragment> TransformFragments = ChunkContext.GetFragmentView<FZombiTransformFragment>();
        TArrayView<FZombiStimuliFragment> StimuliFragments = ChunkContext.GetMutableFragmentView<FZombiStimuliFragment>();
        
        // Procesar cada zombie en el chunk
        for (int32 EntityIndex = 0; EntityIndex < ChunkContext.GetNumEntities(); ++EntityIndex)
        {
            const FZombiTransformFragment& TransformFragment = TransformFragments[EntityIndex];
            FZombiStimuliFragment& StimuliFragment = StimuliFragments[EntityIndex];
            
            // Actualizar timers de respuesta
            UpdateResponseTimers(StimuliFragment, DeltaTime);
            
            // Procesar estímulos para este zombie
            ProcessStimuliForZombie(TransformFragment.GetPosition(), StimuliFragment);
        } });
}

void UZombiStimulusProcessor::UpdateStimulusCache(float DeltaTime)
{
    // Actualizar cache periódicamente
    CacheUpdateTimer += DeltaTime;
    if (CacheUpdateTimer >= StimulusUpdateInterval)
    {
        // Obtener estímulos activos del StimulusSubsystem
        if (StimulusSubsystem && StimulusSubsystem->IsValidLowLevel())
        {
            CachedActiveStimuli = StimulusSubsystem->GetEnvironmentStimuli();

            // Vía rápida: cachear último estímulo del jugador
            FStimulusData LatestPlayerStimulus;
            bHasCachedPlayerStimulus = StimulusSubsystem->TryGetLatestPlayerStimulus(LatestPlayerStimulus);
            if (bHasCachedPlayerStimulus)
            {
                CachedLatestPlayerStimulus = LatestPlayerStimulus;
            }

            // DEBUG: Log estímulos activos (muy infrecuente)
            static int32 DebugCounter = 0;
            if (++DebugCounter % 100 == 0)
            {
                UE_LOG(LogTemp, Log, TEXT("🧠 StimulusProcessor: Env stimuli: %d, PlayerCached: %s"), CachedActiveStimuli.Num(), bHasCachedPlayerStimulus ? TEXT("Sí") : TEXT("No"));
            }
        }
        else
        {
            // Intentar obtener referencia al StimulusSubsystem
            if (UWorld *World = GetWorld())
            {
                StimulusSubsystem = World->GetSubsystem<UStimulusSubsystem>();
                if (StimulusSubsystem)
                {
                    CachedActiveStimuli = StimulusSubsystem->GetEnvironmentStimuli();

                    // Vía rápida: cachear último estímulo del jugador
                    FStimulusData LatestPlayerStimulus;
                    bHasCachedPlayerStimulus = StimulusSubsystem->TryGetLatestPlayerStimulus(LatestPlayerStimulus);
                    if (bHasCachedPlayerStimulus)
                    {
                        CachedLatestPlayerStimulus = LatestPlayerStimulus;
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("🧠 StimulusProcessor: No se pudo obtener StimulusSubsystem"));
                }
            }
        }

        CacheUpdateTimer = 0.0f;
    }
}

void UZombiStimulusProcessor::ProcessStimuliForZombie(const FVector &ZombiePosition, FZombiStimuliFragment &StimuliFragment)
{
    // Limpiar estímulos expirados
    if (StimuliFragment.IsStimulusExpired())
    {
        StimuliFragment.ClearStimuli();
        return;
    }

    // Vía rápida: estímulo del jugador cacheado
    if (bHasCachedPlayerStimulus && IsStimulusInRange(ZombiePosition, CachedLatestPlayerStimulus))
    {
        StimuliFragment.UpdateStimulus(CachedLatestPlayerStimulus, ZombiePosition);
    }

    // Procesar cada estímulo activo (ambientales)
    for (const FStimulusData &Stimulus : CachedActiveStimuli)
    {
        // Verificar si el estímulo está en rango
        if (IsStimulusInRange(ZombiePosition, Stimulus))
        {
            // Actualizar estímulo del zombie
            StimuliFragment.UpdateStimulus(Stimulus, ZombiePosition);
            // Log por-entidad eliminado del hot-path
        }
    }
}

bool UZombiStimulusProcessor::IsStimulusInRange(const FVector &ZombiePosition, const FStimulusData &Stimulus) const
{
    // Calcular distancia al estímulo
    float Distance = FVector::Dist(ZombiePosition, Stimulus.Position);

    // Verificar si está en rango de detección y en radio del estímulo
    return Distance <= StimulusDetectionRange && Distance <= Stimulus.Radius;
}

void UZombiStimulusProcessor::UpdateResponseTimers(FZombiStimuliFragment &StimuliFragment, float DeltaTime)
{
    // Actualizar timer de respuesta
    if (StimuliFragment.HasAnyStimulus())
    {
        StimuliFragment.ResponseTimer += static_cast<uint16>(DeltaTime * 100.0f);
    }
}
