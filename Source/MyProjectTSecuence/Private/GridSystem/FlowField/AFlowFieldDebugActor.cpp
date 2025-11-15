#include "GridSystem/FlowField/AFlowFieldDebugActor.h"
#include "GridSystem/FlowField/UFlowFieldMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AFlowFieldDebugActor::AFlowFieldDebugActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crear componentes
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    MovementComponent = CreateDefaultSubobject<UFlowFieldMovementComponent>(TEXT("MovementComponent"));
    // No necesita SetupAttachment - es un ActorComponent
    // MovementComponent->SetupAttachment(RootComponent);

    // Configurar mesh
    SetupMesh();
    
    // Configurar componente de movimiento
    SetupMovementComponent();
}

void AFlowFieldDebugActor::BeginPlay()
{
    Super::BeginPlay();
}

void AFlowFieldDebugActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AFlowFieldDebugActor::SetupMesh()
{
    // Buscar la esfera por defecto del engine
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(
        TEXT("/Engine/BasicShapes/Sphere")
    );
    
    if (SphereMeshAsset.Succeeded())
    {
        MeshComponent->SetStaticMesh(SphereMeshAsset.Object);
        
        // Escalar a 25cm de diámetro (la esfera por defecto es 100cm)
        MeshComponent->SetRelativeScale3D(FVector(0.25f, 0.25f, 0.25f));
        
        // Configurar colisión básica
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        MeshComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
        MeshComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
        
        // Material básico
        UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicMaterials/BasicMaterial"));
        if (BaseMaterial)
        {
            UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DebugColor);
            MeshComponent->SetMaterial(0, DynamicMaterial);
        }
    }
}

void AFlowFieldDebugActor::SetupMovementComponent()
{
    if (MovementComponent)
    {
        MovementComponent->MovementSpeed = MovementSpeed;
        MovementComponent->bAutoActivate = true;
    }
}
