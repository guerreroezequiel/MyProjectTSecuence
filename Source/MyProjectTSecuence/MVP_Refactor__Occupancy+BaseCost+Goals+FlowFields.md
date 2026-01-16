# Plan detallado — MVP Macro FlowField (sin Capacity)
> Enfoque: arquitectura + responsabilidades + orden de trabajo (no código)

---

## 0) Alcance del MVP
**Incluye**
- TileContext por tile (HOT)
- Capas macro:
  - OccupancyHard (bloqueo)
  - BaseCost (costo base)
  - FinalCost_Static (resultado)
- Goals multi-source (positivos) por intención:
  - Players
  - Influences
  - Ambient
- FlowFields por intención:
  - FF_Players
  - FF_Influences
  - FF_Ambient
- Epochs + DirtyFlags + pipeline de rebuild
- Debug mínimo para verificar validez/invalidación

**Excluye**
- Capacity (macro/micro)
- Repulsion (negativos)
- Steering / slots / lógica por entidad
- Multiplayer

---

## 1) Definir el modelo mental (contratos)
### 1.1 Regla de oro
- El **tile** versiona y cachea (epochs).
- El **solver** consume un único costo final (FinalCost_Static).
- Las **entidades** solo consumen direcciones del flowfield.

### 1.2 “Inputs” vs “Outputs”
- Inputs:
  - OccupancyHard, BaseCost
  - Goals_* (listas de celdas fuente)
- Output:
  - FF_* (direcciones + distancias)

---

## 2) Estructura por tile (separación de responsabilidades)
### 2.1 TileContext — metadatos (mínimo)
**Epochs**
- StaticCostEpoch
- GoalsEpoch_Players
- GoalsEpoch_Influences
- GoalsEpoch_Ambient
- FlowEpoch_Players (debug)
- FlowEpoch_Influences (debug)
- FlowEpoch_Ambient (debug)

**DirtyFlags**
- bStaticCostDirty
- bGoalsDirty_Players
- bGoalsDirty_Influences
- bGoalsDirty_Ambient

> No contiene datos pesados ni buffers. Solo metadatos/versionado.

### 2.2 TileStaticData — caches estáticos (por tile)
**Grids**
- OccupancyHardGrid
- BaseCostGrid
- FinalCost_StaticGrid

**Baked**
- BakedStaticCostEpoch (época con la que se horneó FinalCost_Static)

> Se actualiza cuando `StaticCostEpoch` avanza y se rehace `FinalCost_Static`.

### 2.3 FlowFieldStorage — caches runtime (por intención)
**FlowFields (por tile/intención)**
- FF_Players: DistanceGrid + DirectionGrid + BuiltStaticCostEpoch + BuiltGoalsEpoch
- FF_Influences: idem
- FF_Ambient: idem

> Almacenamiento de lectura (ECS/Systems consumen direcciones). No guarda estado por entidad.

### 2.4 Responsabilidades
- TileContext: versionado (epochs) + suciedad (dirty) — liviano.
- TileStaticData: datos estáticos cacheados — costo de memoria por tile.
- FlowFieldStorage: resultados de solver por intención — cache runtime por tile/intent.

**Criterio de completitud**
- Podés loggear: epochs y dirty flags desde TileContext.
- Podés inspeccionar tamaños de grids en TileStaticData.
- Podés visualizar direcciones desde FlowFieldStorage.

---

## 3) CostComposer (macro estático)
### 3.1 Qué hace
Construye `FinalCost_Static` a partir de:
- BaseCost (valor base por celda)
- OccupancyHard (si bloqueada, no transitable)

### 3.2 Reglas (sin ambigüedad)
- Si celda está bloqueada → `unwalkable` (o costo infinito / flag)
- Si no bloqueada → `FinalCost = BaseCost`

### 3.3 Eventos que disparan costos
Marcan `bStaticCostDirty = true`:
- Cambios en OccupancyHard
- Cambios en BaseCost
- Inicialización del tile

### 3.4 Epoch y datos horneados
Cuando se reconstruye `FinalCost_Static`:
- `StaticCostEpoch++` en TileContext
- `BakedStaticCostEpoch = StaticCostEpoch` en TileStaticData
- `bStaticCostDirty = false`

**Criterio de completitud**
- Cambiar una celda bloqueada actualiza el costo final del tile.
- StaticCostEpoch sube únicamente cuando corresponde.

