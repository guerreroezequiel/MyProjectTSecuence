#include "GridSystem/Debug/GridDebugSubsystem.h"
#include "Engine/World.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "Misc/App.h"  // Para FApp::GetCurrentTime()
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"


UGridDebugSubsystem* UGridDebugSubsystem::GetGridDebugSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    UWorld* World = WorldContextObject->GetWorld();
    if (!World)
    {
        return nullptr;
    }

    return World->GetSubsystem<UGridDebugSubsystem>();
}

void UGridDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Inicializar dependencias
    Collection.InitializeDependency<UGridEpochSubsystem>();
    
    // Inicializar estado
    bIsDebugActive = false;
    bIsPaused = false;
    bStepRequested = false;
    
    // Configurar el ticker
    FTickerDelegate TickDelegate = FTickerDelegate::CreateUObject(this, &UGridDebugSubsystem::OnEpochTick);
    EpochTickHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
    
    UE_LOG(LogTemp, Log, TEXT("GridDebugSubsystem inicializado"));
}

void UGridDebugSubsystem::Deinitialize()
{
    // Limpiamos el manejador del tick si existe
    if (EpochTickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(EpochTickHandle);
        EpochTickHandle.Reset();
    }
    
    // Limpiamos el área de debug
    ClearDebugArea();
    
    Super::Deinitialize();
    UE_LOG(LogTemp, Log, TEXT("GridDebugSubsystem desinicializado"));
}

bool UGridDebugSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Solo crear el subsistema en el editor o en PIE
    return GIsEditor || GetWorld()->IsPlayInEditor();
}

void UGridDebugSubsystem::SetDebugMode(bool bEnable)
{
    if (bIsDebugActive == bEnable)
    {
        return; // Ya está en el estado solicitado
    }
    
    bIsDebugActive = bEnable;
    
    if (bIsDebugActive)
    {
        // Registramos el callback para el tick del epoch
        EpochTickHandle = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateUObject(this, &UGridDebugSubsystem::OnEpochTick)
        );
        
        UE_LOG(LogTemp, Log, TEXT("Modo de depuración de la grilla ACTIVADO"));
    }
    else
    {
        // Eliminamos el callback
        if (EpochTickHandle.IsValid())
        {
            FTSTicker::GetCoreTicker().RemoveTicker(EpochTickHandle);
            EpochTickHandle.Reset();
        }
        
        UE_LOG(LogTemp, Log, TEXT("Modo de depuración de la grilla DESACTIVADO"));
    }
}

void UGridDebugSubsystem::SetPauseState(bool bPause)
{
    if (bIsPaused == bPause)
    {
        return; // Ya está en el estado solicitado
    }
    
    bIsPaused = bPause;
    
    if (bIsPaused)
    {
        UE_LOG(LogTemp, Log, TEXT("Simulación de depuración PAUSADA"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Simulación de depuración REANUDADA"));
    }
}

void UGridDebugSubsystem::StepSimulation()
{
    if (bIsPaused)
    {
        bStepRequested = true;
        UE_LOG(LogTemp, Log, TEXT("Solicitado paso de simulación"));
    }
}

bool UGridDebugSubsystem::OnEpochTick(float DeltaTime)
{
    if (!bIsDebugActive)
    {
        return true; // Mantener el ticker activo
    }
    
    // Si está pausado, solo procesar si se solicitó un paso
    if (bIsPaused && !bStepRequested)
    {
        return true;
    }
    
    // Resetear bandera de paso
    bStepRequested = false;
    
    // Verificar tiempo transcurrido (200ms = 5Hz)
    static double LastTickTime = 0.0;
    double CurrentTime = FApp::GetCurrentTime();
    
    if ((CurrentTime - LastTickTime) * 1000.0 < 200.0)
    {
        return true;
    }
    
    LastTickTime = CurrentTime;
    
    // Lógica de depuración aquí
    static int32 TickCount = 0;
    UE_LOG(LogTemp, VeryVerbose, TEXT("Tick de depuración #%d"), ++TickCount);
    
    return true; // Mantener el ticker activo
}

void UGridDebugSubsystem::UpdateDebugArea(const FIntPoint& Center, int32 Radius, float CellSize, FLinearColor Color, float Duration)
{
    if (!GetWorld())
    {
        return;
    }

    // Calcular el tamaño total del área
    const float TotalSize = (Radius * 2 + 1) * CellSize;
    const FVector Extent(TotalSize * 0.5f, TotalSize * 0.5f, 10.0f);
    
    // Convertir la posición de la celda a posición en el mundo
    // Esto es un ejemplo - ajusta según cómo esté configurado tu sistema de grilla
    FVector WorldLocation = FVector(
        Center.X * CellSize,
        Center.Y * CellSize,
        50.0f // Altura por defecto
    );

    // Actualizar el área de debug actual
    CurrentDebugArea.CenterLocation = WorldLocation;
    CurrentDebugArea.Extent = Extent;
    CurrentDebugArea.Color = Color.ToFColor(true);
    CurrentDebugArea.Duration = Duration;
    bHasDebugArea = true;

    // Dibujar el área de debug
    DrawDebugBox(
        GetWorld(),
        WorldLocation,
        Extent,
        FQuat::Identity,
        CurrentDebugArea.Color,
        false,
        Duration,
        0,
        CurrentDebugArea.LineThickness
    );
}

void UGridDebugSubsystem::ClearDebugArea()
{
    if (bHasDebugArea)
    {
        // Limpiar cualquier debug draw existente
        FlushPersistentDebugLines(GetWorld());
        bHasDebugArea = false;
    }
}
