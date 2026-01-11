#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/TileRegistry.h"
#include "GridSystem/FlowField/FlowField.h"

// FlowFieldRegistry: compat helpers en Grid::Flow para consola y sistemas existentes

namespace Grid
{
	namespace Flow
	{
		inline TMap<FIntPoint, TSharedPtr<FFlowField>> GFlowFields;

		FORCEINLINE TSharedPtr<FFlowField> EnsureFlowField(const FIntPoint& TileXY)
		{
			TSharedPtr<FFlowField>* Found = GFlowFields.Find(TileXY);
			if (!Found)
			{
				TSharedPtr<FFlowField> NewField = MakeShared<FFlowField>();
				GFlowFields.Add(TileXY, NewField);
				return NewField;
			}
			return *Found;
		}

		FORCEINLINE FFlowField* GetFlowField(const FIntPoint& TileXY)
		{
			// Auto-create if missing to satisfy console/tools expectations
			return EnsureFlowField(TileXY).Get();
		}

		FORCEINLINE FTileContext* GetTileContext(const FIntPoint& TileXY)
		{
			TSharedPtr<FTileContext> Ctx = Grid::Tiles::GetTileContext(TileXY);
			return Ctx.Get();
		}
	}
}
