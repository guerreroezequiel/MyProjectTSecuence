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
├── Layers/
│  ├── OccupancyGrid.h/.cpp        # walkable/blocked (+events de cambio)
│  ├── CapacityGrid.h/.cpp         # capacidad por celda (hard/soft)
│  └── DensityHeatGrid.h/.cpp
└── FlowField/
   ├── FlowFieldStorage.h/.cpp     # SOA: dist[], dir[]; acceso ReadDir(CellId)
   ├── FlowFieldSolver.h/.cpp      # BFS/Dijkstra multi-fuente + softmax/gradiente
   └── FlowFieldRebuilder.h/.cpp   # cola mínima + presupuesto simple por frame
```

---

## Consola mínima
- `grid.flow.rebuild_now`
- `grid.heat.inject x y v`
- `grid.debug.flow on|off`
- `grid.debug.heat on|off`

---

## Checklist
- Inicializar `OccupancyGrid` y (opcional) `CapacityGrid`.
- `DensityHeatGrid`: acumular y decaer Heat.
- `FlowFieldStorage` + `FlowFieldRebuilder`:
  - Resolver `dist` (BFS/Dijkstra) a 1 m.
  - Calcular `dir` continua (softmax/gradiente).
  - Versionar por tile. Rebuild parcial por dirty.
- Agentes: leer `dir(i)` y mover `v = v_max·dir(i)` sin A*.
- Debug overlays: `dist` (cost field), `dir` (flow), `Heat`.

---
