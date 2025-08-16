#pragma once

#include "MassProcessor.h"
#include "MassEntityTypes.h"
#include "Systems/Zombies/ECS/Fragments/ZombiUltraConsolidatedFragment.h"
#include "Systems/Zombies/ECS/Tags/ZombiTags.h"
#include "Engine/Engine.h"
#include "ZombiUltraConsolidatedProcessor.generated.h"

/**
 * @brief Procesador ultra-consolidado para 10,000+ entidades
 *
 * REGLA: UN SOLO PROCESADOR + UN SOLO FRAGMENTO = MÁXIMO RENDIMIENTO
 * - Todo en un solo procesador
 * - Un solo fragmento por entidad
 * - Sin tráfico de datos
 * - Cache locality perfecta
 */
UCLASS()
class MYPROJECTTSECUENCE_API UZombiUltraConsolidatedProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UZombiUltraConsolidatedProcessor();

protected:
    virtual void ConfigureQueries() override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    // UN SOLO QUERY = TODAS LAS ENTIDADES
    FMassEntityQuery ZombiQuery{*this};

    // CONSTANTES INLINE
    static constexpr float WALK_SPEED = 100.0f;
    static constexpr float RUN_SPEED = 200.0f;
    static constexpr float ROTATION_SPEED = 360.0f;
    static constexpr float CHASE_DISTANCE = 1000.0f;
    static constexpr float ATTACK_DISTANCE = 150.0f;
    static constexpr float MOVEMENT_RADIUS = 500.0f;
    static constexpr float MAX_PROCESSING_DISTANCE = 1500.0f;

    // LÓGICA INLINE = TODO EN UN SOLO MÉTODO
    void ProcessZombi(FZombiUltraConsolidatedFragment &Zombi,
                      float DeltaTime,
                      const FVector &PlayerLocation);

    // MÉTODOS DE PROCESAMIENTO POR ESTADO
    void ProcessIdleState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime);
    void ProcessWalkState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime);
    void ProcessChaseState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime, const FVector &PlayerLocation);
    void ProcessAttackState(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime);
    void ProcessBehavior(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime, const FVector &PlayerLocation);
    void UpdateCooldowns(FZombiUltraConsolidatedFragment &Zombi, float DeltaTime);
    void UpdateAnimation(FZombiUltraConsolidatedFragment &Zombi, uint8 PreviousState);

    // CÁLCULOS INLINE = SIN HELPERS
    inline float GetDistanceToPlayer(const FVector &ZombiPos, const FVector &PlayerPos) const
    {
        return FVector::Dist(ZombiPos, PlayerPos);
    }

    inline bool ShouldChase(float Distance) const
    {
        return Distance < CHASE_DISTANCE;
    }

    inline bool ShouldAttack(float Distance) const
    {
        return Distance < ATTACK_DISTANCE;
    }

    inline FVector GetRandomDirection() const
    {
        return FVector(FMath::RandRange(-1.0f, 1.0f), FMath::RandRange(-1.0f, 1.0f), 0.0f).GetSafeNormal();
    }

    inline void UpdateTransform(FZombiUltraConsolidatedFragment &Zombi) const
    {
        // Construir transform solo cuando sea necesario
        if (Zombi.IsMeshDataValid())
        {
            // TurboSequence maneja el resto automáticamente
            // Solo necesitamos actualizar posición y rotación
        }
    }

    // OPTIMIZACIÓN DE PRIORIDADES
    inline uint8 CalculatePriority(float Distance) const
    {
        if (Distance <= 300.0f)
            return ZombiPriorities::CRITICAL;
        if (Distance <= 600.0f)
            return ZombiPriorities::HIGH;
        if (Distance <= 1000.0f)
            return ZombiPriorities::NORMAL;
        return ZombiPriorities::LOW;
    }

    inline float CalculateUpdateInterval(uint8 Priority) const
    {
        switch (Priority)
        {
        case ZombiPriorities::CRITICAL:
            return 1.0f / 60.0f; // 60 FPS
        case ZombiPriorities::HIGH:
            return 1.0f / 30.0f; // 30 FPS
        case ZombiPriorities::NORMAL:
            return 1.0f / 15.0f; // 15 FPS
        case ZombiPriorities::LOW:
            return 1.0f / 5.0f; // 5 FPS
        default:
            return 1.0f / 15.0f;
        }
    }
};