---

## 4) GoalsRegistry (multi-source) por intención
### 4.1 Qué es un Goal en este MVP
- Lista de “source cells” (puede ser 1 o muchas)
- No hay pesos en el MVP (todo source es equivalente)

### 4.2 Players (activo en MVP)
- Source: celda del player (o área pequeña si querés)
- Evento: player cambia de celda → `bGoalsDirty_Players = true`

### 4.3 Influences y Ambient (placeholders)
- Estructura lista aunque la lista esté vacía
- Permite habilitarlo después sin refactor

### 4.4 Epochs
Cuando se actualizan goals de una intención:
- `GoalsEpoch_Intent++` en TileContext
- `bGoalsDirty_Intent = false`

> Ownership: vive en un System por tile (no en TileContext). Emite cambios que impactan epochs/dirty.

**Criterio de completitud**
- GoalsEpoch_Players sube cuando el player cambia de celda.
- GoalsEpoch_Influences/Ambient existen y se pueden setear aunque estén vacíos.

---

## 5) FlowFields (3 intenciones, cache determinista)
### 5.1 Qué contiene cada FlowField (FlowFieldStorage)
- DistanceGrid (o equivalente)
- DirectionGrid (vector por celda)
- “BuiltWith”:
  - BuiltStaticCostEpoch
  - BuiltGoalsEpoch_Intent

### 5.2 Regla de validez
Un FF es válido si:
- BuiltStaticCostEpoch == StaticCostEpoch
- BuiltGoalsEpoch == GoalsEpoch_Intent

### 5.3 FlowEpoch (debug)
- Cada rebuild de FF_Intent incrementa `FlowEpoch_Intent`
- No define validez; solo traza “cuántas veces rebuild”

**Criterio de completitud**
- Si cambia StaticCostEpoch o GoalsEpoch, el FF queda inválido y se reconstruye.
- Si no cambia nada, no se reconstruye.

---

## 6) Pipeline de rebuild (orden fijo)
### 6.1 Orden (por tile)
1) Recolectar cambios → marcar dirty
2) Si `bStaticCostDirty`:
   - Rebuild FinalCost_Static (TileStaticData)
   - StaticCostEpoch++ (TileContext)
   - BakedStaticCostEpoch = StaticCostEpoch (TileStaticData)
3) Si `bGoalsDirty_*`:
   - Update goals del intent (GoalsRegistry en System)
   - GoalsEpoch_Intent++ (TileContext)
4) Validar FFs (Players/Influences/Ambient)
5) Rebuild FF inválidos (FlowFieldStorage) con prioridad:
   - FF_Players
   - FF_Influences
   - FF_Ambient
   (cada rebuild → FlowEpoch_Intent++)

### 6.2 Por qué este orden
- Costos primero: todos los FF dependen del costo final.
- Goals después: completás inputs antes de outputs.
- Flow al final: es lo caro; solo si hace falta.

**Criterio de completitud**
- En un tick sin cambios: 0 rebuilds.
- En un tick con move del player: solo FF_Players se invalida.

---

## 7) Prioridades y “no mezclar intenciones”
### 7.1 En el MVP (simple)
- Construís los 3 FF, pero solo consumís FF_Players
- Influences/Ambient quedan listos para consumo futuro

### 7.2 Regla de arquitectura
- No se mezclan directions dentro del solver.
- Si algún día querés “blend”, se hace en runtime (lector/selector), no en el FF.

---

## 8) Debug mínimo obligatorio (para validar el refactor)
### 8.1 Toggling por intención
- Mostrar flechas de:
  - FF_Players
  - FF_Influences
  - FF_Ambient

### 8.2 Overlay de costos
- Mostrar:
  - OccupancyHard (bloqueado/no)
  - BaseCost (si no es constante)
  - FinalCost_Static (resultado)

### 8.3 Telemetría por tile
- StaticCostEpoch
- GoalsEpoch_* / FlowEpoch_*
- Dirty flags

**Criterio de completitud**
- Podés explicar “por qué rebuildó” mirando epochs/flags.
- Podés verificar que Influences/Ambient están desacoplados.

---

## 9) Hitos de implementación (para no perderte)
### Hito A — Estructura
- TileContext con todo lo necesario (aunque vacío)
- Debug que muestre epochs/flags

