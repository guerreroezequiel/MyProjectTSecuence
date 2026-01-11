# Bake de FinalCost_Static (Checklist de Implementación)

## Objetivo
Generar y mantener un buffer estático por Tile (`FinalCost_Static`) que:
- se reconstruya automáticamente cuando cambie el `StaticCostEpoch`
- sea consumido por el solver del FlowField
- encaje con el pipeline existente:
  Dirty → Epochs → Invalidate → Rebuild

---

## Estado actual (ya implementado)
- Tiles con `StaticCostEpoch` y dirty flags
- `UpdateEpochs()` convierte dirty → epochs
- `FlowField::IsValid()` compara epochs actuales vs built
- `FlowFieldSystem::Tick` hace rebuild condicional por intent

---

## 1. Artefacto nuevo por Tile
Definir un buffer estático asociado al Tile:

- `FinalCost_Static[cellIndex]`
- `INF_COST` como sentinel de bloqueo
- `BakedStaticCostEpoch`

Regla:
- `BakedStaticCostEpoch == StaticCostEpoch` ⇒ el buffer está actualizado

---

## 2. Static Cost Baker
Introducir una responsabilidad clara de bake:

**Nombre sugerido**
- `BakeFinalCostStatic(Tile)`

**Inputs (MVP)**
- Occupancy hard (bloqueado / libre)
- BaseCost (por ahora constante 1.0)

**Resultado por celda**
- bloqueada ⇒ `INF_COST`
- libre ⇒ `BaseCost`

---

## 3. Conexión con el Tick (pipeline)
En `FlowFieldSystem::Tick`, por cada Tile:

1. Guardar `PrevStaticCostEpoch`
2. Ejecutar `Tile.UpdateEpochs()`
3. Si `StaticCostEpoch` cambió:
   - ejecutar `BakeFinalCostStatic(Tile)`
   - setear `BakedStaticCostEpoch = StaticCostEpoch`

El bake **siempre ocurre antes** de evaluar `IsValid()` y `Rebuild`.

---

## 4. Contrato de RebuildFlowField
`RebuildFlowField(Tile, Intent)` debe asumir que:

- `FinalCost_Static` ya está actualizado
- el solver consume solo ese buffer estático

Durante el rebuild:
- se generan IntegrationField + DirectionField
- se capturan epochs:
  - `BuiltStaticCostEpoch`
  - `BuiltGoalsEpoch(Intent)`

---

## 5. Ajuste del Solver
Actualizar el contrato conceptual del solver:

- Entrada principal: `FinalCost_Static`
- No consultar Occupancy/BaseCost directo
- Regla:
  - `FinalCost_Static == INF_COST` ⇒ no expandir

---

## 6. Dirty marking (auditoría mínima)
Asegurar que marcan `StaticCostDirty`:

- `grid.occ.set`
- `grid.occ.clear`
- `grid.basecost.set` (si existe)

Cambios de goals **no** deben tocar StaticCostDirty.

---

## 7. Clear masivo (caso especial)
Resolver cómo `grid.occ.clear` invalida el sistema:

Opción MVP recomendada:
- `MarkAllTilesStaticDirty()` para tiles activos (HOT/WARM)

Objetivo:
- subir `StaticCostEpoch`
- disparar bake + rebuild

---

## 8. Debug mínimo recomendado
Agregar/usar estos inspectores:

- `grid.tile.info tx ty`
  - StaticCostEpoch
  - BakedStaticCostEpoch
  - GoalsEpoch por intent

- `grid.cost.cell x y`
  - valor de `FinalCost_Static`
  - bloqueado / libre

- `grid.ff.valid tx ty`
  - IsValid por intent
  - epochs actuales vs built

---

## 9. Tests rápidos de validación
### Test A — Bake por Occupancy
- `grid.occ.set x y blocked`
- tick
- verificar:
  - StaticCostEpoch++
  - BakedStaticCostEpoch actualizado
  - `FinalCost_Static(x,y) == INF_COST`

### Test B — Invalidate + Rebuild
- cambiar occupancy
- tick
- verificar:
  - `IsValid == false`
  - se ejecuta rebuild
  - `IsValid == true` luego

### Test C — Clear global
- `grid.occ.clear`
- tick
- verificar:
  - epochs suben
  - bake ocurre
  - flowfields se reconstruyen

---

## Resultado esperado
- Cambios de costo estático se bakean automáticamente
- `IsValid()` refleja cambios reales
- Rebuild ocurre solo cuando corresponde
- Solver consume datos estables y preprocesados
- Sistema listo para sumar terrain, puertas, ramps, etc.
