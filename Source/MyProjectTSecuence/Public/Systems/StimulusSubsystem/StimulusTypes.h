#pragma once

#include "CoreMinimal.h"
#include "StimulusTypes.generated.h"

/**
 * Tipos de estímulos que pueden recibir los zombis
 * Optimizado para ECS - usando flags en lugar de enums complejos
 * Escalable para futuros tipos de estímulos
 */
UENUM(BlueprintType)
enum class EStimulusType : uint8
{
    None = 0,
    Visual = 1,        // Estímulo visual (ver algo)
    Auditory = 2,      // Estímulo auditivo (escuchar algo)
    Tactile = 3,       // Estímulo táctil (tocar algo)
    Olfactory = 4,     // Estímulo olfativo (oler algo)
    Environmental = 5, // Estímulo ambiental (temperatura, etc.)
    Custom = 6         // Estímulo personalizado
};

/**
 * Fuentes de estímulos
 * Escalable para futuras fuentes
 */
UENUM(BlueprintType)
enum class EStimulusSource : uint8
{
    None = 0,
    Player = 1,      // Jugador
    Item = 2,        // Item/objeto
    Animal = 3,      // Animal
    Environment = 4, // Entorno
    Zombie = 5,      // Otro zombie
    Explosion = 6,   // Explosión
    Custom = 7       // Fuente personalizada
};

/**
 * Estructura de datos para un estímulo
 * Ultra-optimizada para ECS - 32 bytes máximo
 * Escalable para cualquier tipo de estímulo
 */
USTRUCT(BlueprintType)
struct FStimulusData
{
    GENERATED_BODY()

    // Posición del estímulo (12 bytes)
    UPROPERTY()
    FVector Position = FVector::ZeroVector;

    // Dirección del estímulo (12 bytes)
    UPROPERTY()
    FVector Direction = FVector::ZeroVector;

    // Tipo de estímulo (1 byte)
    UPROPERTY()
    uint8 StimulusType = 0;

    // Fuente del estímulo (1 byte)
    UPROPERTY()
    uint8 StimulusSource = 0;

    // Intensidad del estímulo (1 byte) - 0-255
    UPROPERTY()
    uint8 Intensity = 0;

    // Radio de propagación (4 bytes)
    UPROPERTY()
    float Radius = 0.0f;

    // Timer de decaimiento (1 byte)
    UPROPERTY()
    uint8 DecayTimer = 0;

    // Constructor por defecto
    FStimulusData()
    {
        Position = FVector::ZeroVector;
        Direction = FVector::ZeroVector;
        StimulusType = 0;
        StimulusSource = 0;
        Intensity = 0;
        Radius = 0.0f;
        DecayTimer = 0;
    }

    // Constructor con parámetros
    FStimulusData(const FVector &InPosition, const FVector &InDirection,
                  EStimulusType InType, EStimulusSource InSource,
                  uint8 InIntensity, float InRadius)
    {
        Position = InPosition;
        Direction = InDirection;
        StimulusType = static_cast<uint8>(InType);
        StimulusSource = static_cast<uint8>(InSource);
        Intensity = InIntensity;
        Radius = InRadius;
        DecayTimer = 0;
    }

    // Getters
    EStimulusType GetStimulusType() const { return static_cast<EStimulusType>(StimulusType); }
    EStimulusSource GetStimulusSource() const { return static_cast<EStimulusSource>(StimulusSource); }
    bool IsValid() const { return StimulusType != 0 && Intensity > 0; }
    bool HasExpired() const { return DecayTimer >= 255; }

    // Setters
    void SetStimulusType(EStimulusType Type) { StimulusType = static_cast<uint8>(Type); }
    void SetStimulusSource(EStimulusSource Source) { StimulusSource = static_cast<uint8>(Source); }
    void SetIntensity(uint8 InIntensity) { Intensity = InIntensity; }
    void SetRadius(float InRadius) { Radius = InRadius; }
};
