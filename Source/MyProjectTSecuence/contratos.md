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

## 1. Owner de Tiles (TileRegistry)
Un sistema de nivel Grid (`GridWorld` / `GridEpochSubsystem` / `TileRegistry`) es el único responsable de:

- Crear tiles
- Definir tiles activos (HOT / WARM / COLD)
- Setear `TileXY` en cada `FTileContext`
- Exponer acceso estable a `FTileContext` por `TileXY`

---

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
  - Si `StaticCostDirty` → incrementa `StaticCostEpoch` y limpia el flag.
  - Si `GoalsDirty[Intent]` → incrementa `GoalsEpoch(Intent)` y limpia el flag.
- No contiene buffers.
- No ejecuta bake.
- No ejecuta solver.

### Epoch Semantics
- Los epochs no representan tiempo ni importancia.
- Son contadores de invalidez.
- Incrementos frecuentes no son un problema mientras:
  - el rebuild sea condicional
  - y esté limitado a tiles activos.

### Explicit Exclusions (MVP)
- El solver no considera:
  - Occupancy dinámica
  - Capacity macro o micro
  - Repulsion
  - Steering
- Estas capas se integrarán en fases posteriores.

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


## 4. FlowFieldSystem.h / .cpp
**Rol:** Orquestador del lifecycle por tile e intent.

### Tile Lifecycle Contract
- Solo tiles **HOT** y **WARM** participan del pipeline de epochs y rebuild.
- Tiles **COLD**:
  - Mantienen su estado interno (epochs y caches).
  - No ejecutan `UpdateEpochs()` ni `Rebuild`.
  - No actualizan Goals.
  - Pueden reactivarse sin pérdida de consistencia.

### Responsabilidades
- Mantener:
  - `TileContext` por tile
  - `TileStaticData` por tile
- Ejecutar bake de costos estáticos.
- Decidir cuándo reconstruir FlowFields.
- Ser el owner de `FlowFieldStorage`.

### Pipeline obligatorio por tile
1. `TileContext.UpdateEpochs()`
2. Si cambió `StaticCostEpoch`:
   - Bake de `FinalCost_Static`
   - Setear `TileStaticData.BakedStaticCostEpoch`
3. Para cada `EFlowIntent` activo:
   - Evaluar `FlowField.IsValid(Context, Intent)`
   - Si no es válido → ejecutar o encolar `Rebuild`

### Reglas
- Nunca se evalúa `IsValid()` con datos estáticos desactualizados.
- El bake ocurre **antes** de cualquier rebuild.
- La cola de rebuild (si existe) es responsabilidad exclusiva del System.

---

## 5. FlowFieldSolver.h / .cpp
**Rol:** Algoritmo de pathfinding / integración / direcciones.

### Inputs
- `TileStaticData` (tile-space)
- Goals del intent
- Parámetros del solver

### Goals Scope Contract (MVP)
- Los Goals son estrictamente tile-local.
- No existe propagación de goals entre tiles.
- Cada tile resuelve su FlowField de forma independiente.

### Reglas
- `FinalCost_Static == INF_COST` → celda no transitable.
- El solver no consulta Occupancy ni otras capas directamente.

### Outputs
- `IntegrationField[NumCells]`
- `DirectionField[NumCells]`

---

## 6. FlowFieldStorage.h / .cpp
**Rol:** Cache y frontera de consumo de resultados del solver.

### FlowFieldStorage Contract
- Es la única fuente de datos consumidos en runtime por las entidades.
- El solver nunca es consultado directamente.
- Los FlowFields pueden ser:
  - cacheados
  - invalidados
  - reemplazados
sin afectar a los consumidores.

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
- El ownership de `FlowFieldStorage` pertenece al `FlowFieldSystem`.

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

## 8. FlowField Consumption Contract (Entity Side)
- Las entidades:
  - leen `DirectionField`
  - no modifican el FlowField
- La selección de intent (`Players` / `Influences` / `Ambient`)
  ocurre fuera del solver.
- No existe lógica de blending dentro del FlowField.

---

## 9. Definition of Done (MVP)
El sistema se considera completo cuando:

- Cambios en Occupancy:
  - incrementan `StaticCostEpoch`
  - disparan bake de `FinalCost_Static`
  - invalidan FlowFields
  - reconstruyen solo los intents afectados
- El solver consume exclusivamente `TileStaticData`.
- No existen rebuilds forzados manuales.
- El consumo de FlowFields está desacoplado del solver.

---

## 10. ECS (Mass) + TurboSequence — Contratos (MVP)
**Rol:** Consumo por entidad de FlowField + movimiento determinista + sincronización de render/animación vía TurboSequence.

### Fuentes de verdad
- Headers públicos: `Source/MyProjectTSecuence/Public/ECS/*`
- El FlowField se consume vía `FlowFieldStorage` (RO) y nunca se recalcula por entidad.

### Ownership (regla base)
- ECS/Mass es dueño del estado por entidad (Transform, velocidad, intención, vida).
- FlowField es dueño del `DirectionField` por tile/celda (ECS solo lee snapshots).
- TurboSequence es dueño del render/animación; ECS solo sincroniza instancias y transforms.

---

## 10.1 Tags (marcadores)
**Rol:** Filtrado explícito de entidades para cada pipeline.

