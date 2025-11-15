#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UFlowFieldMovementComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UFlowFieldMovementComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFlowFieldMovementComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Velocidad de movimiento en unidades/segundo
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float MovementSpeed = 200.0f;

    // Distancia mínima a la meta para considerar llegada
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    float GoalThreshold = 50.0f;

    // Si debe detenerse al llegar a la meta
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
    bool bStopAtGoal = true;

    // Si debe mostrar debug de dirección
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebugDirection = false;

    // Color de línea de debug
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FLinearColor DebugLineColor = FLinearColor::Red;

protected:
    virtual void BeginPlay() override;

private:
    // Obtener la coordenada de celda del mundo
    FIntPoint GetCellCoordinate(const FVector& WorldPosition) const;

    // Leer dirección del FlowField en posición actual
    FVector2D GetFlowDirectionAtPosition(const FVector& WorldPosition) const;

    // Aplicar movimiento basado en dirección del FlowField
    void ApplyFlowMovement(float DeltaTime);

    // Debug visual
    void DrawDebugInfo() const;

    // Verificar si hemos llegado a la meta
    bool HasReachedGoal() const;

    // Cache de última posición para optimización
    mutable FVector LastWorldPosition;
    mutable FVector2D CachedFlowDirection;
    mutable double LastDirectionCheckTime = 0.0;
    static constexpr double DIRECTION_CACHE_DURATION = 0.1; // Cache por 100ms
};
