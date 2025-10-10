#pragma once

#include "CoreMinimal.h"
#include "GridSystem/Core/GridConfig.h"
#include "GridSystem/Core/GridTypes.h"
#include "GridSystem/Core/GridMath.h"
#include "GridSystem/Occupancy/OccupancyGrid.h"
#include "GridSystem/Occupancy/CapacityGrid.h"
#include "GridSystem/Density/DensityHeatGrid.h"
#include "GridSystem/Core/GridWorld.h"
#include "GridSystem/FlowField/FlowFieldStorage.h"
#include <limits>

// FlowFieldSolver: interfaz de solver (BFS/Dijkstra multi-fuente) + dirección continua.
// Referencia: FLOWFIELD_CORE.md
// Header-only stub (sin almacenamiento). Implementación real vendrá luego.

namespace Grid
{
	namespace Flow
	{
		struct FSolverParams
		{
			// Temperatura para softmax de dirección
			float T = 0.2f;
			// Inflación por Heat (α) y por presión de capacidad (β)
			float AlphaHeat = 0.4f;
			float BetaCapacity = 0.4f;
			// Si costos uniformes y Alpha/Beta ~0, usar BFS; si no, Dijkstra.
		};

		// Define un conjunto de metas (celdas con dist = 0)
		struct FGoalSet
		{
			TArray<FIntPoint> GoalCells; // multi-fuente
		};

		// Helpers para metas
		FORCEINLINE void ClearGoals(FGoalSet& Goals)
		{
			Goals.GoalCells.Reset();
		}

		FORCEINLINE void AddGoalCell(FGoalSet& Goals, const FIntPoint& CellXY)
		{
			Goals.GoalCells.Add(CellXY);
		}

		FORCEINLINE void AddGoalCells(FGoalSet& Goals, const TArray<FIntPoint>& Cells)
		{
			Goals.GoalCells.Append(Cells);
		}

		FORCEINLINE void AddGoalWorldPos(FGoalSet& Goals, const FVector& WorldPosUU)
		{
			Goals.GoalCells.Add(GridWorld::WorldToCellXY(WorldPosUU));
		}

		// Resultado del solve (placeholder)
		struct FSolveStats
		{
			int32 VisitedCells = 0;
			int32 ExpandedCells = 0;
			bool  UsedDijkstra = false;
		};

		// Calcula dirección continua para una celda a partir de 8 vecinos y deltas de dist.
		FORCEINLINE FVector2D ComputeDirFromNeighborhood(const float NeighborDelta[8], const FSolverParams& Params)
		{
			float W[8];
			GridMath::Softmax8(NeighborDelta, Params.T, W);
			FVector2D v(0,0);
			for (int i = 0; i < 8; ++i)
			{
				v += W[i] * Grid::Neigh8Dir[i];
			}
			const float lenSq = v.SizeSquared();
			return (lenSq > KINDA_SMALL_NUMBER) ? (v / FMath::Sqrt(lenSq)) : FVector2D::ZeroVector;
		}

		// Solver simple por tile: distancia euclidiana hacia la meta más cercana (ignora obstáculos y heat)
		// Escribe dist/dir en storage para el rectángulo [MinCell..MaxCell] 
		FORCEINLINE FSolveStats SolveTileDistance(const FIntPoint& MinCell, const FIntPoint& MaxCell, const FGoalSet& Goals, const FSolverParams& Params)
		{
			FSolveStats Stats;
			if (Goals.GoalCells.Num() == 0) { return Stats; }

			// Precompute centros de metas en mundo
			TArray<FVector2D> GoalCenters;
			GoalCenters.Reserve(Goals.GoalCells.Num());
			for (const FIntPoint& g : Goals.GoalCells)
			{
				GoalCenters.Add(GridWorld::CellToWorldCenterXY(g));
			}

			for (int32 y = MinCell.Y; y <= MaxCell.Y; ++y)
			{
				for (int32 x = MinCell.X; x <= MaxCell.X; ++x)
				{
					const FIntPoint c(x, y);
					const FVector2D cCenter = GridWorld::CellToWorldCenterXY(c);
					// Distancia a la meta más cercana
					float bestDist = TNumericLimits<float>::Max();
					int32 bestIdx = -1;
					for (int32 gi = 0; gi < GoalCenters.Num(); ++gi)
					{
						const float d = FVector2D::Distance(cCenter, GoalCenters[gi]) / GridConfig::CellSizeUU; // en celdas
						if (d < bestDist) { bestDist = d; bestIdx = gi; }
					}
					WriteDist(c, bestDist);
					// Dir: hacia la meta más cercana
					FVector2D dir = FVector2D::ZeroVector;
					if (bestIdx >= 0 && bestDist > KINDA_SMALL_NUMBER)
					{
						const FVector2D toGoal = (GoalCenters[bestIdx] - cCenter);
						const float len = toGoal.Size();
						dir = (len > KINDA_SMALL_NUMBER) ? (toGoal / len) : FVector2D::ZeroVector;
					}
					WriteDir(c, dir);
					Stats.VisitedCells++;
				}
			}
			// Nota: UsedDijkstra falso (aún) en este solver simple
			return Stats;
		}

