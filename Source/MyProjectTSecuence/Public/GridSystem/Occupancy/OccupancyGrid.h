#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"

// Stub mínimo de OccupancyGrid para permitir compilar dependencias.
// Referencia: GRID_SYSTEM.md → Capa 2 – Occupancy/Capacity
// Header-only (retornos por defecto) hasta implementar almacenamiento real.

namespace Grid
{
	// Estado de celda (mínimo)
	enum class ECellState : uint8
	{
		Empty = 0,
		Obstacle,
		Portal
	};

	// Consultas inline (por ahora retornos por defecto no bloqueantes)
	FORCEINLINE bool IsBlocked(const FIntPoint& CellXY)
	{
		// TODO: reemplazar con consulta real a datos de ocupación
		return false;
	}

	FORCEINLINE bool IsPortal(const FIntPoint& CellXY)
	{
		// TODO: reemplazar con consulta real a portales
		return false;
	}

	FORCEINLINE ECellState GetCellState(const FIntPoint& CellXY)
	{
		// TODO: reemplazar con consulta real
		return ECellState::Empty;
	}
}
