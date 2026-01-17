// Copyright 2024 MyProjectTSec

#include "ECS/Processors/FlowDirReadPlayersProcessor.h"
#include "MassExecutionContext.h"
#include "MassEntitySubsystem.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include "GridSystem/FlowField/FlowFieldTypes.h"
#include "ECS/Fragments/FlowReadFragment.h"
#include "ECS/Fragments/CellLocationFragment.h"
#include "ECS/Tags/ZombiTag.h"

UFlowDirReadPlayersProcessor::UFlowDirReadPlayersProcessor()
{
    bAutoRegisterWithProcessingPhases = true;
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
}

void UFlowDirReadPlayersProcessor::ConfigureQueries()
{
    EntityQuery.AddRequirement<FCellLocationFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FFlowReadFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddTagRequirement<FZombiTag>(EMassFragmentPresence::All);
    EntityQuery.RegisterWithProcessor(*this);
}

void UFlowDirReadPlayersProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    EntityQuery.ForEachEntityChunk(EntityManager, Context, [](FMassExecutionContext& Context)
    {
        const TConstArrayView<FCellLocationFragment> CellLocationFragments = Context.GetFragmentView<FCellLocationFragment>();
        const TArrayView<FFlowReadFragment> FlowReadFragments = Context.GetMutableFragmentView<FFlowReadFragment>();
        
        const int32 NumEntities = Context.GetNumEntities();
        for (int32 i = 0; i < NumEntities; ++i)
        {
            const FCellLocationFragment& CellLocation = CellLocationFragments[i];
            FFlowReadFragment& FlowRead = FlowReadFragments[i];
            
            // Inicializar como no válido por defecto
            FlowRead.bValid = false;
            
            if (CellLocation.bValid)
            {
                // Convertir de TileXY + CellIndex a coordenadas de celda globales
                // Primero obtenemos las coordenadas locales dentro del tile
                const int32 LocalX = CellLocation.CellIndex % GridConfig::TileDim;
                const int32 LocalY = CellLocation.CellIndex / GridConfig::TileDim;
                const FIntPoint LocalCellXY(LocalX, LocalY);
                
                // Convertir a coordenadas de celda globales
                const FIntPoint GlobalCellXY(
                    CellLocation.TileXY.X * GridConfig::TileDim + LocalCellXY.X,
                    CellLocation.TileXY.Y * GridConfig::TileDim + LocalCellXY.Y
                );
                
                // Obtener la dirección del flujo para la celda actual
                const FVector2D FlowDir = Grid::Flow::ReadDir(GlobalCellXY, EFlowIntent::Players);
                
                if (!FlowDir.IsNearlyZero())
                {
                    // Actualizar la dirección del flujo (normalizada)
                    FlowRead.DirWS = FVector(FlowDir.X, FlowDir.Y, 0.0f).GetSafeNormal();
                    
                    // Intentar obtener el epoch del campo de flujo
                    int32 StaticCostEpoch, GoalsEpoch;
                    Grid::Flow::GetBuiltMeta(CellLocation.TileXY, EFlowIntent::Players, StaticCostEpoch, GoalsEpoch);
                    
                    // Usar el máximo de los dos epochs como referencia
                    FlowRead.EpochSeen = FMath::Max(StaticCostEpoch, GoalsEpoch);
                    FlowRead.bValid = true;
                }
            }
        }
    });
}
