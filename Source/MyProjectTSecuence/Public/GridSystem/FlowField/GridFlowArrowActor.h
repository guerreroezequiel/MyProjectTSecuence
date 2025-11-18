#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GridFlowArrowActor.generated.h"

class UArrowComponent;

UCLASS()
class MYPROJECTTSECUENCE_API AGridFlowArrowActor : public AActor
{
    GENERATED_BODY()
public:
    AGridFlowArrowActor();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category="Flow")
    void SetCell(const FIntPoint& InCell);

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, Category="Components")
    UArrowComponent* Arrow;

    UPROPERTY(EditAnywhere, Category="Flow")
    FIntPoint Cell = FIntPoint::ZeroValue;

    // Altura visual del arrow sobre el plano
    UPROPERTY(EditAnywhere, Category="Flow")
    float ArrowHeightUU = 20.0f;

    // Largo deseado de la flecha en unidades Unreal
    UPROPERTY(EditAnywhere, Category="Flow")
    float ArrowLengthUU = 50.0f;

    void UpdateTransformFromFlow();
    void ApplyArrowLength();
};
