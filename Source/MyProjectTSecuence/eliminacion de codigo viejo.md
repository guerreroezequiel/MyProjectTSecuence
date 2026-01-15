# Limpieza del Pipeline Viejo y Clarificación de Contratos — FlowField System

## Objetivo
Eliminar definitivamente el **pipeline viejo** (dirty queues, rebuilder step, versionado paralelo) y dejar el sistema alineado a un **único contrato claro**, basado en:

- TileContext (epochs + dirty flags)
- Bake de costos estáticos
- FlowFieldSystem como orquestador
- Solver desacoplado
- Storage único para datos runtime

Este documento describe:
- Qué partes del pipeline viejo siguen existiendo
- Qué archivos se pueden eliminar
- Qué lógica heredada conviene mover, renombrar o redefinir
- Cómo cerrar los contratos para evitar ambigüedades futuras

---

## 1. Qué consideramos “pipeline viejo”

Se considera pipeline viejo a cualquier lógica basada en:

- Dirty queues globales
- Rebuild incremental por presupuesto
- Versionado propio del flowfield independiente de epochs
- Accesos globales no ligados a TileContext

Concretamente:
- `FlowFieldRebuilder.h`
- `GDirtyTiles`
- `GDirtyQueue`
- `MarkTileDirty(...)`
- `RebuildStep(...)`
- `TileVersion` como fuente de validez

El pipeline nuevo reemplaza todo esto por:

DirtyFlag (TileContext)
→ UpdateEpochs()
→ Bake FinalCost_Static
→ FlowField.IsValid()
→ FlowField.Rebuild()


---

## 2. Pipeline viejo real que queda en el proyecto

### 2.1 FlowFieldRebuilder.h
**Estado:** código muerto

- Define dirty queues y rebuild step legacy.
- No está referenciado por ningún `.cpp` o `.h` del set actual.
- No participa del pipeline nuevo.

**Acción recomendada:**
- Eliminar el archivo completo del proyecto.

Con esto, el pipeline viejo de “cola + presupuesto” desaparece oficialmente.

---

## 3. Componentes que NO son pipeline viejo, pero rompen los contratos

Estas piezas no pertenecen estrictamente al pipeline viejo, pero **heredan supuestos del diseño anterior** y hacen que los contratos nuevos no sean claros.

---

### 3.1 FlowFieldStorage.h (estado actual: legacy-stub)

#### Situación actual
- Storage global (`GStorage`)
- Clave por `TileXY` solamente
- Guarda `Dist` / `Dir`
- Usa `TileVersion` como versionado interno
- No contempla `Intent`

#### Problemas con el contrato nuevo
- No soporta múltiples intents (`Players`, `Influences`, `Ambient`)
- Introduce un versionado paralelo que compite con:
  - `StaticCostEpoch`
  - `GoalsEpoch`
- Permite estados ambiguos (data válida según TileVersion, inválida según epochs reales)

#### Decisión necesaria
Elegir **una** de las siguientes opciones para dejar el contrato claro:

**Opción A (recomendada):**
- Convertir este archivo en el **FlowFieldStorage nuevo**
- Clave por `(TileXY, Intent)`
- Almacenar:
  - `IntegrationField`
  - `DirectionField`
  - `BuiltStaticCostEpoch`
  - `BuiltGoalsEpoch`
- Eliminar `TileVersion`

**Opción B (mínimo cambio):**
- Renombrar el archivo a `LegacyFlowFieldStorage.h`
- Crear un `FlowFieldStorage.h` nuevo que cumpla el contrato actual

---

### 3.2 FlowFieldSolver.h (mezcla de responsabilidades)

#### Situación actual
El solver:
- Consulta Occupancy
- Consulta Capacity
- Consulta Heat/Density
- Tiene pesos (`AlphaHeat`, `BetaCapacity`)

#### Problema
El contrato MVP define que:
- El solver **solo consume** `FinalCost_Static`
- Todas las capas (Occupancy, BaseCost, etc.) deben estar horneadas antes

Hoy el solver:
- Consulta el mundo directamente
- Viola el desacople solver ↔ capas

#### Opciones para aclarar el contrato

**Opción A (recomendada MVP):**
- Simplificar el solver:
  - Input: `TileStaticData.FinalCost_Static` + Goals
  - Output: Integration + Direction
- Ninguna consulta directa a Occupancy/Capacity/Heat

**Opción B (separación explícita):**
- `FlowFieldSolver_MVP`
- `FlowFieldSolver_Legacy`
- Cada uno con responsabilidades claras

---

### 3.3 FlowFieldRegistry / Consola (compatibilidad heredada)

#### Situación actual
- La consola crea `FFlowField` por tile usando un registry.
- Esto puede crear FlowFields paralelos a los usados por `FlowFieldSystem`.

#### Problema
- El sistema real rebuilda FlowFields desde `FlowFieldSystem`.
- La consola puede estar mostrando datos que **no son los reales**.

#### Recomendación
- La consola **no debe crear FlowFields**.
- La consola debe:
  - consultar al `FlowFieldSystem`, o
  - leer directamente del `FlowFieldStorage` (fuente runtime)

---

## 4. Qué eliminar definitivamente

Eliminar del proyecto:

- `FlowFieldRebuilder.h`
- Cualquier referencia a:
  - `MarkTileDirty`
  - `GDirtyTiles`
  - `GDirtyQueue`
  - `RebuildStep`

Hacer un grep global para confirmar que no quedan usos.

---

## 5. Contratos finales esperados (post-limpieza)

Cuando la limpieza esté completa, debe cumplirse:

- TileContext es el **único owner de epochs y dirty flags**
- No existen colas globales de rebuild
- El único pipeline es:


Dirty → Epoch → Bake → Validate → Rebuild

- FlowField **no es owner de buffers**
- FlowFieldStorage es la **única fuente runtime** de:
- IntegrationField
- DirectionField
- Solver no consulta capas externas
- No existe versionado paralelo al sistema de epochs

---

## 6. Definition of Done — Limpieza completa

La limpieza se considera completa cuando:

- El proyecto compila sin `FlowFieldRebuilder.h`
- No existe lógica de dirty queue
- `FlowFieldStorage` cumple (o está claramente separado como legacy)
- El solver respeta el contrato MVP
- La consola no crea FlowFields paralelos
- Los contratos son explícitos y no ambiguos

---

## 7. Beneficio de hacer esta limpieza ahora

- Evita bugs silenciosos de versionado
- Hace el sistema razonable de debuggear
- Habilita:
- streaming de tiles
- multiplayer
- múltiples intents reales
- Reduce deuda técnica antes de agregar complejidad (Influences, Capacity, etc.)

---

Este documento define el estado “correcto” del sistema FlowField a partir de ahora.
