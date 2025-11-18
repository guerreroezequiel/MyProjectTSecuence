#include "GridSystem/FlowField/GridFlowArrowActor.h"
#include "Components/ArrowComponent.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"

AGridFlowArrowActor::AGridFlowArrowActor()
{
    PrimaryActorTick.bCanEverTick = true;

    Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
    RootComponent = Arrow;
    Arrow->ArrowSize = 1.0f;
    Arrow->SetHiddenInGame(false);
}

void AGridFlowArrowActor::BeginPlay()
{
    Super::BeginPlay();
    ApplyArrowLength();
    UpdateTransformFromFlow();
}

void AGridFlowArrowActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateTransformFromFlow();
}

void AGridFlowArrowActor::SetCell(const FIntPoint& InCell)
{
    Cell = InCell;
    UpdateTransformFromFlow();
}

void AGridFlowArrowActor::UpdateTransformFromFlow()
{
    const FVector2D Center2D = GridWorld::CellToWorldCenterXY(Cell);
    const FVector WorldPos(Center2D.X, Center2D.Y, ArrowHeightUU);
    SetActorLocation(WorldPos);

    const FVector2D Dir = Grid::Flow::ReadDir(Cell);
    float YawDeg = 0.0f;
    if (!Dir.IsNearlyZero())
    {
        YawDeg = FMath::RadiansToDegrees(FMath::Atan2(Dir.Y, Dir.X));
    }
    SetActorRotation(FRotator(0.0f, YawDeg, 0.0f));
}

void AGridFlowArrowActor::ApplyArrowLength()
{
    // UArrowComponent no usa una longitud en UU directa, pero podemos escalar el componente para aproximar.
    // ArrowSize escala el gizmo; para tener 50uu de largo visual, usaremos un factor relativo simple.
    // Mantener ArrowSize=1 y usar escala X proporcional a ArrowLengthUU.
    const float ScaleX = ArrowLengthUU / 50.0f; // 50uu como base -> ScaleX=1 da 50uu aprox
    Arrow->SetWorldScale3D(FVector(ScaleX, 1.0f, 1.0f));
}
