# Eliminación del Pipeline Viejo — Checklist Final (MVP)

Este documento resume **qué falta eliminar / ajustar** para cerrar definitivamente el *pipeline viejo* y dejar **solo el pipeline nuevo** activo, coherente con los contratos acordados:

> **Pipeline nuevo**  
> TileContext (epochs + dirty)  
> → Bake de costos estáticos  
> → Validación por epochs  
> → Rebuild condicional de FlowFields  
> → Solver desacoplado (consume costo final)

---

## 1. FlowFieldSolver — Eliminar lógica del pipeline viejo (CRÍTICO)

### Estado actual
El solver todavía mezcla lógica **dinámica y legacy**:
- Heat (`DensityHeatGrid`)
- Capacity (`CapacityGrid`)
- Occupancy live (`Grid::IsBlocked`)

Esto corresponde **100% al pipeline viejo**.

### Contrato correcto (MVP)
El solver debe:
- ❌ NO conocer heat, capacity ni occupancy live
- ✅ Consumir **únicamente**:
  - `TileStaticData.FinalCost_Static`
  - Goals del intent
- Tratar `INF_COST` como celda no transitable

### Acciones
- Eliminar includes:
  - `DensityHeatGrid.h`
  - `CapacityGrid.h`
  - `OccupancyGrid.h`
- Reemplazar toda lógica de bloqueo dinámico por:
  - lectura de `FinalCost_Static`
  - skip si el costo es infinito
- El costo de movimiento se calcula **solo** en base al costo estático bakeado

> Resultado esperado:  
> El solver es **puro**, determinista y desacoplado del mundo live.

---

## 2. FlowFieldRegistry — Eliminar compatibilidad global (LEGACY)

### Estado actual
Existe un `FlowFieldRegistry` global que:
- auto-crea `FlowField`
- expone acceso fuera del `FlowFieldSystem`
- existe solo por compatibilidad/consola

Esto **rompe el contrato nuevo**.

### Contrato correcto
- Owner de FlowFields: **FlowFieldSystem**
- Runtime data: **FlowFieldStorage**
- No existen FlowFields “globales”

### Acciones
- Eliminar `FlowFieldRegistry.h`
- Quitar cualquier uso desde consola o debug
- No auto-crear FlowFields fuera del sistema principal

---

## 3. FlowFieldConsoleLite — Limpiar comandos del pipeline viejo

### Estado actual
La consola todavía:
- Expone comandos de **Capacity**
- Usa `FlowFieldRegistry` para crear/consultar FlowFields
- Valida estado usando objetos legacy (`FFlowField`)

### Contrato correcto (MVP)
La consola debe:
- ❌ No conocer Capacity
- ❌ No instanciar FlowFields
- ✅ Leer:
  - `TileContext` (epochs, dirty flags)
  - Metadata de `FlowFieldStorage`
- ✅ Forzar cambios solo vía:
  - `MarkStaticCostDirty`
  - `MarkGoalsDirty(Intent)`

### Acciones
- Eliminar comandos:
  - `SetBaseCapacity`
  - `SetCurrentCount`
  - `ClearCapacity`
- Reemplazar validaciones por:
  - checks de epochs
  - estado baked / dirty
- Si existe “rebuild now”:
  - que **solo marque dirty**
  - nunca ejecute solve directo

---

## 4. Goals — Estado aceptable (pero dejar explícito)

### Estado actual
- Goals viven en el Subsystem
- `FlowFieldSystem` los escribe
- `FFlowField::Rebuild()` los consume

Esto **está OK para MVP**, pero parece legacy si no se documenta.

### Acción recomendada
- Agregar comentario/TODO explícito:
  - Goals son globales **solo en MVP**
  - El contrato final es por intent (y potencialmente por tile)
- (Opcional) separar `GoalsByIntent`

> No es obligatorio refactorizar ahora.

---

## 5. Regla de coherencia que falta sellar (IMPORTANTE)

### Regla del pipeline nuevo
Antes de resolver un FlowField:
- `TileStaticData.BakedStaticCostEpoch`
  **debe coincidir con**
- `TileContext.StaticCostEpoch`

### Acción
- Dejar esto explícito:
  - comentario
  - check debug
  - assert opcional

Esto garantiza:
- bake → solve siempre en orden correcto
- nunca se resuelve con datos stale

---

## Definition of Done — Pipeline viejo eliminado

El pipeline viejo se considera **completamente eliminado** cuando:

- [ ] El solver no incluye heat, capacity ni occupancy live
- [ ] No existe `FlowFieldRegistry`
- [ ] La consola no toca Capacity ni crea FlowFields
- [ ] El rebuild ocurre solo por epochs + dirty flags
- [ ] El solver consume solo `FinalCost_Static`
- [ ] La coherencia bake → solve está explícita

---

## Estado final esperado

- Arquitectura clara
- Responsabilidades bien separadas
- Pipeline determinista
- MVP sólido y extensible
- Sin residuos legacy ocultos

> A partir de este punto, cualquier feature nueva (capacity, heat, repulsion, multiplayer) entra **como extensión**, no como parche.
