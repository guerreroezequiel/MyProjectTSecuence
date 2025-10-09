# Flow Field Grid – Core

## Objetivo
- Navegación masiva basada únicamente en Flow Field.
- Lectura O(1) por agente. Sin pathfinding individual.
- Capas mínimas: Occupancy, Distance (cost), Heat/Density.

---

## Base Grid
- **Resolución**: 1 m por celda (solver).
- **Vecindad**: 8 direcciones.
- **Tiling sugerido**: 64×64 celdas por tile (rebuild parcial por tiles dirty).
- **Subdivisión 2×2 (0.5 m)**: para movimiento/ocupación (micro‑avoid), no para solver.

---

## Capas esenciales
- **Occupancy (mínima)**
  - `Walkable / Blocked`. Opcional: `Capacity` (entero pequeño) para cuellos de botella.
- **Distance / Cost Field**
  - Distancia/costo‑to‑go multi‑fuente desde metas.
  - BFS (brushfire) si costo uniforme. Dijkstra si hay pesos.
- **Heat/Density**
  - Acumula tráfico reciente (accumulate + decay).
  - Se usa para inflar costos y evitar congestión.

---

## Solver (1 m) y Dirección continua
- **Fuentes**: metas G = {g1, g2, ...} con dist=0 (multi‑fuente).
- **Costo efectivo** al expandir vecino n:
  - `next = curr + moveCost(n) + α·Heat[n] + β·CapacityPressure[n]`
- **Algoritmo**
  - Dijkstra (min‑heap) o BFS si α=β=0 y costos uniformes.
  - Escribe `dist[i]` (campo escalar) por celda.
- **Dirección continua (salida del rebuild)**
  - Softmax 8‑dir:
    - `Δk = dist[k] − dist[i]`
    - `w_k = exp(−Δk / T)`; `p_k = w_k / Σ_j w_j`
    - `dir(i) = Σ_k p_k · normalize(vec(i→k))`
  - Gradiente 3×3 (opcional):
    - `gx = D(x+1,y) − D(x−1,y)`, `gy = D(x,y+1) − D(x,y−1)`
    - `dir = normalize(−[gx, gy])`
  - Estabilidad:
    - `dir = lerp(dir_prev, dir_new, γ)` (histéresis ligera)

---

## Movimiento y sub‑slots (2×2)
- Cada agente tiene un sub‑slot local s en su celda.
- Dirección por sub‑slot: `dir_s = normalize( lerp(dir_cell, dir_neighbor, λ(s)) )`.
- Micro‑avoid: jitter leve de posición dentro del sub‑slot y restricciones si `Capacity` saturada.

---

## Rebuild parcial
- **Dirty marking**: metas cambiadas, ΔHeat > umbral, cambios de Occupancy/Capacity.
- **Presupuesto**: token‑bucket con N tiles y M celdas por frame.
- **Prioridad**: tiles cercanos a cámara o con Heat alto.
- **Versionado**: escrituras versionadas por tile; lecturas O(1) coherentes.

---

## Parámetros (tunables)
- `T` (temperatura softmax): 0.1–0.25
- `α` (inflación por Heat): 0.2–0.6
- `β` (presión por capacidad): 0.2–0.6
- `H_th` (umbral dirty por Heat): dependiente del decay/escala
- `Budget` rebuild: N tiles, M celdas por frame

---

## API mínima (C++)
- `Source/MyProjectTSecuence/GridSystem/FlowField/`
  - `FlowFieldStorage.h/.cpp`: SOA dist[], dir[], version[], tileVersion[]
  - `FlowFieldSolver.h/.cpp`: BFS/Dijkstra multi‑fuente; hooks `α, β, T`
  - `FlowFieldRebuilder.h/.cpp`: cola de dirty; token‑bucket; escribe `dist` y `dir`
- `Source/MyProjectTSecuence/GridSystem/Density/`
  - `DensityHeatGrid.h/.cpp`: `AccumulateTraffic`, `Decay`, `GetHeat`
