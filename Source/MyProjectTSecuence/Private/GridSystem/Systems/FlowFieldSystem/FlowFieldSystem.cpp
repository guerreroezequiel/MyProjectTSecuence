// Copyright 2024 MyProjectTSec

#include "GridSystem/Systems/FlowFieldSystem/FlowFieldSystem.h"
#include "GridSystem/TileContext/TileContext.h"
#include "GridSystem/FlowField/FlowField.h"

UFlowFieldSystem::UFlowFieldSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UFlowFieldSystem::BeginPlay()
{
    Super::BeginPlay();
    
    // Inicialización básica
    // TODO: Inicializar la grilla de tiles y flowfields
}

void UFlowFieldSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bAutoUpdate)
    {
        // Actualizar todos los tiles
        for (auto& Tile : Tiles)
        {
            if (Tile->IsDirty())
            Tile->UpdateEpochs();

            // Verificar y reconstruir FlowFields si es necesario
            for (int32 i = 0; i < static_cast<int32>(EFlowIntent::MAX); ++i)
            {
                EFlowIntent CurrentIntent = static_cast<EFlowIntent>(i);
                
                // Asegurarse de que existe el FlowField para este Intent
                if (!FlowFields.Contains(CurrentIntent))
                {
                    FlowFields.Add(CurrentIntent, MakeShared<FFlowField>());
                }

                // Verificar validez y reconstruir si es necesario
                if (!FlowFields[CurrentIntent]->IsValid(*Tile, CurrentIntent))
                {
                    RebuildFlowField(Tile, CurrentIntent);
                }
            }
        }
    }

    // Dibujar debug si está habilitado
    if (bDebugVisualization)
    {
        DrawDebugInfo();
    }
}

void UFlowFieldSystem::ToggleDebugVisualization(bool bShow)
{
    // TODO: Implementar lógica de visualización de debug
}

void UFlowFieldSystem::RebuildFlowField(const TSharedPtr<FTileContext>& Tile, EFlowIntent Intent)
{
    if (!Tile.IsValid() || !FlowFields.Contains(Intent))
    {
        return;
    }

    // Obtener el FlowField para este Intent
    TSharedPtr<FFlowField> FlowField = FlowFields[Intent];
    
    // Reconstruir el FlowField
    FlowField->Rebuild(*Tile, Intent);
    
    // Aquí podrías añadir lógica adicional después de reconstruir, como:
    // - Actualizar estadísticas
    // - Disparar eventos
    // - Actualizar visualización
    
    UE_LOG(LogTemp, Log, TEXT("FlowField reconstruido para Intent: %d"), static_cast<int32>(Intent));
}

void UFlowFieldSystem::DrawDebugInfo() const
{
    if (!bDebugVisualization)
    {
        return;
    }

    // TODO: Implementar visualización de debug
    // Mostrar epochs, dirty flags, etc.
    
    // Ejemplo de visualización básica
    for (const auto& Tile : Tiles)
    {
        if (Tile.IsValid())
        {
            for (const auto& FlowFieldPair : FlowFields)
            {
                EFlowIntent Intent = FlowFieldPair.Key;
                const TSharedPtr<FFlowField>& FlowField = FlowFieldPair.Value;
                
                if (FlowField.IsValid())
                {
                    FString DebugText = FString::Printf(TEXT("Intent: %d\n%s"), 
                        static_cast<int32>(Intent), 
                        *FlowField->GetDebugInfo());
                    
                    // Dibujar el texto de debug en pantalla
                    GEngine->AddOnScreenDebugMessage(-1, 0.f, FColor::White, DebugText);
                }
            }
        }
    }
}
