#include "GridSystem/Debug/GridDebugSubsystem.h"
#include "Engine/World.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "Misc/App.h"  // Para FApp::GetCurrentTime()
#include "DrawDebugHelpers.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/Debug/WidgetGridDebugDrawComponent.h"

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
    
    // Limpiar componentes de widget registrados
    WidgetDebugComponents.Empty();
    
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
        UE_LOG(LogTemp, Warning, TEXT("No hay mundo válido para dibujar el área de debug"));
        return;
    }

    // 1. Calcular el tamaño total del área en celdas
    const int32 GridSize = (Radius * 2) + 1;
    const float TotalSize = GridSize * CellSize;
    
    FVector WorldLocation;
    
    // 2. Obtener la posición en el mundo
    if (bUseGridWorldOrigin)
    {
        // Usar el sistema de coordenadas de GridWorld
        const FVector2D TileOrigin2D = GridWorld::TileToWorldOriginXY(Center);
        WorldLocation = FVector(
            TileOrigin2D.X + (TotalSize * 0.5f) - (CellSize * 0.5f),
            TileOrigin2D.Y + (TotalSize * 0.5f) - (CellSize * 0.5f),
            GridWorld::GetOriginWS().Z  // Usar la misma altura que el grid
        );
        UE_LOG(LogTemp, Log, TEXT("Actualizando área de debug en posición GridWorld: %s"), *WorldLocation.ToString());
    }
    else
    {
        // Sistema de coordenadas simple (backup)
        WorldLocation = FVector(
            Center.X * CellSize,
            Center.Y * CellSize,
            50.0f
        );
        UE_LOG(LogTemp, Log, TEXT("Actualizando área de debug en posición simple: %s"), *WorldLocation.ToString());
    }

    // 3. Calcular la extensión (mitad del tamaño total)
    const FVector Extent(TotalSize * 0.5f, TotalSize * 0.5f, 10.0f);

    // 4. Actualizar el área de debug actual
    CurrentDebugArea.CenterLocation = WorldLocation;
    CurrentDebugArea.Extent = Extent;
    CurrentDebugArea.Color = Color.ToFColor(true);
    CurrentDebugArea.Duration = Duration;
    CurrentDebugArea.LineThickness = 2.0f; // Grosor de línea más visible
    bHasDebugArea = true;

    // 5. Dibujar el área de debug
    DrawDebugBox(
        GetWorld(),
        WorldLocation,
        Extent,
        FQuat::Identity,
        CurrentDebugArea.Color,
        false,  // Persistent
        Duration > 0 ? Duration : -1.0f,  // Si es 0, usar -1 para que sea permanente
        0,      // Depth priority
        CurrentDebugArea.LineThickness
    );

    // 6. Dibujar un punto en el centro para referencia
    DrawDebugPoint(
        GetWorld(),
        WorldLocation,
        10.0f,  // Tamaño
        FColor::Red,  // Rojo para mayor visibilidad
        false,  // Persistent
        Duration > 0 ? Duration : -1.0f
    );
    
    // 7. Dibujar líneas desde el centro a los bordes para mejor visibilidad
    const FVector Right = WorldLocation + FVector(Extent.X, 0, 0);
    const FVector Left = WorldLocation - FVector(Extent.X, 0, 0);
    const FVector Forward = WorldLocation + FVector(0, Extent.Y, 0);
    const FVector Backward = WorldLocation - FVector(0, Extent.Y, 0);
    
    DrawDebugLine(GetWorld(), Left, Right, FColor::Yellow, false, Duration > 0 ? Duration : -1.0f, 0, 1.0f);
    DrawDebugLine(GetWorld(), Backward, Forward, FColor::Yellow, false, Duration > 0 ? Duration : -1.0f, 0, 1.0f);
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

void UGridDebugSubsystem::RegisterWidgetDebugComponent(UWidgetGridDebugDrawComponent* Component)
{
    if (Component && !WidgetDebugComponents.Contains(Component))
    {
        WidgetDebugComponents.Add(Component);
        UE_LOG(LogTemp, Log, TEXT("Widget debug component registered. Total: %d"), WidgetDebugComponents.Num());
    }
}

void UGridDebugSubsystem::UnregisterWidgetDebugComponent(UWidgetGridDebugDrawComponent* Component)
{
    if (Component)
    {
        WidgetDebugComponents.Remove(Component);
        UE_LOG(LogTemp, Log, TEXT("Widget debug component unregistered. Remaining: %d"), WidgetDebugComponents.Num());
    }
}

void UGridDebugSubsystem::UpdateAllDebugComponents()
{
    // Actualizar componentes de widget
    for (const TWeakObjectPtr<UWidgetGridDebugDrawComponent>& Component : WidgetDebugComponents)
    {
        if (Component.IsValid())
        {
            // No need to call anything here as the widget components handle their own updates
            // through the UpdateDebugArea method when needed
        }
    }
}

void UGridDebugSubsystem::UpdateWidgetDebugArea(const FVector& CenterWorldLocation, float GridSize, int32 Radius)
{
    UE_LOG(LogTemp, Log, TEXT("GridDebugSubsystem: UpdateWidgetDebugArea - Location: %s, GridSize: %.2f, Radius: %d, NumWidgetComponents: %d"), 
        *CenterWorldLocation.ToString(), GridSize, Radius, WidgetDebugComponents.Num());
    
    bool bAnyWidgetUpdated = false;
    
    // Update all widget debug components
    for (TWeakObjectPtr<UWidgetGridDebugDrawComponent> Component : WidgetDebugComponents)
    {
        if (Component.IsValid())
        {
            Component->UpdateDebugArea(CenterWorldLocation, GridSize, Radius);
            bAnyWidgetUpdated = true;
        }
    }
    
    if (!bAnyWidgetUpdated)
    {
        UE_LOG(LogTemp, Warning, TEXT("GridDebugSubsystem: No valid widget debug components found to update"));
    }
}

void UGridDebugSubsystem::ClearWidgetDebugAreas()
{
    for (TWeakObjectPtr<UWidgetGridDebugDrawComponent> Component : WidgetDebugComponents)
    {
        if (Component.IsValid())
        {
            Component->ClearDebugArea();
        }
    }
}

void UGridDebugSubsystem::UpdateAllDebugComponents(const FVector& CenterWorldLocation, float GridSize, int32 Radius)
{
    // This is just a wrapper that calls UpdateWidgetDebugArea
    // to maintain backward compatibility
    UpdateWidgetDebugArea(CenterWorldLocation, GridSize, Radius);
}