		// Dijkstra multi-fuente por tile con costos: moveCost + Alphaheat*heat + BetaCapacity*Pressure(placeholder)
		// Limita la expansión a [MinCell..MaxCell]. Si no hay metas dentro del tile, opcionalmente cae al solver euclidiano.
		FORCEINLINE FSolveStats SolveTileDijkstra(const FIntPoint& MinCell, const FIntPoint& MaxCell, const FGoalSet& Goals, const FSolverParams& Params)
		{
			FSolveStats Stats;
			const int32 TileDim = GridConfig::TileDim;
			const int32 N = TileDim * TileDim;
			TArray<float> Dist;
			Dist.Init(TNumericLimits<float>::Max(), N);

			auto InBounds = [&](const FIntPoint& c)->bool {
				return c.X >= MinCell.X && c.X <= MaxCell.X && c.Y >= MinCell.Y && c.Y <= MaxCell.Y;
			};
			auto LocalXY = [&](const FIntPoint& c)->FIntPoint {
				return GridWorld::LocalCellInTile(c);
			};
			auto LocalIdx = [&](const FIntPoint& c)->int32 {
				const FIntPoint l = LocalXY(c);
				return l.Y * TileDim + l.X;
			};

			// Open list (naive): vector y pop del mínimo a mano (suficiente para 64x64)
			struct Node { FIntPoint C; float D; };
			TArray<Node> Open;

			int32 Seeds = 0;
			for (const FIntPoint& g : Goals.GoalCells)
			{
				if (InBounds(g) && !Grid::IsBlocked(g))
				{
					const int32 gi = LocalIdx(g);
					Dist[gi] = 0.0f;
					Open.Add({ g, 0.0f });
					Seeds++;
				}
			}

			if (Seeds == 0)
			{
				// Sin seeds en el tile: fallback a euclidiano
				return SolveTileDistance(MinCell, MaxCell, Goals, Params);
			}

			while (Open.Num() > 0)
			{
				// Pop mínimo
				int32 bestIdx = 0; float bestD = Open[0].D;
				for (int32 i = 1; i < Open.Num(); ++i)
				{
					if (Open[i].D < bestD) { bestD = Open[i].D; bestIdx = i; }
				}
				const Node curr = Open[bestIdx];
				Open.RemoveAtSwap(bestIdx, 1, EAllowShrinking::No);

				Stats.ExpandedCells++;

				// Expandir 8 vecinos
				for (int k = 0; k < 8; ++k)
				{
					const FIntPoint n(curr.C.X + Grid::Neigh8[k].X, curr.C.Y + Grid::Neigh8[k].Y);
					if (!InBounds(n)) { continue; }
					if (Grid::IsBlocked(n)) { continue; }
					const int32 ni = LocalIdx(n);
					const float moveCost = GridMath::MoveCost8(k);
					const float heat = Grid::Density::GetHeat(n);
					const int32 baseCap = Grid::Capacity::GetBaseCapacity(n);
					const int32 currCnt = Grid::Capacity::GetCurrentCount(n);
					const float capInfl = Grid::Capacity::CostInflationFactor(currCnt, baseCap);
					const float stepCost = moveCost + Params.AlphaHeat * heat + Params.BetaCapacity * (capInfl - 1.0f);
					const float newCost = bestD + stepCost;
					if (newCost < Dist[ni])
					{
						Dist[ni] = newCost;
						Open.Add({ n, newCost });
					}
				}
			}

			// Escribir distancias y direcciones
			for (int32 y = MinCell.Y; y <= MaxCell.Y; ++y)
			{
				for (int32 x = MinCell.X; x <= MaxCell.X; ++x)
				{
					const FIntPoint c(x, y);
					const int32 ci = LocalIdx(c);
					const float dHere = Dist[ci];
					WriteDist(c, dHere);

					// Dirección usando softmax sobre deltas hacia vecinos
					float deltas[8];
					for (int k = 0; k < 8; ++k)
					{
						const FIntPoint n(c.X + Grid::Neigh8[k].X, c.Y + Grid::Neigh8[k].Y);
						if (!InBounds(n)) { deltas[k] = 1e6f; continue; }
						const float nd = Dist[LocalIdx(n)];
						deltas[k] = nd - dHere;
					}
					const FVector2D dir = ComputeDirFromNeighborhood(deltas, Params);
					WriteDir(c, dir);
					Stats.VisitedCells++;
				}
			}

			Stats.UsedDijkstra = true;
			return Stats;
		}

		// Placeholder de solver global (por ahora no utilizado)
		FORCEINLINE FSolveStats SolveGlobal(const FGoalSet& Goals, const FSolverParams& Params)
		{
			FSolveStats Stats; (void)Goals; (void)Params; return Stats;
		}
	}
}
