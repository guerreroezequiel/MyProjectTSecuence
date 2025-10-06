#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"

// Helpers mínimos para manejar Epoch global (ver GRID_SYSTEM.md: Epoch = 200 ms)
// Sin .cpp para mantenerlo simple y header-only.

namespace GridEpoch
{
	// Duración de un epoch en milisegundos (tomado de GridConfig)
	inline constexpr float EpochMs = GridConfig::EpochMs;

	// Convierte segundos a cantidad de epochs (float)
	FORCEINLINE float SecondsToEpochs(const float Seconds)
	{
		return (Seconds * 1000.0f) / EpochMs;
	}

	// Convierte milisegundos a cantidad de epochs (float)
	FORCEINLINE float MillisToEpochs(const float Millis)
	{
		return Millis / EpochMs;
	}

	// Devuelve un índice de epoch (uint32) dado el tiempo del juego en segundos
	FORCEINLINE uint32 EpochIndexFromTimeSeconds(const double TimeSeconds)
	{
		const double Millis = TimeSeconds * 1000.0;
		return static_cast<uint32>(FMath::FloorToDouble(Millis / static_cast<double>(EpochMs)));
	}

	// Determina si hubo cambio de epoch entre dos tiempos (segundos)
	FORCEINLINE bool HasEpochAdvanced(const double PrevTimeSeconds, const double CurrTimeSeconds)
	{
		return EpochIndexFromTimeSeconds(CurrTimeSeconds) != EpochIndexFromTimeSeconds(PrevTimeSeconds);
	}
}
