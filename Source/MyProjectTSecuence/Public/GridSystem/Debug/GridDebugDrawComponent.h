#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GridDebugDrawComponent.generated.h"

/**
 * Componente mínimo para debug visual del grid.
 * Empieza dibujando la capa Capacity; luego se puede extender a Flow/Heat/Occupancy.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UGridDebugDrawComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGridDebugDrawComponent();

	// Toggles simples expuestos a Blueprint para empezar sin CVars
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	bool bDrawCapacity = true;

	// Densidad de muestreo (dibujar 1 de cada N celdas)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug", meta=(ClampMin="1", UIMin="1"))
	int32 GridStep = 2;

	// Tamaño de caja/altura del texto
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	float BoxExtent = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	float TextZOffset = 30.f;

	// Opcional: origen custom del grid (si no se usa, toma el actual de GridWorld)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug|Origin")
	bool bUseCustomOrigin = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug|Origin", meta=(EditCondition="bUseCustomOrigin"))
	FVector CustomOriginWS = FVector::ZeroVector;

	// Toggle por instancia para el contorno del tile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug|Visual")
	bool bDrawTileOutline = true;

protected:
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void DrawCapacity();
	void DrawOccupancy();
	FIntPoint GetFocusTileXY() const;
};