### Contrato
- `FZombiTag` (`Public/ECS/Tags/ZombiTag.h`) identifica el set principal del MVP.
- `FTurboSequenceTag` (`Public/ECS/Tags/TurboSequenceTag.h`) habilita el pipeline de TurboSequence.
- `FHiddenTag` (`Public/ECS/Tags/HiddenTag.h`) representa estado “no visible” a nivel ECS.
- `FTSPendingCleanupTag` (`Public/ECS/Tags/TSPendingCleanupTag.h`) marca entidades para limpieza diferida (si se usa).

### Reglas
- Los Processors de TurboSequence no deben tocar entidades sin `FTurboSequenceTag`.
- Si una entidad pierde `FTurboSequenceTag`, debe converger a “sin instancia TS” sin dejar handles colgados.

---

## 10.2 Fragments (contratos de datos)

### 10.2.1 CellLocationFragment.h
**Rol:** Ubicación en grilla por entidad.

### Responsabilidades
- Mantener `TileXY`, `CellIndex` y `bValid` (`Public/ECS/Fragments/CellLocationFragment.h`).

### Contrato
- `CellIndex` es tile-local.
- `bValid=false` implica que no se puede consumir FlowField para esa entidad.

### 10.2.2 FlowReadFragment.h
**Rol:** Cache liviano del resultado de lectura del FlowField.

### Responsabilidades
- Almacenar `DirWS`, `EpochSeen`, `bValid` y fallback (`CachedLastValidDirWS`, `FallbackFramesLeft`) (`Public/ECS/Fragments/FlowReadFragment.h`).

### Contrato
- `EpochSeen` proviene del storage (fuente única) y se usa solo como validez/telemetría.
- Si `bValid=false`, el consumer debe tener fallback seguro (Idle / 0 / cached dir según reglas del proyecto).

### 10.2.3 MoveFragment.h
**Rol:** Parámetros mínimos de movimiento.

### Contrato
- `Speed` (`Public/ECS/Fragments/MoveFragment.h`) es la velocidad base; el movimiento se integra en base a `DirWS`.

### 10.2.4 TileLODFragment.h
**Rol:** LOD de simulación por entidad (derivado de tile/criterios del proyecto).

### Contrato
- `ETileLOD` (`Hot/Warm/Cold`) (`Public/ECS/Fragments/TileLODFragment.h`) guía degradación de frecuencia/costo.

### 10.2.5 TurboSequenceFragment.h
**Rol:** Estado mínimo para representar una entidad con TurboSequence.

### Responsabilidades
- Mantener:
  - `SpawnData`, `Anim`, `AnimSettings`, `Instance`, `UpdateGroupIndex`, `bHasInstance` (`Public/ECS/Fragments/TurboSequenceFragment.h`).

### Invariante
- `bHasInstance == true` es el único indicador permitido para tratar `Instance` como válido.

---

## 10.3 Processors (pipeline ECS)
**Rol:** Ejecutar un pipeline determinista por frame (o por LOD) sin work pesado por entidad.

### Orden lógico (MVP)
1. `UUpdateCellLocationProcessor` (`Public/ECS/Processors/UpdateCellLocationProcessor.h`)
2. `UFlowDirReadPlayersProcessor` (`Public/ECS/Processors/FlowDirReadPlayersProcessor.h`)
3. `UMoveIntegrateProcessor` (`Public/ECS/Processors/MoveIntegrateProcessor.h`)
4. `UTileLODUpdateProcessor` (`Public/ECS/Processors/TileLODUpdateProcessor.h`) (si aplica)

### Contrato clave
- `UFlowDirReadPlayersProcessor` solo lee `Intent::Players` en el MVP y escribe `FFlowReadFragment`.
- `UMoveIntegrateProcessor` debe tener fallback seguro si `FFlowReadFragment.bValid == false`.

---

## 10.4 TurboSequenceECSSubsystem (bridge mínimo)
**Rol:** Asegurar el Manager de TurboSequence y exponer el Solve por grupo.

### Contrato
- `EnsureManager_GameThread()` (`Public/ECS/Subsystems/TurboSequenceECSSubsystem.h`):
  - Solo GameThread.
  - Find/spawn + cache de `ATurboSequence_Manager_Lf`.
- `SolveGroup_GameThread(DeltaTime, GroupIndex)`:
  - Solo GameThread.
  - Resuelve exactamente para ese `GroupIndex`.

### Invariante
- En cualquier frame donde se ejecute Solve, existe exactamente un `ATurboSequence_Manager_Lf` válido en el `World`.

---

## 10.5 TurboSequence Processors (lifecycle por entidad)
**Rol:** Create/Update/Solve/Destroy de instancias TS siguiendo el modelo “Solve una vez por frame por grupo”.

### Contrato (MVP)
- Spawn: `UTurboSequenceSpawnProcessor` (`Public/ECS/Processors/TurboSequenceSpawnProcessor.h`).
- Update: `UTurboSequenceUpdateProcessor` (`Public/ECS/Processors/TurboSequenceUpdateProcessor.h`).
- Solve: `UTurboSequenceSolveProcessor` (`Public/ECS/Processors/TurboSequenceSolveProcessor.h`).
- Destroy: `UTurboSequenceDestroyProcessor` (`Public/ECS/Processors/TurboSequenceDestroyProcessor.h`).

### Reglas
- Solve es **a lo sumo 1 vez por frame y por UpdateGroup**.
- El orden lógico recomendado es: Spawn → Update → Solve → Destroy.

---

## 10.6 Debug (opcional)
**Rol:** Herramientas de test/spawn y visualización.

### Contrato
- `UEntityDebugSpawnerSubsystem` (`Public/ECS/Debug/EntityDebugSpawnerSubsystem.h`) puede spawnear entidades de test y exponer control para debug.