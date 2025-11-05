#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Math/IntPoint.h"
#include "GridDebugDrawComponent.generated.h"

/**
 * Componente para debug visual del grid.
 * Permite controlar la visualización mediante comandos de consola.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYPROJECTTSECUENCE_API UGridDebugDrawComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UGridDebugDrawComponent();

	// Toggles simples expuestos a Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	bool bDrawCapacity = false;  // Desactivado por defecto, se activa por consola

	// Muestra el borde del tile actual
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	bool bDrawTileBorder = true;

	// Celda específica a dibujar (si es válida)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	FIntPoint DebugCell = FIntPoint(INDEX_NONE, INDEX_NONE);

	// Tamaño de caja/altura del texto
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	float BoxExtent = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid Debug")
	float TextZOffset = 30.f;

	// Actualiza la celda de depuración
	void SetDebugCell(int32 X, int32 Y);

protected:
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// Funciones de dibujo
	void DrawCapacity();
	void DrawTileBorder();
	void DrawDebugCell(const FIntPoint& Cell);
};
