// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MassCommonFragments.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Core/GridConfig.h"
#include "CellLocationFragment.generated.h"

/**
 * Fragment that stores the grid cell location of an entity
 */
USTRUCT(BlueprintType)
struct MYPROJECTTSECUENCE_API FCellLocationFragment : public FMassFragment
{
    GENERATED_BODY()

    /** Tile coordinates in the grid */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    FIntPoint TileXY = FIntPoint(0, 0);

    /** Cell index within the tile */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    int32 CellIndex = 0;

    /** Whether this location is valid */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid")
    bool bValid = false;
};
