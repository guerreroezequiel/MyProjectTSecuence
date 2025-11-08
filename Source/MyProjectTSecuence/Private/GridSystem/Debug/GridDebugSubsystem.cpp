#include "GridSystem/Debug/GridDebugSubsystem.h"
#include "Engine/World.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "Misc/App.h"  // Para FApp::GetCurrentTime()
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"

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
    // Limpiar el ticker al destruir el subsistema
    if (EpochTickHandle.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(EpochTickHandle);
    }
    
    // Limpiar el área de debug
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
    if (bIsDebugActive != bEnable)
    {
        bIsDebugActive = bEnable;
        
        if (bEnable)
        {
            // Inicializar el modo de depuración
            bIsPaused = false;
            bStepRequested = false;
            
            // Notificar a los suscriptores
            OnDebugModeChanged.Broadcast(true);
            
            UE_LOG(LogTemp, Log, TEXT("Modo de depuración ACTIVADO"));
        }
        else
        {
            // Limpiar el área de depuración
            ClearDebugArea();
            
            // Notificar a los suscriptores
            OnDebugModeChanged.Broadcast(false);
            
            UE_LOG(LogTemp, Log, TEXT("Modo de depuración DESACTIVADO"));
        }
    }
}

void UGridDebugSubsystem::SetPauseState(bool bPause)
{
    if (bIsPaused != bPause)
    {
        bIsPaused = bPause;
        
        if (bIsPaused)
        {
            UE_LOG(LogTemp, Log, TEXT("Simulación de depuración EN PAUSA"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Simulación de depuración REANUDADA"));
        }
        
        // Notificar a los suscriptores
        OnPauseStateChanged.Broadcast(bIsPaused);
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
    
    // Lógica de depuración
    if (bIsDebugActive && !bIsPaused)
    {
        // Aquí iría la lógica de actualización de la simulación
        static int32 TickCount = 0;
        UE_LOG(LogTemp, VeryVerbose, TEXT("Tick de depuración #%d"), ++TickCount);
    }
    
    return true; // Mantener el ticker activo
}

void UGridDebugSubsystem::UpdateDebugArea(const FIntPoint& Center, int32 Radius, float CellSize, FLinearColor Color, float Duration)
{
    if (!GetWorld() || !GetWorld()->IsGameWorld() || !bEnableDebugDrawing)
    {
        return;
    }
    
    // Validación de parámetros
    if (Radius <= 0 || CellSize <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("Parámetros inválidos: Radius=%d, CellSize=%.2f"), Radius, CellSize);
        return;
    }
    
    // Actualizar el área de depuración actual
    CurrentDebugArea.CenterLocation = FVector(Center.X * CellSize, Center.Y * CellSize, 0.0f);
    CurrentDebugArea.Extent = FVector(Radius * CellSize, Radius * CellSize, 10.0f);
    CurrentDebugArea.Color = Color.ToFColor(true);
    CurrentDebugArea.Duration = Duration;
    bHasDebugArea = true;
    
    // Notificar a los suscriptores
    OnDebugAreaUpdated.Broadcast(CurrentDebugArea);
    
    UE_LOG(LogTemp, Verbose, TEXT("Área de depuración actualizada: Center=(%d,%d), Radius=%d, CellSize=%.2f"), 
        Center.X, Center.Y, Radius, CellSize);
}

void UGridDebugSubsystem::ClearDebugArea()
{
    if (bHasDebugArea)
    {
        bHasDebugArea = false;
        
        // Notificar a los suscriptores
        OnDebugAreaCleared.Broadcast();
        
        UE_LOG(LogTemp, Verbose, TEXT("Área de depuración limpiada"));
    }
}

// Implementación de los métodos de plantilla

template<typename T>
void UGridDebugSubsystem::RegisterDebugSubscriber(T* Subscriber)
{
    if (Subscriber)
    {
        DebugSubscribers.AddUnique(Subscriber);
    }
}

template<typename T>
void UGridDebugSubsystem::UnregisterDebugSubscriber(T* Subscriber)
{
    if (Subscriber)
    {
        DebugSubscribers.Remove(Subscriber);
    }
}

// Especializaciones para UObject
template MYPROJECTTSECUENCE_API void UGridDebugSubsystem::RegisterDebugSubscriber<UObject>(UObject* Subscriber);
template MYPROJECTTSECUENCE_API void UGridDebugSubsystem::UnregisterDebugSubscriber<UObject>(UObject* Subscriber);

void UGridDebugSubsystem::UpdateAllDebugComponents(const FVector& CenterWorldLocation, float GridSize, int32 Radius)
{
    // Notificar a los suscriptores sobre la actualización del área de depuración
    if (bHasDebugArea)
    {
        // Actualizar la ubicación del área de depuración si es necesario
        CurrentDebugArea.CenterLocation = CenterWorldLocation;
        OnDebugAreaUpdated.Broadcast(CurrentDebugArea);
    }
}
