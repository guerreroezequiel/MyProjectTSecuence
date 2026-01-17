# MVP Ultra-min — Mass ECS + leer FlowField (Intent = Players) + consola spawn

## Objetivo
Spawnear entidades Mass por consola y que **todas**:
1) calculen `TileXY + CellIndex`
2) lean **FlowField Intent = Players**
3) muevan su posición siguiendo `DirField`

Sin estados, sin idle, sin blending, sin avoidance.

---

## 1) Datos ECS mínimos

### Tag
- `FZombiTag` (o `FTestFFTag`) para filtrar la query.

### Fragments
1) `FZombiCoreFragment` (o un core mínimo)
- `FVector Position`
- `FRotator Rotation`

2) `FCellLocationFragment`
- `FIntPoint TileXY`
- `int32 CellIndex`
- `bool bValid`

3) `FFlowReadFragment`
- `FVector DirWS`
- `int32 EpochSeen`
- `bool bValid`

4) `FMoveFragment`
- `float Speed`

---

## 2) Contrato mínimo con FlowFieldStorage

Necesitamos **una sola llamada** para leer el campo del tile:

`TryGetFieldView(TileXY, EFlowIntent::Players) -> { DirFieldPtr, Epoch, bValid }`

Reglas:
- `DirFieldPtr` es válido durante el tick (read-only).
- Si no existe / no está built / tile cold sin build => `bValid=false`.

---

## 3) Processors mínimos (en orden)

> Todos en `EMassProcessingPhase::PrePhysics` y con `bAutoRegisterWithProcessingPhases=true`.

### A) UpdateCellLocationProcessor
**Core(Position) -> CellLocation(TileXY, CellIndex, bValid)**

- Convierte `Position` a `TileXY`
- Convierte `Position` a `CellXY` dentro del tile
- Calcula `CellIndex` tile-local

Si la posición está fuera del mundo: `bValid=false`.

---

### B) FlowDirReadPlayersProcessor
**CellLocation -> FlowRead**

- Si `CellLocation.bValid==false` => `FlowRead.bValid=false`
- `view = FlowFieldStorage.TryGetFieldView(TileXY, Players)`
- Si `view.bValid` y `CellIndex` en rango:
  - `DirWS = view.DirFieldPtr[CellIndex]`
  - `EpochSeen = view.Epoch`
  - `bValid=true`
- Si no:
  - `bValid=false`

---

### C) MoveIntegrateProcessor
**FlowRead + Move + Core -> Core(Position/Rotation)**

- Si `FlowRead.bValid`:
  - `Vel = Normalize(DirWS) * Speed`
  - `Position += Vel * DeltaSeconds`
  - `Rotation` mira hacia `Vel` si `|Vel| > EPS`
- Si no:
  - no mueve (o vel=0)

---

## 4) Spawn por consola (mínimo)

### Subsystem (Spawn/Clear)
- `SpawnZombis(int32 Count, float Radius)`
  - crea `Count` entidades con:
    - `Core.Position = PlayerPos + RandomPointInCircle(Radius)` *(esto es “random de spawn”, no de movimiento)*
    - `Move.Speed = default`
    - `CellLocation.bValid=false` (se completa en el primer tick)
    - `FlowRead.bValid=false`
    - Tag `FZombiTag`

- `ClearZombis()`

### Comandos
- `ecs.spawn <count> <radius>`
- `ecs.clear`

---

## 5) Definition of Done
- `ecs.spawn 500 2000` crea entidades.
- En el tick:
  - se calcula tile/cell
  - se lee **Players** flowfield
  - las entidades se mueven hacia el/los goals de Players.
- `ecs.clear` borra todo.

---
