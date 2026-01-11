#include "GridSystem/Core/GridEpochSubsystem.h"
#include "Engine/World.h"
#include "Containers/Ticker.h" // FTSTicker
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/Density/DensityHeatGrid.h"

#include "HAL/IConsoleManager.h"

// Consola: Grid.Epoch.Dump
// Uso: en la consola (~), escribir "Grid.Epoch.Dump" para ver estado del epoch actual y configuracion
// CVar para habilitar/deshabilitar el pipeline viejo (Decay + RebuildStep)
static TAutoConsoleVariable<int32> CVar_Grid_EnableLegacyEpoch(
    TEXT("Grid.Epoch.EnableLegacy"),
    0,
    TEXT("Habilita el pipeline viejo del epoch (Decay + RebuildStep). 0=off (default), 1=on"),
    ECVF_Default);

static FAutoConsoleCommandWithWorldArgsAndOutputDevice CCmd_GridEpochDump(
    TEXT("Grid.Epoch.Dump"),
    TEXT("Muestra informacion del epoch actual y parametros del grid (EpochMs)."),
    FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(
        [](const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
        {
            if (!World)
            {
                Ar.Logf(TEXT("[GridEpoch] World=null"));
                return;
            }

            const double TimeSec = World->GetTimeSeconds();
            const uint32 EpochIdx = GridEpoch::EpochIndexFromTimeSeconds(TimeSec);

            Ar.Logf(TEXT("[GridEpoch] Time=%.3f s  EpochIdx=%u  EpochMs=%.1f (%.2f Hz)"),
                TimeSec,
                EpochIdx,
                GridEpoch::EpochMs,
                1000.0f / GridEpoch::EpochMs);
        }
    )
);

void UGridEpochSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    LastTimeSeconds = 0.0;
    LastEpochIndex = 0u;

    // Asegurar estado limpio por sesión de mundo/PIE: limpiar metas, storage y colas sucias
    Goals.GoalCells.Empty();
    Grid::Flow::ClearAll();
    Grid::Flow::GDirtyTiles.Reset();
    {
        FIntPoint Tmp; while (Grid::Flow::GDirtyQueue.Dequeue(Tmp)) {}
    }

    // Registrar ticker en el core ticker (thread-safe). Tick cada frame, nosotros gateamos por epoch.
    FTickerDelegate TickDelegate = FTickerDelegate::CreateUObject(this, &UGridEpochSubsystem::TickInternal);
    TickerHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
}

void UGridEpochSubsystem::Deinitialize()
{
    if (TickerHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
        TickerHandle.Reset();
    }

    // Limpieza final para evitar persistencias entre mundos/PIE
    Goals.GoalCells.Empty();
    Grid::Flow::ClearAll();
    Grid::Flow::GDirtyTiles.Reset();
    {
        FIntPoint Tmp; while (Grid::Flow::GDirtyQueue.Dequeue(Tmp)) {}
    }

    Super::Deinitialize();
}

bool UGridEpochSubsystem::TickInternal(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return true; // Seguir intentando en siguientes frames
    }

    const double CurrTimeSeconds = World->GetTimeSeconds();

    // Primera pasada: inicializar índices
    if (LastTimeSeconds <= 0.0)
    {
        LastTimeSeconds = CurrTimeSeconds;
        LastEpochIndex = GridEpoch::EpochIndexFromTimeSeconds(CurrTimeSeconds);
        return true;
    }

    // Gate por epoch: solo ejecutar si avanzó
    if (GridEpoch::HasEpochAdvanced(LastTimeSeconds, CurrTimeSeconds))
    {
        // Ejecutar solo si está habilitado el pipeline viejo (por defecto deshabilitado)
        if (CVar_Grid_EnableLegacyEpoch.GetValueOnGameThread() != 0)
        {
            // Decaimiento de Heat/Density por epoch
            Grid::Density::Decay(static_cast<float>(CurrTimeSeconds - LastTimeSeconds));

            // Ejecutar trabajo de rebuild con presupuesto actual
            Grid::Flow::RebuildStep(Budget, Goals, SolverParams);
        }

        // Actualizar estado
        LastTimeSeconds = CurrTimeSeconds;
        LastEpochIndex = GridEpoch::EpochIndexFromTimeSeconds(CurrTimeSeconds);
    }

    return true; // Mantener el ticker activo
}

void UGridEpochSubsystem::Tick(float DeltaSeconds)
{
	// No usado directamente; el tick real va por FTSTicker para tener control fino.
	// Se deja por si en el futuro se decide enrutar por aquí.
}

void UGridEpochSubsystem::RequestDirtyRebuildAll()
{
	// TODO: implementar escaneo de tiles del mundo y marcarlos sucios.
	// Ejemplo (pseudo): Grid::Flow::MarkTilesDirty(MinTile, MaxTile);
}
