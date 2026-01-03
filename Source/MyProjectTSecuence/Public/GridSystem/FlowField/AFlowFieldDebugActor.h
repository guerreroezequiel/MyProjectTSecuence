#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AFlowFieldDebugActor.generated.h"

class UFlowFieldMovementComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class MYPROJECTTSECUENCE_API AFlowFieldDebugActor : public AActor
{
    GENERATED_BODY()

public:
    AFlowFieldDebugActor();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // Componente de movimiento que sigue el FlowField
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FlowField")
    UFlowFieldMovementComponent* MovementComponent;

    // Mesh visual (esfera estática de 25cm)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    // Texto para mostrar coordenadas del mundo
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UTextRenderComponent* TextRenderComponent;

    // Velocidad de movimiento configurable
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowField")
    float MovementSpeed = 30.0f;

    // Color para debug visual
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowField")
    FLinearColor DebugColor = FLinearColor::Green;

private:
    void SetupMesh();
    void SetupTextRender();
    void SetupMovementComponent();
};
