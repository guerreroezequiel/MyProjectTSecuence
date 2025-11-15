#include "GridSystem/FlowField/AFlowFieldDebugActor.h"
#include "GridSystem/FlowField/UFlowFieldMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

AFlowFieldDebugActor::AFlowFieldDebugActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Crear componentes
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    TextRenderComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TextRenderComponent"));
    TextRenderComponent->SetupAttachment(RootComponent);

    MovementComponent = CreateDefaultSubobject<UFlowFieldMovementComponent>(TEXT("MovementComponent"));
    // No necesita SetupAttachment - es un ActorComponent
    // MovementComponent->SetupAttachment(RootComponent);

    // Configurar mesh
    SetupMesh();
    
    // Configurar texto
    SetupTextRender();
    
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
    
    // Actualizar texto con coordenadas del mundo
    if (TextRenderComponent)
    {
        const FVector WorldLocation = GetActorLocation();
        const FString CoordinateText = FString::Printf(TEXT("X:%.0f Y:%.0f Z:%.0f"), 
            WorldLocation.X, WorldLocation.Y, WorldLocation.Z);
        TextRenderComponent->SetText(FText::FromString(CoordinateText));
    }
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
        
        // Escalar a 25cm de diámetro (la esfera por defecto es 100cm de radio = 200cm diámetro)
        // Para 25cm diámetro, necesitamos escalar por 0.125
        MeshComponent->SetRelativeScale3D(FVector(0.125f, 0.125f, 0.125f));
        
        // Ajustar posición para que el origen esté en la base de la esfera
        // La esfera por defecto tiene su origen en el centro, así que la elevamos 12.5cm
        MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 12.5f));
        
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

void AFlowFieldDebugActor::SetupTextRender()
{
    if (TextRenderComponent)
    {
        // Configurar propiedades básicas del texto
        TextRenderComponent->SetText(FText::FromString(TEXT("X:0 Y:0 Z:0")));
        TextRenderComponent->SetTextRenderColor(FColor::Green);
        TextRenderComponent->SetHorizontalAlignment(EHTA_Center);
        TextRenderComponent->SetVerticalAlignment(EVRTA_TextCenter);
        
        // Escalar el texto para que sea visible
        TextRenderComponent->SetWorldSize(120.0f);
        
        // Posicionar el texto arriba de la esfera
        TextRenderComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
        
        // Configurar fuente - usar fuente por defecto
        // TextRenderComponent->SetFont(LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto")));
        
        // El texto se mostrará con configuración básica
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
