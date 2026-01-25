# TurboSequence + Mass ECS — Arquitectura “desde cero” (clean-room)  
**Objetivo:** eliminar todo lo previo y crear un set mínimo de archivos **correcto, estable y escalable** para integrar TurboSequence con tu sistema (Mass ECS), manteniendo el diseño **online-friendly** para futuro.

Este diseño asume:
- TurboSequence se opera vía **Manager** (único punto de control).
- Mass produce entidades y transforms; TS renderiza/anim con instancias.
- Queremos miles de entidades: necesitamos **update groups** y **solve 1 vez** por frame.
- Queremos una arquitectura que no dependa de hacks ni de “procesors globales invisibles”.

---

## 0) Principios de arquitectura (lo importante de verdad)

### P0 — Un solo “bridge” toca TurboSequence
Ningún Processor llama al Manager de TS.
Solo existe un módulo: **TS Bridge** (subsystem) que:
- asegura el Manager
- recibe comandos
- aplica comandos al Manager
- llama `SolveMeshes()` **una vez** por frame

### P0 — Lifecycle explícito (state machine)
Una entidad TS no puede ser “medio creada”.
El vínculo Mass↔TS se modela con estados:
- `None` → no hay instancia TS
- `Alive` → hay instancia TS válida
- (opcional) `PendingCreate/PendingDestroy` si necesitás robustez extra

En el diseño “simple y correcto”, el estado se vuelve robusto evitando “pending” con un pipeline de 2 fases (ver abajo).

### P0 — Command Buffer por frame (doble buffer)
Para evitar carreras y simplificar:
- Los Producers escriben en un buffer “CurrentFrame”.
- El Flush consume el buffer “CurrentFrame” y lo rota al finalizar.
- El buffer se escribe en GameThread (versión simple).  
  (Versión escalable: thread-safe o buffers por chunk → merge)

### P0 — Solve exactamente 1 vez por frame (y siempre en la misma fase)
`SolveMeshes()` después de aplicar creates/updates/destroys.

---

## 1) Qué archivos se crean (y solo estos)

### 1.1 Subsystem / Bridge (único dueño de TS)
- `TSBridgeSubsystem.h/.cpp`
  - encuentra/spawnea/cacha `ATurboSequence_Manager_Lf`
  - mantiene command buffers
  - expone API “Enqueue”
  - `FlushAndSolve(World, EntityManager)` aplica todo

### 1.2 Fragments (contratos de datos por entidad)
- `TSLinkFragment.h`
  - contiene el “handle” TS y config mínima (asset, group)
- `TSAnimFragment.h` *(opcional en v1; recomendado para separar anim del spawn)*
  - estado de anim “alto nivel” (Idle/Walk/Run/Attack), o un asset/clip id

### 1.3 Processor(s)
- `TSCommandBuildProcessor.h/.cpp`
  - lee fragments y decide Create/Update/Destroy
  - encola comandos al TSBridgeSubsystem
- `TSFlushProcessor.h/.cpp`
  - corre 1 vez por frame
  - llama `TSBridgeSubsystem.FlushAndSolve(...)`

### 1.4 (Opcional pero recomendado) Tag/Marker
- `TSRenderableTag.h`
  - marca entidades que deben tener TS (y evita queries “por accidente”)

> En un clean-room ideal, **no agregás tags de cleanup**.  
> Destrucción se maneja por “ShouldRenderTS = false” o LOD/Distance y el Build encola Destroy.

---

## 2) Contratos de Fragments (desde cero)

### 2.1 `FTSLinkFragment` (mínimo correcto)
Campos:
- `UTurboSequence_MeshAsset_Lf* MeshAsset` (asignado al spawn o por tipo)
- `FTurboSequence_MinimalMeshData MeshData` (handle/id de TS)
- `uint8 GroupId` (0 por defecto; ideal: Hot/Warm/Cold)
- `bool bAlive` (o `ETSLifecycle { None, Alive }`)

Reglas:
- `MeshData` solo es válido si `bAlive == true`.
- BuildProcessor es el único que decide “querés TS o no”.
- TSBridgeSubsystem es el único que crea/destruye y por ende el único que puede cambiar `MeshData` y `bAlive`.

### 2.2 `FTSAnimFragment` (opcional v1, recomendado)
Campos:
- `EAnimState DesiredState`
- `EAnimState CurrentState`
- (o `UTurboSequence_AnimAsset* DesiredAnim` si tu integración es directa)

Reglas:
- Producers setean `DesiredState`.
- TSBridge aplica cambios si hay transición.

---

## 3) Command Buffer (qué comandos existen)

### 3.1 Tipos de comando (mínimos)
- `CreateInstance { Entity, MeshAsset, Transform, GroupId }`
- `UpdateTransform { MeshData, Transform }`
- `DestroyInstance { MeshData, GroupId }`
- (opcional) `SetAnim { MeshData, Anim/State }`
- (opcional) `MoveGroup { MeshData, OldGroupId, NewGroupId }`

### 3.2 Invariantes
- Nunca se encola `UpdateTransform` si no hay `bAlive`.
- Nunca se encola `DestroyInstance` si no hay `bAlive`.
- `CreateInstance` solo si `bAlive == false`.

> Esta simpleza mata el 80% de los bugs típicos (duplicados, “zombie handles”, etc.)

---

