# FlowField System — Revisión de Implementación vs Contratos (MVP)

## Estado General
La implementación actual del sistema FlowField **cumple el contrato funcional del MVP**:
- El pipeline de epochs, dirty flags y rebuild condicional está correctamente armado.
- No existen rebuilds innecesarios.
- El orden de ejecución respeta las dependencias (costos → goals → flow).

No se detectaron violaciones críticas.  
Sí se identifican **ajustes conceptuales recomendados** para reforzar límites y evitar problemas futuros.

---

## 1. Validación por Componente

### 1.1 TileContext
**Estado:** ✅ Correcto

Cumple el rol acordado de *versionador lógico* del tile.

Incluye:
- `StaticCostEpoch`
- `GoalsEpoch[Intent]`
- `FlowEpoch[Intent]` (solo debug)
- DirtyFlags:
  - `bStaticCostDirty`
  - `bGoalsDirty[Intent]`

Comportamiento:
- `UpdateEpochs()` incrementa únicamente los epochs correspondientes.
- Los dirty flags se limpian correctamente.
- No ejecuta bake ni solver.

✔️ Alineado con el contrato.

---

### 1.2 FlowField::IsValid
**Estado:** ✅ Correcto

Regla de validez implementada:
- `BuiltStaticCostEpoch == Context.StaticCostEpoch`
- `BuiltGoalsEpoch == Context.GoalsEpoch(Intent)`

No depende de lógica externa ni de flags.
No mezcla responsabilidades.

✔️ Implementación mínima y correcta.

---

### 1.3 FlowFieldSystem (Pipeline)
**Estado:** ✅ Correcto

Orden de ejecución actual:
1. `TileContext.UpdateEpochs()`
2. Si cambió `StaticCostEpoch` → bake de `FinalCost_Static`
3. Por cada `EFlowIntent`:
   - `IsValid()`
   - `Rebuild()` si corresponde

Reglas cumplidas:
- El bake ocurre antes del solver.
- Los FlowFields solo se reconstruyen si están inválidos.
- No hay rebuilds forzados.

✔️ Pipeline cerrado y coherente.

---

## 2. Ajustes Conceptuales Recomendados

### 2.1 Límite del TileContext (Importante)
**Situación actual:**
El `TileContext` está funcionando como contenedor de:
- Occupancy
- BaseCost
- FinalCost
- FlowFields

Esto **funciona**, pero **no respeta el contrato lógico original**, donde:

- `TileContext` = estado + epochs + flags
- Los datos pesados viven en sistemas externos

**Recomendación (sin refactor inmediato):**
- No agregar más lógica al TileContext.
- No ejecutar bake ni solver dentro del TileContext.
- Tratarlo conceptualmente como *header lógico del tile*.

Este límite es clave para:
- Streaming
- Multiplayer
- Replicación futura

---

### 2.2 Falta explícita de FlowFieldStorage 
**Situación actual:**
`FlowField`:
- Calcula
- Almacena
- Expone resultados

Esto mezcla dos responsabilidades:
- Cálculo
- Almacenamiento/cache

**Contrato deseado:**
- Solver / FlowField → calcula
- FlowFieldStorage → almacena y expone datos runtime

**Recomendación MVP-friendly:**
- Dejar explícito (comentarios / TODO) que:
  - `IntegrationField` y `DirectionField` son **datos cacheados**
  - Conceptualmente pertenecen a un `FlowFieldStorage`

Esto habilita a futuro:
- Double buffer
- Serialización
- Networking
- Debug avanzado

---

## 3. Detalles Menores a Vigilar

### 3.1 GoalsDirty vs StaticCostDirty
- El movimiento del player marca `bGoalsDirty_Players`.
- No debe marcar `bStaticCostDirty`.

✔️ Correcto para MVP  
⚠️ Verificar que no haya caminos indirectos que toquen StaticCostEpoch.

---

### 3.2 FlowEpoch (Debug)
- `FlowEpoch` se usa solo para trazabilidad.
- No define validez.
- No afecta gameplay.

✔️ Uso correcto  
⚠️ No reutilizarlo para lógica futura.

---

### 3.3 Solver (Pendiente)
Cuando se implemente:
- Debe consumir **solo** `FinalCost_Static`.
- No debe consultar Occupancy ni BaseCost.
- No debe mezclar intents.

---

## 4. Conclusión

**Estado actual del sistema:**
- 🟢 Funcional
- 🟢 Alineado al MVP
- 🟡 Con límites conceptuales a reforzar

No falta ningún componente crítico para continuar.

