#pragma once

#include "CoreMinimal.h"

// Stub mínimo de CapacityGrid (header-only)
// Referencia: GRID_SYSTEM.md → Capa 2 – Occupancy/Capacity (HardOnly vs SoftAdaptive)
// Mantiene defaults locales para no tocar GridConfig en esta etapa.

namespace Grid
{
	namespace Capacity
	{
		// Modo de capacidad
		enum class ECapacityMode : uint8
		{
			HardOnly = 0,     // no se permite exceso
			SoftAdaptive      // se permite exceso suave que influye en costos
		};

		// Defaults mínimos (pueden cambiarse al integrar con tunables)
		inline constexpr ECapacityMode Mode = ECapacityMode::SoftAdaptive;
		inline constexpr float SoftCapacityDelta = 0.5f; // exceso relativo permitido (p.ej., +50%)

		// Devuelve la capacidad efectiva de una celda dado un baseCapacity.
		// SoftAdaptive: base + floor(base * SoftCapacityDelta). HardOnly: base.
		FORCEINLINE int32 EffectiveCapacity(const int32 BaseCapacity)
		{
			if (Mode == ECapacityMode::SoftAdaptive)
			{
				const int32 Extra = FMath::FloorToInt(static_cast<float>(BaseCapacity) * SoftCapacityDelta);
				return BaseCapacity + FMath::Max(0, Extra);
			}
			return BaseCapacity;
		}

		// Limita un conteo actual a la capacidad efectiva (clamp)
		FORCEINLINE int32 ClampToCapacity(const int32 CurrentCount, const int32 BaseCapacity)
		{
			return FMath::Min(CurrentCount, EffectiveCapacity(BaseCapacity));
		}

		// Factor de inflación de costo por exceso (placeholder para integrar con Flow)
		// overRatio = max(0, (CurrentCount - BaseCapacity) / max(1, BaseCapacity))
		// Retorna 1 si no hay exceso o si Mode == HardOnly.
		FORCEINLINE float CostInflationFactor(const int32 CurrentCount, const int32 BaseCapacity)
		{
			if (Mode == ECapacityMode::HardOnly || BaseCapacity <= 0)
			{
				return 1.0f;
			}
			const float OverRatio = FMath::Max(0.0f, (static_cast<float>(CurrentCount - BaseCapacity)) / FMath::Max(1.0f, static_cast<float>(BaseCapacity)));
			return 1.0f + OverRatio; // simple y monotónica; integrar con α/β más adelante
		}
	}
}
