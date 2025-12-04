#include "GridSystem/FlowField/UFlowFieldMovementComponent.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/Core/GridEpochSubsystem.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UFlowFieldMovementComponent::UFlowFieldMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    MovementSpeed = 25.0f;
    GoalThreshold = 50.0f;
    bStopAtGoal = true;
    bShowDebugDirection = false;
    DebugLineColor = FLinearColor::Red;
}

void UFlowFieldMovementComponent::BeginPlay()
{
    Super::BeginPlay();
    
    LastWorldPosition = GetOwner()->GetActorLocation();
    LastDirectionCheckTime = 0.0;
    CachedFlowDirection = FVector2D::ZeroVector;
}

void UFlowFieldMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetWorld())
    {
        return;
    }

    // Si hemos llegado a la meta y debemos detenernos, no hacer nada
    if (bStopAtGoal && HasReachedGoal())
    {
        return;
    }

    // Aplicar movimiento basado en FlowField
    ApplyFlowMovement(DeltaTime);

    // Debug visual si está habilitado
    if (bShowDebugDirection)
    {
        DrawDebugInfo();
    }
}

FIntPoint UFlowFieldMovementComponent::GetCellCoordinate(const FVector& WorldPosition) const
{
    return GridWorld::WorldToCellXY(WorldPosition);
}

FVector2D UFlowFieldMovementComponent::GetFlowDirectionAtPosition(const FVector& WorldPosition) const
{
    const double CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Cache para evitar consultas repetidas
    if (CurrentTime - LastDirectionCheckTime < DIRECTION_CACHE_DURATION)
    {
        const float DistanceMoved = FVector::Dist(WorldPosition, LastWorldPosition);
        if (DistanceMoved < 10.0f) // Si no nos hemos movido mucho, usar cache
        {
            return CachedFlowDirection;
        }
    }

    // Actualizar cache
    LastWorldPosition = WorldPosition;
    LastDirectionCheckTime = CurrentTime;

    // Obtener coordenada de celda
    const FIntPoint CellCoord = GetCellCoordinate(WorldPosition);
    
    // Leer dirección del FlowField
    CachedFlowDirection = Grid::Flow::ReadDir(CellCoord);
    
    return CachedFlowDirection;
}

void UFlowFieldMovementComponent::ApplyFlowMovement(float DeltaTime)
{
    if (!GetOwner())
    {
        return;
    }

    // Obtener posición actual
    const FVector CurrentLocation = GetOwner()->GetActorLocation();
    
    // Obtener dirección del FlowField
    const FVector2D FlowDirection2D = GetFlowDirectionAtPosition(CurrentLocation);
    
    // Si no hay dirección válida, no mover
    if (FlowDirection2D.IsNearlyZero())
    {
        return;
    }

    // Convertir a 3D (Grid X→World X, Grid Y→World Y, Z=altura)
    const FVector FlowDirection3D = FVector(FlowDirection2D.X, FlowDirection2D.Y, 0.0f);
    
    // Calcular desplazamiento
    const FVector MovementDelta = FlowDirection3D.GetSafeNormal() * MovementSpeed * DeltaTime;
    
    // Aplicar movimiento sin sweep: la evitación es lógica por flowfield
    const FVector NewLocation = CurrentLocation + MovementDelta;
    GetOwner()->SetActorLocation(NewLocation, false);
}

void UFlowFieldMovementComponent::DrawDebugInfo() const
{
    if (!GetOwner() || !GetWorld())
    {
        return;
    }

    const FVector CurrentLocation = GetOwner()->GetActorLocation();
    const FVector2D FlowDirection2D = GetFlowDirectionAtPosition(CurrentLocation);
    
    if (!FlowDirection2D.IsNearlyZero())
    {
        const FVector FlowDirection3D = FVector(FlowDirection2D.X, FlowDirection2D.Y, 0.0f);
        const FVector EndPoint = CurrentLocation + (FlowDirection3D.GetSafeNormal() * 100.0f);
        
        // Dibujar línea de dirección
        DrawDebugLine(
            GetWorld(),
            CurrentLocation,
            EndPoint,
            DebugLineColor.ToFColor(true), // Convertir FLinearColor a FColor
            false,
            0.1f, // Duración: 100ms
            0,
            2.0f  // Grosor de línea
        );

        // Dibujar punto en el extremo
        DrawDebugSphere(
            GetWorld(),
            EndPoint,
            5.0f,
            8,
            DebugLineColor.ToFColor(true), // Convertir FLinearColor a FColor
            false,
            0.1f
        );
    }
}

bool UFlowFieldMovementComponent::HasReachedGoal() const
{
    if (!GetOwner())
    {
        return false;
    }

    // Obtener el subsistema para acceder a las metas
    UWorld* World = GetOwner()->GetWorld();
    if (!World)
    {
        return false;
    }

    UGridEpochSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridEpochSubsystem>();
    if (!Subsystem)
    {
        return false;
    }

    // Si no hay metas, no hemos llegado a ninguna
    if (Subsystem->Goals.GoalCells.Num() == 0)
    {
        return false;
    }

    // Obtener posición actual
    const FVector CurrentLocation = GetOwner()->GetActorLocation();
    
    // Verificar distancia a cada meta
    for (const FIntPoint& GoalCell : Subsystem->Goals.GoalCells)
    {
        const FVector2D GoalWorldPosition2D = GridWorld::CellToWorldCenterXY(GoalCell);
        const FVector GoalWorldPosition = FVector(GoalWorldPosition2D.X, 0.0f, GoalWorldPosition2D.Y);
        const float Distance = FVector::Dist(CurrentLocation, GoalWorldPosition);
        
        if (Distance <= GoalThreshold)
        {
            return true;
        }
    }

    return false;
}