Próximos pasos seguros:
- Implementar solver
- Completar debug visual
- Activar Influences y Ambient

La arquitectura es sólida y escalable si se respetan los límites definidos en este documento.


=============================================


# Resolver “FlowFieldStorage” (sin refactor grande, sin código)

## Objetivo del cambio
Separar responsabilidades:

- **FlowField / Solver**: calcula (produce Integration + Direction)
- **FlowFieldStorage**: almacena + expone datos runtime (Integration + Direction + epochs built-with)

El resultado buscado es que:
- `FFlowField` NO sea dueño de buffers pesados.
- `IsValid()` se base en *epochs built-with del storage* (y no en estado interno del FlowField).
- El runtime (movimiento/debug/network) lea siempre desde una única fuente: Storage.

---

## 1) Agregar el componente “FlowFieldStorage” (nuevo archivo / módulo)
Crear un nuevo componente conceptual (puede ser clase, singleton, subsistema o miembro del FlowFieldSystem).

### 1.1 Qué debe almacenar (por TileXY + Intent)
Por cada combinación `(TileXY, Intent)` el storage debe guardar:

**Datos runtime**
- `IntegrationField[NumCells]`
- `DirectionField[NumCells]`

**Meta (built-with)**
- `BuiltStaticCostEpoch`
- `BuiltGoalsEpoch`

**Regla de existencia**
- Si no hay datos construidos aún, el entry puede no existir, o existir con “data vacía”.

### 1.2 Qué debe exponer (API conceptual)
- `GetOrCreate(TileXY, Intent)` → devuelve/asegura buffers del tamaño correcto.
- `Find(TileXY, Intent)` → acceso read-only (para runtime/debug).
- `ClearTile(TileXY)` y/o `ClearAll()` (útil para COLD/unload).
- `IsValid(Context, Intent)`:
  - existe data
  - y `BuiltStaticCostEpoch == Context.StaticCostEpoch`
  - y `BuiltGoalsEpoch == Context.GoalsEpoch(Intent)`

> Nota: puede vivir dentro de `FlowFieldSystem` como `Storage` o como “global” MVP.
> Lo importante es que sea la fuente única runtime.

---

## 2) Cambiar la responsabilidad de `FFlowField`
Hoy `FFlowField` probablemente:
- guarda `IntegrationField` y `DirectionField`
- guarda `BuiltStaticCostEpoch` / `BuiltGoalsEpoch`
- expone getters a runtime

Eso es lo que vamos a desarmar.

### 2.1 Qué le sacás a `FFlowField`
- Cualquier buffer:
  - `IntegrationField`
  - `DirectionField`
- Cualquier meta de “built-with epochs” que vivía adentro.

### 2.2 Qué conserva `FFlowField`
- La lógica de:
  - `IsValid(Context, Intent)`
  - `Rebuild(Context, Intent)`
- Debug helpers si querés (pero el debug lee del Storage).

### 2.3 Qué cambia en `IsValid()`
Antes: comparaba epochs del FlowField interno vs Context.

Ahora:
- `IsValid()` delega en `FlowFieldStorage.IsValid(Context, Intent)`.

Esto garantiza que:
- si el storage no tiene datos → inválido
- si los epochs no matchean → inválido

---

## 3) Cambiar cómo se escribe el resultado del solver
Hoy el solver “rebuild” probablemente termina escribiendo dentro de miembros de `FFlowField`.

Ahora debe escribir en buffers que le da el Storage.

### 3.1 Cambio conceptual en Rebuild()
En `Rebuild(Tile, Intent)`:
1) Pedir al Storage los buffers del `(TileXY, Intent)`:
   - `IntegrationField`
   - `DirectionField`
2) Ejecutar el solver y escribir ahí.
3) Actualizar meta:
   - `BuiltStaticCostEpoch = Context.StaticCostEpoch`
   - `BuiltGoalsEpoch = Context.GoalsEpoch(Intent)`
4) Incrementar `FlowEpoch(Intent)` (solo telemetría).

> El solver no debe “poseer” buffers; solo llenarlos.

---

## 4) Cambiar el punto de lectura runtime (movimiento / debug)
Actualmente puede que el runtime consulte datos desde:
- el `FFlowField` o desde TileContext.

Esto debe migrar a:

- **Toda lectura** de `DirectionField` / `IntegrationField` se hace desde:
  - `FlowFieldStorage.Find(TileXY, Intent)`

### 4.1 Regla de oro
- Runtime / debug / networking NUNCA leen desde `FFlowField`.
- Runtime / debug / networking leen SOLO desde Storage.

---

