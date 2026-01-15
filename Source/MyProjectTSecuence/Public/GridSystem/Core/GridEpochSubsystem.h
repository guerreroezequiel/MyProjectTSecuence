#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GridSystem/Core/GridEpoch.h"
#include "GridSystem/FlowField/FlowFieldSolver.h" // For Grid::Flow::FGoalSet
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridEpochSubsystem.generated.h"

/**
 * UGridEpochSubsystem
 * - Subsystem por mundo que ejecuta tareas del Grid solo cuando avanza el epoch global.
 * - Frecuencia definida por GridConfig::EpochMs (ver GridConfig.h y GridEpoch.h).
 */
UCLASS()
class MYPROJECTTSECUENCE_API UGridEpochSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// UWorldSubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// Tick manual a través de WorldSubsystem: usamos el Ticker del mundo
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// Tick principal: llamado desde el FTSTicker vía binding interno
	void Tick(float DeltaSeconds);

	// Si se quiere forzar una pasada de rebuild en el siguiente epoch
	void RequestDirtyRebuildAll();

	// Acceso público a las metas para componentes externos
	Grid::Flow::FGoalSet Goals;

private:
	double LastTimeSeconds = 0.0;
	uint32 LastEpochIndex = 0u;

	// Handle del ticker
	FTSTicker::FDelegateHandle TickerHandle;

	bool TickInternal(float DeltaSeconds);
};
