#pragma once

#include "CoreMinimal.h"

// Tipos y utilidades básicas del grid (IDs, dirs 8-neigh, costos)
// Header-only, sin almacenamiento.

namespace Grid
{
	// Identificador lineal opcional para celdas dentro de un tile (64x64)
	using FCellIndex = int32;

	// Offsets de vecinos en 8 direcciones (DX,DY)
	static const FIntPoint Neigh8[8] = {
		FIntPoint(-1, -1), FIntPoint(0, -1), FIntPoint(1, -1),
		FIntPoint(-1,  0),                    FIntPoint(1,  0),
		FIntPoint(-1,  1), FIntPoint(0,  1), FIntPoint(1,  1)
	};

	// Costos relativos (ortho=1, diag=1.41421356)
	static const float Neigh8Cost[8] = {
		1.41421356f, 1.0f, 1.41421356f,
		1.0f,                 1.0f,
		1.41421356f, 1.0f, 1.41421356f
	};

	// Vector de dirección normalizado aproximado por cada vecino (XY en espacio de celdas)
	static const FVector2D Neigh8Dir[8] = {
		FVector2D(-0.70710678f, -0.70710678f), FVector2D(0.0f, -1.0f), FVector2D(0.70710678f, -0.70710678f),
		FVector2D(-1.0f,        0.0f),                                   FVector2D(1.0f,         0.0f),
		FVector2D(-0.70710678f,  0.70710678f), FVector2D(0.0f,  1.0f), FVector2D(0.70710678f,  0.70710678f)
	};
}
