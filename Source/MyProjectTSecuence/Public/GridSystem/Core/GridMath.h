#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"

// Utilidades matemáticas del grid: costos, clamps y helpers de dirección
// Header-only para avanzar gradualmente.

namespace GridMath
{
	// Costo de movimiento para índice de vecino 0..7 (ver Grid::Neigh8Cost)
	FORCEINLINE float MoveCost8(const int32 NeighIndex)
	{
		return Grid::Neigh8Cost[FMath::Clamp(NeighIndex, 0, 7)];
	}

	// Lerp entre dos direcciones 2D normalizadas, con normalización final (evita NaN)
	FORCEINLINE FVector2D LerpDirNormalized(const FVector2D& A, const FVector2D& B, const float T)
	{
		const FVector2D L = FMath::Lerp(A, B, T);
		const float LenSq = L.SizeSquared();
		if (LenSq > KINDA_SMALL_NUMBER)
		{
			return L / FMath::Sqrt(LenSq);
		}
		return FVector2D::ZeroVector;
	}

	// Softmax simple sobre 8 deltas de distancia. Retorna pesos normalizados en OutW[8].
	// T: temperatura (>0). Si suma ~0, retorna distribución uniforme.
	FORCEINLINE void Softmax8(const float Deltas[8], const float T, float OutW[8])
	{
		const float InvT = (T > KINDA_SMALL_NUMBER) ? (1.0f / T) : 1.0f;
		float Sum = 0.0f;
		for (int i = 0; i < 8; ++i)
		{
			const float w = FMath::Exp(-Deltas[i] * InvT);
			OutW[i] = w;
			Sum += w;
		}
		if (Sum <= KINDA_SMALL_NUMBER)
		{
			const float U = 1.0f / 8.0f;
			for (int i = 0; i < 8; ++i) { OutW[i] = U; }
			return;
		}
		const float InvSum = 1.0f / Sum;
		for (int i = 0; i < 8; ++i) { OutW[i] *= InvSum; }
	}
}
