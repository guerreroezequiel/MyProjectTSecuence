# FlowField System — Contratos (Estructura MVP)

## Alcance
Este documento define los contratos formales del sistema FlowField usando la estructura mínima acordada:

- TileContext
- TileStaticData
- FlowFieldSystem
- FlowFieldSolver
- FlowFieldStorage

El objetivo es soportar bake de costos estáticos, rebuild por epochs y solver desacoplado, sin sobre-ingeniería.

---

## 0. GridConfig (fuente de verdad)
- `GridConfig::TileDim` define el tamaño del tile en celdas.
- Todos los buffers tile-local usan:
  - `NumCells = TileDim * TileDim`
- El indexing es **tile-space**, nunca grid-space.

---
## 1. Owner de tiles (TileRegistry)
Un sistema de nivel Grid (GridWorld / GridEpochSubsystem / TileRegistry) es el único responsable de:
- crear tiles
- definir tiles activos (HOT/WARM/COLD)
- setear TileXY en cada FTileContext
- exponer acceso estable a FTileContext por TileXY

## 2. TileContext.h / .cpp
**Rol:** Metadatos del tile (epochs y dirty flags).

### Responsabilidades
- Mantener epochs:
  - `StaticCostEpoch`
  - `GoalsEpoch(Intent)`
- Mantener dirty flags:
  - `StaticCostDirty`
  - `GoalsDirty[Intent]`

### Contrato
- `UpdateEpochs()`:
  - Si `StaticCostDirty` → incrementa `StaticCostEpoch` y limpia flag.
  - Si `GoalsDirty[Intent]` → incrementa `GoalsEpoch(Intent)` y limpia flag.
- No contiene buffers ni ejecuta bake o solver.

---

## 3. TileStaticData.h
**Rol:** Cache estático tile-local de costos preprocesados.

### Responsabilidades
- Almacenar:
  - `FinalCost_Static[NumCells]`
  - `BakedStaticCostEpoch`
- Proveer acceso tile-space:
  - `GetCost(LocalX, LocalY)`
  - `SetCost(LocalX, LocalY, Cost)`

### Indexing obligatorio
- `Index = LocalY * GridConfig::TileDim + LocalX`

### Semántica
- `INF_COST` indica celda no transitable.
- Costo base MVP: `1.0`.

### Invariante
Antes de ejecutar el solver:
- `BakedStaticCostEpoch == TileContext.StaticCostEpoch`

---

## 4. FlowFieldSystem.h / .cpp
**Rol:** Orquestador del lifecycle por tile e intent.

### Responsabilidades
- Mantener:
  - `TileContext` por tile
  - `TileStaticData` por tile
- Ejecutar bake de costos estáticos.
- Decidir cuándo reconstruir flowfields.

### Pipeline obligatorio por tile
1. `TileContext.UpdateEpochs()`
2. Si cambió `StaticCostEpoch`:
   - bake de `FinalCost_Static`
   - setear `TileStaticData.BakedStaticCostEpoch`
3. Para cada `EFlowIntent` activo:
   - evaluar `FlowField.IsValid(Context, Intent)`
   - si no es válido → ejecutar o encolar `Rebuild`

### Reglas
- Nunca se evalúa `IsValid()` con datos estáticos desactualizados.
- El bake ocurre **antes** de cualquier rebuild.
- La cola de rebuild (si existe) es responsabilidad exclusiva del System.

---

## 5. FlowFieldSolver.h / .cpp
**Rol:** Algoritmo de pathfinding / integración / direcciones.

### Inputs
- `TileStaticData` (tile-space)
- `Goals` del intent
- Parámetros del solver

### Reglas
- `FinalCost_Static == INF_COST` → celda no transitable.
- El solver no consulta Occupancy ni otras capas directamente.

### Outputs
- IntegrationField[NumCells]
- DirectionField[NumCells]

---

## 6. FlowFieldStorage.h / .cpp
**Rol:** Cache de resultados del solver.

### Responsabilidades
- Almacenar outputs por `(TileXY, Intent)`:
  - IntegrationField
  - DirectionField
- Exponer acceso de solo lectura para:
  - movement
  - debug
  - networking

### Reglas
- Es la fuente única de datos consumidos en runtime.
- Puede versionar datos para debug (epochs o hashes).

---

## 7. Dirty Marking
### Regla general
Cualquier cambio que afecte el costo estático:
1. Actualiza la capa correspondiente (ej. Occupancy).
2. Determina `TileXY`.
3. Marca `StaticCostDirty` en el `TileContext`.

### Bordes (MVP)
- Si una celda modificada está en el borde del tile:
  - se marca dirty el tile vecino correspondiente.

---

## 8. Definition of Done (MVP)
El sistema se considera completo cuando:

- Cambios en Occupancy:
  - incrementan `StaticCostEpoch`.
  - disparan bake de `FinalCost_Static`.
  - invalidan flowfields.
  - reconstruyen solo los intents afectados.
- El solver consume exclusivamente `TileStaticData`.
- No existen rebuilds forzados manuales.