### Hito B — Costos
- CostComposer + StaticCostEpoch funcionando
- Overlay de FinalCost_Static

### Hito C — Goals Players
- Goals_Players + GoalsEpoch_Players
- Trigger por movimiento del player

### Hito D — FF Players
- Rebuild FF_Players por validez
- Overlay de flechas funcionando

### Hito E — Placeholders
- Estructura completa para FF_Influences / FF_Ambient
- Toggling debug y epochs listos (aunque sin fuentes)

---

## 10) “Definition of Done” del MVP
- El sistema tiene 3 intents (Players/Influences/Ambient) a nivel estructura.
- El costo macro estático (Occupancy + BaseCost) produce FinalCost_Static.
- FF_Players se recalcula solo por cambios reales (epochs/dirty).
- Debug permite ver:
  - costos
  - flechas
  - epochs/dirty
- No hay dependencias con steering/slots/entidades.



---------------------------------------------------------------------------------------------------------------------------------

## Paso 1 — Sistema de Epochs, DirtyFlags y Rebuild Condicional (MVP)

### Objetivo
Establecer el **control de validez y cache** del sistema antes de implementar lógica de costos, goals o solver.
Este paso define **cuándo** se recalcula algo y **por qué**, sin depender todavía del contenido del cálculo.

---

### Alcance de este paso
Incluye:
- Epochs por tile (cost, goals, flow)
- DirtyFlags explícitos
- Regla de validez de FlowFields
- Pipeline de rebuild condicional
- Debug mínimo para trazabilidad

No incluye:
- Solver real
- Cálculo de costos
- Lógica de goals
- Movimiento de entidades

---

### Epochs definidos (por Tile)

#### Cost
- `StaticCostEpoch`  
  Versión del costo estático del tile (Occupancy + BaseCost).

#### Goals
- `GoalsEpoch_Players`
- `GoalsEpoch_Influences` (placeholder)
- `GoalsEpoch_Ambient` (placeholder)

#### Flow (output / debug)
- `FlowEpoch_Players`
- `FlowEpoch_Influences`
- `FlowEpoch_Ambient`

---

### DirtyFlags definidos

- `bStaticCostDirty`
- `bGoalsDirty_Players`
- `bGoalsDirty_Influences`
- `bGoalsDirty_Ambient`

Los DirtyFlags **no recalculan nada por sí mismos**.
Solo indican que, en el próximo pipeline, el epoch correspondiente debe avanzar.

---

### Regla de validez de FlowFields

Cada FlowField debe almacenar:
- `BuiltStaticCostEpoch`
- `BuiltGoalsEpoch`

Un FlowField es válido si:
- `BuiltStaticCostEpoch == StaticCostEpoch`
- `BuiltGoalsEpoch == GoalsEpoch_<Intent>`

Si alguna condición falla, el FlowField debe reconstruirse.

---

### Pipeline de Rebuild (MVP)

Orden fijo por tile:

1. Recolectar cambios y marcar DirtyFlags.
2. Si `bStaticCostDirty`:
   - Avanzar `StaticCostEpoch`
   - Limpiar `bStaticCostDirty`
3. Si `bGoalsDirty_<Intent>`:
   - Avanzar `GoalsEpoch_<Intent>`
   - Limpiar `bGoalsDirty_<Intent>`
4. Validar FlowFields por intención.
5. Reconstruir FlowFields inválidos (sin solver real por ahora):
   - Actualizar `BuiltStaticCostEpoch`
   - Actualizar `BuiltGoalsEpoch`
   - Avanzar `FlowEpoch_<Intent>`

---

### Debug mínimo requerido

Por cada tile, exponer:
- `StaticCostEpoch`
- `GoalsEpoch_*`
- `FlowEpoch_*`
- Estado de DirtyFlags

Este debug debe permitir responder:
- “¿Por qué se reconstruyó este FlowField?”
- “¿Qué cambió desde el último rebuild?”

---

### Criterio de finalización de este paso

Este paso se considera completo cuando:
- Los FlowFields **no se reconstruyen** si no cambió ningún epoch.
- Cambiar un DirtyFlag provoca el avance del epoch correcto.
- Cada rebuild deja trazabilidad clara vía `FlowEpoch`.
- Influences y Ambient existen como intents aunque no tengan lógica activa.
