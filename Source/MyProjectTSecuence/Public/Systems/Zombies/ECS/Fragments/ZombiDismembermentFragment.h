#pragma once

#include "MassEntityTypes.h"
#include "Engine/Engine.h"
#include "ZombiDismembermentFragment.generated.h"

/**
 * @brief Modos de locomoción basados en partes faltantes
 */
UENUM(BlueprintType)
enum class EZombiLocomotionMode : uint8
{
    Walk,  // Caminar normal (todas las partes)
    Run,   // Correr (todas las partes)
    Crawl, // Gatear (falta pierna)
    Hop    // Saltar con una pierna
};

/**
 * @brief Capacidades de ataque según brazos disponibles
 */
UENUM(BlueprintType)
enum class EZombiAbilityMask : uint8
{
    None = 0x00,        // Sin capacidades
    AttackLeft = 0x01,  // Puede atacar con brazo izquierdo
    AttackRight = 0x02, // Puede atacar con brazo derecho
    AttackBoth = 0x03,  // Puede atacar con ambos brazos
    Grab = 0x04,        // Puede agarrar
    Throw = 0x08        // Puede lanzar
};

/**
 * @brief Fragmento para sistema de desmembramiento optimizado para 10,000+ entidades
 *
 * Este fragmento maneja:
 * - Máscara de partes faltantes (cabeza, brazos, piernas)
 * - Modo de locomoción basado en partes faltantes
 * - Capacidades de ataque según brazos disponibles
 * - Variantes de animación para diferentes estados
 * - Material clipping para ocultar partes faltantes
 * - Optimización LOD para desmembramiento
 */
USTRUCT()
struct FZombiDismembermentFragment : public FMassFragment
{
    GENERATED_BODY()

    // Máscara de partes faltantes (8 bits - 1 byte)
    // Bit 0: Cabeza faltante
    // Bit 1: Brazo izquierdo faltante
    // Bit 2: Brazo derecho faltante
    // Bit 3: Pierna izquierda faltante
    // Bit 4: Pierna derecha faltante
    // Bit 5-7: Reservados para futuras partes
    UPROPERTY()
    uint8 MissingPartsMask = 0x00;

    // Modo de locomoción actual (1 byte)
    UPROPERTY()
    EZombiLocomotionMode LocomotionMode = EZombiLocomotionMode::Walk;

    // Capacidades de ataque (1 byte)
    UPROPERTY()
    EZombiAbilityMask AbilityMask = EZombiAbilityMask::AttackBoth;

    // Índice de variante de animación (1 byte)
    UPROPERTY()
    uint8 AnimVariantIndex = 0;

    // Flags de optimización (1 byte)
    // Bit 0: Necesita actualización de material
    // Bit 1: Necesita actualización de animación
    // Bit 2: Necesita actualización de locomoción
    // Bit 3: Necesita actualización de LOD
    UPROPERTY()
    uint8 UpdateFlags = 0x00;

    // Tiempo desde el último desmembramiento (4 bytes)
    UPROPERTY()
    float LastDismembermentTime = 0.0f;

    // Constructor por defecto
    FZombiDismembermentFragment()
    {
        MissingPartsMask = 0x00;
        LocomotionMode = EZombiLocomotionMode::Walk;
        AbilityMask = EZombiAbilityMask::AttackBoth;
        AnimVariantIndex = 0;
        UpdateFlags = 0x00;
        LastDismembermentTime = 0.0f;
    }

    // Constantes para máscaras de partes
    static constexpr uint8 HEAD_MASK = 0x01;
    static constexpr uint8 LEFT_ARM_MASK = 0x02;
    static constexpr uint8 RIGHT_ARM_MASK = 0x04;
    static constexpr uint8 LEFT_LEG_MASK = 0x08;
    static constexpr uint8 RIGHT_LEG_MASK = 0x10;

    // Constantes para flags de actualización
    static constexpr uint8 MATERIAL_UPDATE_FLAG = 0x01;
    static constexpr uint8 ANIMATION_UPDATE_FLAG = 0x02;
    static constexpr uint8 LOCOMOTION_UPDATE_FLAG = 0x04;
    static constexpr uint8 LOD_UPDATE_FLAG = 0x08;

    // SOLO DATOS - SIN MÉTODOS (REGLA DE SIMPLICIDAD PARA 10,000+ ENTIDADES)
};
