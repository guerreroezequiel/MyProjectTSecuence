# FlowField System — World Tiles + HOT/WARM Pipeline (PR1)

## Objetivo
Definir de forma cerrada:
- la creación fija de tiles del mundo,
- la clasificación dinámica HOT/WARM/COLD según el jugador,
- y cómo esto se integra con el pipeline nuevo de FlowField
  (UpdateEpochs → Bake → IsValid/Rebuild).

---

## World Definition

- `GridConfig::WorldDim = 16`
- Universo de tiles del mundo:
  - `TileXY.X ∈ [0, 15]`
  - `TileXY.Y ∈ [0, 15]`
- Los tiles existen **siempre**; lo que cambia es su prioridad de procesamiento.

---

## Creación de Tiles (Startup)

### Punto de creación
- `FlowFieldSystem::BeginPlay()`  
  (o `InitializeWorldTiles()` llamado desde ahí)

### Procedimiento
Para cada `TileXY` válido del mundo:

- Crear `FTileContext`.
- Setear `TileXY` en el contexto.
- Inicializar epochs y dirty flags en estado default.
- (Opcional) crear `FTileStaticData` vacío o lazy.
- Registrar en:
  - `TilesByXY[TileXY]`
  - (Opcional) `StaticByXY[TileXY]`

### Resultado
Todos los tiles del mundo existen desde el inicio y tienen identidad correcta.
No se crean tiles dinámicamente en PR1.

---

## Clasificación HOT / WARM / COLD

### Definiciones
- **HOT**: tile donde está el jugador actualmente (1 tile).
- **WARM**: tiles vecinos a distancia Chebyshev 1 (hasta 8 tiles).
- **COLD**: todos los demás tiles del mundo.

### Regla WARM
Un tile es WARM si:
abs(dx) ≤ 1 AND abs(dy) ≤ 1 AND no es el HOT

---

## Obtención del Tile del Jugador

- `PlayerTileXY = GridWorld::WorldToTileXY(PlayerWorldPosition)`
- Alternativa mínima:
  - World → Cell → Tile

---

## Estado Interno del FlowFieldSystem

- `CurrentHotTileXY`
- `HotTiles` (1 tile)
- `WarmTiles` (hasta 8 tiles)
- `ActiveTiles = HotTiles ∪ WarmTiles`

---

## Actualización HOT/WARM (Runtime)

### Frecuencia
- Por tick o por epoch (criterio de performance).

### Algoritmo
1. Calcular `NewHot = PlayerTileXY`.
2. Si `NewHot == CurrentHotTileXY`:
   - no hacer nada.
3. Si cambió:
   - `CurrentHotTileXY = NewHot`
   - Recalcular WARM:
     - vecinos `dx,dy ∈ [-1..1]`
     - ignorar fuera de rango `[0, WorldDim-1]`
   - Actualizar:
     - `HOT = {NewHot}`
     - `WARM = vecinos válidos`
     - `ACTIVE = HOT ∪ WARM`

---

## Pipeline de Procesamiento (Solo ACTIVE)

Para cada tile en `ACTIVE`:

1. `TileContext.UpdateEpochs()`
2. Si `StaticCostEpoch` cambió:
   - bake de `FinalCost_Static` (tile-space)
3. Por intent activo (PR1: solo `Players`):
   - si `!FlowField.IsValid()`:
     - ejecutar `Rebuild` (solver + storage)

---

## Tiles COLD (PR1)

- No se procesan:
  - no bake
  - no rebuild
- Cuando el jugador se acerca:
  - pasan a WARM/HOT
  - se calculan recién en ese momento.

---

## Bordes del Mundo

- Tiles fuera de `[0, WorldDim-1]` se ignoran.

Ejemplo:
- HOT `(0,0)`
- WARM válidos: `(1,0)`, `(0,1)`, `(1,1)`

---

## Checklist PR1

- `WorldDim` define el mundo completo.
- Todos los tiles se crean en `BeginPlay`.
- `FTileContext` tiene `TileXY`.
- HOT/WARM se recalculan por movimiento del jugador.
- Solo tiles ACTIVE entran al pipeline.
- Pipeline: `UpdateEpochs → Bake → IsValid/Rebuild (Players)`.

---

## Scope
Este documento define completamente PR1.
Streaming dinámico, vecinos COLD con budget, multiplayer y múltiples intents
quedan fuera de alcance y se abordarán en PR2/PR3.