- `Source/MyProjectTSecuence/GridSystem/Occupancy/`
  - `OccupancyGrid.h/.cpp`: `IsWalkable`
  - `CapacityGrid.h/.cpp`: `GetCapacity`, `CapacityMode{HardOnly, SoftAdaptive}`

---
## Estructura de carpetas (propuesta mínima)

```text
Source/MyProjectTSecuence/GridSystem/
├── Core/
│  ├── GridTypes.h                 # IDs, dirs (8-neigh), structs básicos
│  ├── GridConfig.h/.cpp           # UDeveloperSettings: tamaños, T, α, β, budgets
│  └── GridMath.h                  # world↔cell, vecinos, costos ort/diag, clamp/lerp
├── Occupancy/
│  ├── OccupancyGrid.h/.cpp        # walkable/blocked (+events de cambio)
│  └── CapacityGrid.h/.cpp         # capacidad por celda (hard/soft)
├── Density/
│  └── DensityHeatGrid.h/.cpp      # heat SOA por tile (Accumulate/Decay/Get)
└── FlowField/
   ├── FlowFieldStorage.h/.cpp     # SOA: dist[], dir[]; acceso ReadDir(CellId)
   ├── FlowFieldSolver.h/.cpp      # BFS/Dijkstra multi-fuente + softmax/gradiente
   └── FlowFieldRebuilder.h/.cpp   # cola mínima + presupuesto simple por frame
```

---

## Consola mínima
- `grid.flow.set_goal x y` — define una meta en coordenadas de celda.
- `grid.flow.mark_tile tx ty` — marca un tile como dirty.
- `grid.flow.rebuild_step [tiles] [cells]` — ejecuta un paso con presupuesto.
- `grid.flow.rebuild_now` — consume toda la cola de tiles dirty.
- `grid.flow.clear_all` — limpia storage y colas.
- `grid.heat.inject x y v` — inyecta Heat en una celda.
- `grid.heat.decay dt` — aplica decay global.
- `grid.heat.clear` — limpia todo el Heat.
 - `grid.occ.set x y state` — setea estado de celda (0=Empty,1=Obstacle,2=Portal).
 - `grid.occ.clear` — limpia toda la capa de occupancy.

Pendiente (debug/visualización):
- `grid.debug.flow on|off`
- `grid.debug.heat on|off`

---

## Checklist (estado y próximos pasos)
- [hecho] `FlowFieldStorage` (SOA por tile: `dist[]`, `dir[]`, `tileVersion[]`).
- [hecho] `FlowFieldRebuilder` (cola de tiles dirty; presupuesto por frame; integración solver por tile).
- [hecho] Consola mínima (`grid.flow.*`, `grid.heat.*`).
- [hecho] `DensityHeatGrid` con almacenamiento real (SOA por tile) y comandos de consola.
- [hecho] Solver real por tile: Dijkstra multi‑fuente 8‑dir con costo `moveCost + α·Heat + β·CapacityPressure(placeholder)` y dirección softmax.
- [hecho] Base `OccupancyGrid` con almacenamiento por tile (walkable/blocked/portal) y API `IsBlocked/IsPortal/Get/Set`.

Próximos pasos inmediatos (debug visual y authoring):
- [alto] Overlays de debug:
  - `grid.debug.flow on|off`: dibujar `dir` y/o `dist` por celda/tile.
  - `grid.debug.heat on|off`: dibujar `Heat` (colormap) por celda.
- [alto] Authoring/edición rápida de Occupancy desde consola:
  - `grid.occ.set x y state` (0=Empty,1=Obstacle,2=Portal)
  - `grid.occ.clear`
- [medio] Exponer parámetros del solver por consola: `grid.flow.set_params T α β`.
- [medio] Presupuesto por celdas: respetar `MaxCellsPerFrame` en `RebuildStep` (pausar/continuar entre frames).
- [medio] Integrar presión por capacidad real en costo (`CapacityGrid`): `β·CapacityPressure`.
- [bajo] Tunables en `UDeveloperSettings`/CVars (T, α, β, budgets) y pequeñas mejoras de estabilidad (histéresis de dirección).

---
