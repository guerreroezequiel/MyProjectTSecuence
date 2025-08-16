#pragma once

#include "MassEntityTypes.h"
#include "Engine/Engine.h"
#include "TurboSequence_MinimalData_Lf.h"
#include "TurboSequence_MeshAsset_Lf.h"
#include "ZombiUltraConsolidatedFragment.generated.h"

/**
 * @brief Fragmento ultra-consolidado para 10,000+ entidades
 *
 * REGLA: UN SOLO FRAGMENTO = TODO LO ESENCIAL
 * - Minimiza tráfico de datos
 * - Mejor cache locality
 * - Sin AddFragment/RemoveFragment
 * - Solo datos que SIEMPRE se necesitan
 */
USTRUCT()
struct FZombiUltraConsolidatedFragment : public FMassFragment
{
    GENERATED_BODY()

    // === DATOS DE TRANSFORMACIÓN (12 bytes) ===
    UPROPERTY()
    FVector Position = FVector::ZeroVector; // Posición en el mundo

    UPROPERTY()
    FRotator Rotation = FRotator::ZeroRotator; // Rotación actual

    UPROPERTY()
    float MovementSpeed = 100.0f; // Velocidad actual

    // === DATOS DE ESTADO (4 bytes) ===
    UPROPERTY()
    uint8 State = 0; // Estado: 0=Idle, 1=Walk, 2=Chase, 3=Attack, 4=Dead

    UPROPERTY()
    uint8 Health = 100; // Salud (0-255)

    UPROPERTY()
    uint8 Flags = 0; // Flags: bit0=IsChasing, bit1=IsAttacking, bit2=IsDamaged

    UPROPERTY()
    uint8 UpdatePriority = 1; // Prioridad: 0=Critical, 1=High, 2=Normal, 3=Low

    // === DATOS DE MOVIMIENTO (12 bytes) ===
    UPROPERTY()
    FVector MovementDirection = FVector::ForwardVector; // Dirección de movimiento

    UPROPERTY()
    float BehaviorTimer = 0.0f; // Timer para cambios de comportamiento

    UPROPERTY()
    float DirectionChangeInterval = 3.0f; // Intervalo para cambiar dirección

    // === DATOS DE COMBATE (8 bytes) ===
    UPROPERTY()
    float AttackCooldown = 0.0f; // Cooldown de ataque

    UPROPERTY()
    float DamageCooldown = 0.0f; // Cooldown de daño

    UPROPERTY()
    float LastDamageTime = 0.0f; // Tiempo del último daño

    // === DATOS DE OPTIMIZACIÓN (8 bytes) ===
    UPROPERTY()
    float DistanceToPlayer = 0.0f; // Distancia al jugador

    UPROPERTY()
    float LastUpdateTime = 0.0f; // Último tiempo de update

    UPROPERTY()
    float UpdateInterval = 1.0f / 60.0f; // Intervalo de update

    // === DATOS VISUALES (8 bytes) ===
    UPROPERTY()
    FTurboSequence_MinimalMeshData_Lf MeshData; // Handle visual de TurboSequence

    UPROPERTY()
    int32 UpdateGroupIndex = 0; // Grupo de actualización

    // === CONSTRUCTOR ===
    FZombiUltraConsolidatedFragment()
    {
        // Inicialización por defecto
        Position = FVector::ZeroVector;
        Rotation = FRotator::ZeroRotator;
        MovementSpeed = 100.0f;
        State = 1; // Walk por defecto
        Health = 100;
        Flags = 0;
        UpdatePriority = 2; // Normal por defecto
        MovementDirection = FVector::ForwardVector;
        BehaviorTimer = 0.0f;
        DirectionChangeInterval = 3.0f;
        AttackCooldown = 0.0f;
        DamageCooldown = 0.0f;
        LastDamageTime = 0.0f;
        DistanceToPlayer = 0.0f;
        LastUpdateTime = 0.0f;
        UpdateInterval = 1.0f / 60.0f;
        UpdateGroupIndex = 0;
    }

    // === MÉTODOS INLINE PARA ACCESO RÁPIDO ===

    // Estados
    inline bool IsDead() const { return State == 4; }
    inline bool IsChasing() const { return (Flags & 0x01) != 0; }
    inline bool IsAttacking() const { return (Flags & 0x02) != 0; }
    inline bool IsDamaged() const { return (Flags & 0x04) != 0; }

    // Setters
    inline void SetDead() { State = 4; }
    inline void SetChasing(bool bChasing) { Flags = bChasing ? (Flags | 0x01) : (Flags & ~0x01); }
    inline void SetAttacking(bool bAttacking) { Flags = bAttacking ? (Flags | 0x02) : (Flags & ~0x02); }
    inline void SetDamaged(bool bDamaged) { Flags = bDamaged ? (Flags | 0x04) : (Flags & ~0x04); }

    // Combate
    inline bool CanAttack() const { return AttackCooldown <= 0.0f; }
    inline bool CanTakeDamage() const { return DamageCooldown <= 0.0f; }
    inline float GetHealthPercentage() const { return Health / 100.0f; }

    // Optimización
    inline bool NeedsUpdate(float CurrentTime) const { return (CurrentTime - LastUpdateTime) >= UpdateInterval; }
    inline bool IsInRange() const { return DistanceToPlayer <= 1500.0f; }

    // Visual
    inline bool IsMeshDataValid() const { return MeshData.IsMeshDataValid(); }
};

// === CONSTANTES PARA ESTADOS ===
namespace ZombiStates
{
    static constexpr uint8 IDLE = 0;
    static constexpr uint8 WALK = 1;
    static constexpr uint8 CHASE = 2;
    static constexpr uint8 ATTACK = 3;
    static constexpr uint8 DEAD = 4;
}

// === CONSTANTES PARA PRIORIDADES ===
namespace ZombiPriorities
{
    static constexpr uint8 CRITICAL = 0; // 60 FPS
    static constexpr uint8 HIGH = 1;     // 30 FPS
    static constexpr uint8 NORMAL = 2;   // 15 FPS
    static constexpr uint8 LOW = 3;      // 5 FPS
}
