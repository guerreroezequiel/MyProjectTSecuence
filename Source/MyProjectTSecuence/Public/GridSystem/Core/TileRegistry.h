#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/TileContext/TileContext.h"

// TileRegistry: owner de tiles. Fuente de verdad para FTileContext por TileXY.
// MVP: header-only con Ensure/Get y estado HOT/WARM/COLD opcional.

namespace Grid
{
	namespace Tiles
	{
		// Estado opcional del tile (para scheduling futuro)
		enum class ETileState : uint8 { Hot = 0, Warm, Cold };

		// Contextos por TileXY
		inline TMap<FIntPoint, TSharedPtr<FTileContext>> GTileContexts;
		inline TMap<FIntPoint, ETileState>              GTileStates;

		// Crea o devuelve el contexto del tile y asegura que tenga su TileXY seteado
		FORCEINLINE TSharedPtr<FTileContext> EnsureTileContext(const FIntPoint& TileXY)
		{
			TSharedPtr<FTileContext>* Found = GTileContexts.Find(TileXY);
			if (!Found)
			{
				TSharedPtr<FTileContext> NewCtx = MakeShared<FTileContext>();
				NewCtx->SetTileXY(TileXY);
				GTileContexts.Add(TileXY, NewCtx);
				GTileStates.Add(TileXY, ETileState::Cold);
				return NewCtx;
			}
			return *Found;
		}

		FORCEINLINE TSharedPtr<FTileContext> GetTileContext(const FIntPoint& TileXY)
		{
			if (TSharedPtr<FTileContext>* Found = GTileContexts.Find(TileXY))
			{
				return *Found;
			}
			return nullptr;
		}

		FORCEINLINE bool HasTile(const FIntPoint& TileXY)
		{
			return GTileContexts.Contains(TileXY);
		}

		FORCEINLINE void SetTileState(const FIntPoint& TileXY, const ETileState State)
		{
			EnsureTileContext(TileXY);
			GTileStates.Add(TileXY, State);
		}

		FORCEINLINE ETileState GetTileState(const FIntPoint& TileXY)
		{
			if (ETileState* S = GTileStates.Find(TileXY)) { return *S; }
			return ETileState::Cold;
		}

		FORCEINLINE void ClearAll()
		{
			GTileContexts.Reset();
			GTileStates.Reset();
		}
	}
}