## 5) Dónde ubicar FlowFieldStorage (MVP)
Opciones válidas (elegí la más cómoda):

### Opción A (simple MVP)
- `FlowFieldStorage` como singleton / global estático.
Pros: 0 plomería.
Contras: menos elegante.

### Opción B (recomendada)
- `FlowFieldStorage` como miembro del `FlowFieldSystem`.
Pros: ownership claro.
Contras: pasar referencia donde haga falta.

### Opción C
- `FlowFieldStorage` dentro del `GridEpochSubsystem`.
Pros: cercano a tiles/epochs.
Contras: mezcla más sistemas.

Para MVP, A o B están perfectas.

---

## 6) Checklist de “Definition of Done” de este cambio
El cambio está completo cuando:

- `FFlowField` no tiene arrays de integration/direction ni built epochs internos.
- Existe `FlowFieldStorage` con:
  - Integration/Direction por tile+intent
  - built epochs por tile+intent
- `IsValid()` valida contra Storage (no contra estado interno).
- `Rebuild()`:
  - pide buffers al Storage
  - ejecuta solver escribiendo ahí
  - actualiza built epochs del storage
- El runtime lee siempre desde Storage.

---

## 7) Beneficios inmediatos (por qué conviene ahora)
- Evita que `FFlowField` sea “todo-en-uno”.
- Permite:
  - doble buffer (futuro)
  - serialización / replay (futuro)
  - replicación por tile+intent (multiplayer)
  - unload de tiles COLD sin tocar el solver
- Hace que “validez” sea real:
  - si no hay data en Storage → inválido (aunque epochs coincidan por accidente)


---

## 8) Acciones y Ajustes a Aplicar (consolidados)

Estas acciones consolidan las recomendaciones de “Límite del TileContext” y “Resolver FlowFieldStorage” para el MVP.

- **TileContext = header lógico**
  - Mantener solo: `StaticCostEpoch`, `GoalsEpoch[Intent]`, `FlowEpoch[Intent]` (debug) y dirty flags.
  - No agregar bake/solver ni buffers pesados al `TileContext`.
  - `UpdateEpochs()` solo incrementa `StaticCostEpoch` y `GoalsEpochs` cuando corresponda.

- **FlowEpoch = contador de rebuild real**
  - No se incrementa en `UpdateEpochs()`.
  - Incrementar únicamente después de un rebuild efectivo (`FlowField::Rebuild()` exitoso).

- **FlowFieldStorage (única fuente runtime)**
  - Agregar un storage por `(TileXY, Intent)` con:
    - Buffers: `IntegrationField`, `DirectionField`.
    - Meta: `BuiltStaticCostEpoch`, `BuiltGoalsEpoch`.
  - API conceptual:
    - `GetOrCreate(TileXY, Intent)` asegura tamaño y devuelve referencias de escritura.
    - `Find(TileXY, Intent)` lectura para runtime/debug.
    - `IsValid(Context, Intent)` comparando meta vs epochs del `TileContext`.
    - `ClearTile(TileXY)` / `ClearAll()` para COLD/unload.
  - Ubicación MVP: miembro de `FlowFieldSystem` o singleton simple (evitar acoplar al Player).

- **FFlowField = cálculo puro**
  - `IsValid(Context, Intent)` delega en `FlowFieldStorage.IsValid(...)`.
  - `Rebuild(Context, Intent)` pide buffers al Storage, ejecuta solver y escribe allí.
  - Actualiza meta del Storage y luego incrementa `FlowEpoch(Intent)` para trazabilidad.
  - No posee buffers ni meta de built internos.

- **Lectura runtime/debug**
  - Toda lectura de integración/dirección proviene de `FlowFieldStorage.Find(TileXY, Intent)`.
  - Nunca leer datos de `FFlowField` directamente.

- **Ownership y límites**
  - Tiles: owner único = TileRegistry/Grid. Los sistemas solo iteran/leen.
  - Storage: owner por mundo/sistema (no por `FFlowField`).

### Checklist (Definition of Done)
- [ ] `FlowFieldStorage` existe y almacena Integration/Direction + meta por `(TileXY, Intent)`.
- [ ] `FFlowField` no contiene buffers ni meta internos.
- [ ] `IsValid()` usa Storage como autoridad de validez.
- [ ] `Rebuild()` escribe en Storage y actualiza meta; luego incrementa `FlowEpoch(Intent)`.
- [ ] Runtime/debug leen exclusivamente desde Storage.
- [ ] `TileContext` permanece sin lógica pesada; `UpdateEpochs()` no toca `FlowEpoch`.

