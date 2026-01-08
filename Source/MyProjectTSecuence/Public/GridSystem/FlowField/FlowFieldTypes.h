// Copyright 2024 MyProjectTSec

#pragma once

#include "CoreMinimal.h"
#include "FlowFieldTypes.generated.h"

UENUM(BlueprintType)
enum class EFlowIntent : uint8
{
    Players,
    Influences,
    Ambient,
    MAX UMETA(Hidden)
};
