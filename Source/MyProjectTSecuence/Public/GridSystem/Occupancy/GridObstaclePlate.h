#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridObstaclePlate.generated.h"

class UStaticMeshComponent;

UCLASS()
class MYPROJECTTSECUENCE_API AGridObstaclePlate : public AActor
{
    GENERATED_BODY()

public:
    AGridObstaclePlate();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* Mesh;
};