## 4) Scheduling (fases Mass) — simple y correcto

### Fase recomendada
- **PostPhysics**
  - `TSCommandBuildProcessor` (produce commands)
  - `TSFlushProcessor` (consume + solve)

Justificación:
- en PostPhysics ya tenés transform final del frame
- TS aplica transforms y resuelve justo antes de render (conceptualmente)

Orden:
1) BuildProcessor (collect)
2) FlushProcessor (apply + solve)

---

## 5) Flujo completo “por frame” (la arquitectura)

### 5.1 Spawn
Cuando spawneas una entidad:
- agregás `FTSLinkFragment` con:
  - `MeshAsset` seteado (obligatorio)
  - `GroupId` seteado (por LOD/distance default)
  - `bAlive = false`

No creás TS en el spawner.
El spawner solo define “esta entidad es TS-renderable”.

### 5.2 BuildProcessor (produce)
Para cada entidad con `TSRenderableTag + Transform + TSLinkFragment`:
- calcular si “debe renderizar TS” este frame
  - ejemplo v1: siempre true
  - ejemplo v2: depende de LOD / distancia / visibility
- si debe y `bAlive=false` → enqueue Create
- si debe y `bAlive=true` → enqueue UpdateTransform (con epsilon opcional)
- si no debe y `bAlive=true` → enqueue Destroy

Opcional anim:
- si `DesiredAnim != CurrentAnim` → enqueue SetAnim

### 5.3 FlushProcessor → TSBridgeSubsystem (aplica)
TSBridgeSubsystem:
1) asegura Manager (find/spawn, y caching)
2) aplica Creates
   - crea TS instance → obtiene MeshData
   - actualiza `FTSLinkFragment.MeshData` + `bAlive=true`
   - agrega al update group
3) aplica Updates (transforms + anim)
4) aplica Destroys
   - remueve de update group
   - remove instance
   - set `MeshData invalid` + `bAlive=false`
5) `SolveMeshes()` 1 vez

---

## 6) Cómo se actualizan fragments desde el subsystem (parte crítica)

En clean-room, hay dos formas correctas:

### Opción A (simple, recomendada)
El TSBridgeSubsystem mantiene una lista:
- `CreatedResults { Entity, MeshData }`
- `DestroyedEntities { Entity }`

Durante `FlushAndSolve()` usa `FMassEntityManager` para:
- escribir `MeshData` y `bAlive` en los fragments de esas entidades

Ventajas:
- no necesitás “pending states”
- no dependés de que el flush tenga query

### Opción B (más Mass-purista)
El FlushProcessor hace queries sobre entidades “creadas/destroys” y aplica resultados.
Es más complejo y no aporta para v1.

---

## 7) Update Groups (desde cero: correcto pero simple)

### v1
- solo `GroupId = 0` para todos
- igual: **siempre** agregar/remover del group

### v2 (recomendado para hordas)
- `GroupId` por LOD bucket:
  - Hot = 0 (update every frame)
  - Warm = 1 (update cada N frames o budget limitado)
  - Cold = 2 (sin TS o muy low freq)

Esto te da la herramienta de performance que TS promueve: **controlar qué se actualiza** y con qué frecuencia.

---

## 8) Online-friendly (contratos futuros, sin implementar red todavía)

Para multiplayer futuro:
- nunca replicás `MeshData` (es local)
- replicás (o determinás) solo:
  - posición/rotación (ya lo hará tu netcode)
  - `DesiredAnim/State`
  - `Intent/LOD bucket` (si querés coherencia visual)
- TSBridge reconstruye localmente instancias TS a partir del estado replicado.

---

## 9) Qué NO se incluye a propósito (por “clean-room”)

- tags de cleanup tipo `TSPendingCleanupTag`
- flush “global invisible” sin resultados aplicables
- llamadas a TS desde spawner, movement o cualquier otro processor
- buffers compartidos sin contrato de thread

---

## 10) Checklist de aceptación (cuando está “bien implementado”)

- [ ] Existe exactamente 1 TS Manager en el mundo (find/spawn).
- [ ] Ningún Processor llama a TS Manager directamente.
- [ ] Build produce Create/Update/Destroy coherentes (según `bAlive`).
- [ ] Flush aplica commands y actualiza fragments (`MeshData` + `bAlive`).
- [ ] `SolveMeshes()` se llama 1 vez por frame en PostPhysics.
- [ ] UpdateGroups: Add y Remove siempre (aunque sea group 0).
- [ ] Si una entidad deja de ser TS-renderable, su instancia TS se destruye en el mismo frame o el siguiente (sin quedar colgada).

---

## 11) Lista final de archivos (exacta)

**Subsystem**
- `TSBridgeSubsystem.h`
- `TSBridgeSubsystem.cpp`

**Fragments**
- `TSLinkFragment.h`
- `TSAnimFragment.h` *(opcional)*

**Tags**
- `TSRenderableTag.h`

**Processors**
- `TSCommandBuildProcessor.h`
- `TSCommandBuildProcessor.cpp`
- `TSFlushProcessor.h`
- `TSFlushProcessor.cpp`

---

## 12) Extensión inmediata (cuando v1 esté estable)
- agregar `MoveGroup` al command set
- definir `GroupId` desde `TileLODFragment` (Hot/Warm/Cold)
- budget de updates por grupo (Warm/Cold)

