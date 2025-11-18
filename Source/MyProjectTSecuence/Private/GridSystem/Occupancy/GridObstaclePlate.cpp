#include "GridSystem/Occupancy/GridObstaclePlate.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/CollisionProfile.h"

AGridObstaclePlate::AGridObstaclePlate()
{
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = Mesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Mesh->SetStaticMesh(CubeMesh.Object);
    }

    Mesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 0.1f));
    // No collision or overlaps; visualization only, avoidance is handled by flowfield logic
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Mesh->SetGenerateOverlapEvents(false);
}
