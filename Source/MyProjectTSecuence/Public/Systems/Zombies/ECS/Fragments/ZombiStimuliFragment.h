#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "Systems/StimulusSubsystem/StimulusTypes.h"
#include "ZombiStimuliFragment.generated.h"

/**
 * Fragmento para almacenar estímulos recibidos por un zombie
 * Ultra-optimizado para ECS - 32 bytes máximo
 * Optimizado para 10,000 entidades
 */
USTRUCT()
struct FZombiStimuliFragment : public FMassFragment
{
    GENERATED_BODY()

    // Estímulo más fuerte recibido (32 bytes)
    UPROPERTY()
    FStimulusData StrongestStimulus;

    // Flags de estímulos activos (1 byte)
    UPROPERTY()
    uint8 ActiveStimulusFlags = 0;

    // Intensidad total de estímulos (1 byte)
    UPROPERTY()
    uint8 TotalStimulusIntensity = 0;

    // Dirección del estímulo más fuerte (12 bytes)
    UPROPERTY()
    FVector StimulusDirection = FVector::ZeroVector;

    // Distancia al estímulo más fuerte (4 bytes)
    UPROPERTY()
    float StimulusDistance = 0.0f;

    // Timer de respuesta al estímulo (2 bytes)
    UPROPERTY()
    uint16 ResponseTimer = 0;

    // Constructor por defecto
    FZombiStimuliFragment()
    {
        StrongestStimulus = FStimulusData();
        ActiveStimulusFlags = 0;
        TotalStimulusIntensity = 0;
        StimulusDirection = FVector::ZeroVector;
        StimulusDistance = 0.0f;
        ResponseTimer = 0;
    }

    // Getters
    bool HasVisualStimulus() const { return (ActiveStimulusFlags & 0x01) != 0; }
    bool HasAuditoryStimulus() const { return (ActiveStimulusFlags & 0x02) != 0; }
    bool HasPlayerStimulus() const { return StrongestStimulus.GetStimulusSource() == EStimulusSource::Player; }
    bool HasAnyStimulus() const { return ActiveStimulusFlags != 0 && TotalStimulusIntensity > 0; }
    bool IsStimulusExpired() const { return ResponseTimer >= 255; }

    // Setters
    void SetVisualStimulus(bool bHasVisual)
    {
        if (bHasVisual)
            ActiveStimulusFlags |= 0x01;
        else
            ActiveStimulusFlags &= ~0x01;
    }

    void SetAuditoryStimulus(bool bHasAuditory)
    {
        if (bHasAuditory)
            ActiveStimulusFlags |= 0x02;
        else
            ActiveStimulusFlags &= ~0x02;
    }

    // Métodos de utilidad
    void ClearStimuli()
    {
        StrongestStimulus = FStimulusData();
        ActiveStimulusFlags = 0;
        TotalStimulusIntensity = 0;
        StimulusDirection = FVector::ZeroVector;
        StimulusDistance = 0.0f;
        ResponseTimer = 0;
    }

    void UpdateStimulus(const FStimulusData &NewStimulus, const FVector &ZombiePosition)
    {
        // Solo actualizar si el nuevo estímulo es más fuerte
        if (NewStimulus.Intensity > StrongestStimulus.Intensity)
        {
            StrongestStimulus = NewStimulus;

            // Calcular dirección y distancia
            StimulusDirection = (NewStimulus.Position - ZombiePosition).GetSafeNormal();
            StimulusDistance = FVector::Dist(NewStimulus.Position, ZombiePosition);

            // Actualizar flags según el tipo
            switch (NewStimulus.GetStimulusType())
            {
            case EStimulusType::Visual:
                SetVisualStimulus(true);
                break;
            case EStimulusType::Auditory:
                SetAuditoryStimulus(true);
                break;
            default:
                break;
            }

            // Actualizar intensidad total
            TotalStimulusIntensity = NewStimulus.Intensity;

            // Resetear timer de respuesta
            ResponseTimer = 0;
        }
    }
};
