# TurboSequence + Mass ECS — Implementación “oficial-style” (clean-room)

Este diseño está construido **directamente** sobre la arquitectura recomendada en la doc oficial:
- **Crear/registrar instancias** (GameThread)
- **Actualizar todas las instancias en un loop grande** (ECS loop; puede ser multithread)
- **Resolver (Solve) por Update Group** **una vez por frame** (GameThread)  
y con **Update Groups auto-gestionados por vos** (self-managed). :contentReference[oaicite:0]{index=0}

> La wiki lo dice explícito: “ECS is totally fine… make sure to call SolveMeshes… one time per update group, one time a frame.” :contentReference[oaicite:1]{index=1}

---

## 0) Contratos “oficiales” (no negociables)

### C0.1 — Solve por grupo, 1 vez por frame (GameThread)
- Llamar `ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, World, UpdateContext)`
- **Una vez por UpdateGroup** y **una vez por frame**. :contentReference[oaicite:2]{index=2}

### C0.2 — Loop grande de updates antes del Solve
La estructura recomendada es:
1) “Game Thread Logic”
2) **for loop** (todas las instancias, idealmente multithread)
3) “Solve Update Group”
4) “Game Thread Logic” :contentReference[oaicite:3]{index=3}

### C0.3 — Update Groups son self-managed
- Agregar: `AddInstanceToUpdateGroup_Concurrent(GroupIndex, Instance)`
- Remover: `RemoveInstanceFromUpdateGroup_Concurrent(GroupIndex, Instance)`  
y **vos** tenés que add/remove cuando agregás/removés instancias. :contentReference[oaicite:4]{index=4}

---

## 1) Clean-room: archivos nuevos (mínimos y correctos)

### 1.1 Fragments (por entidad)
**A) `FTurboSequenceFragment`**
- `FTurboSequence_MeshSpawnData_Lf SpawnData` (o lo mínimo para spawn)
- `TObjectPtr<UAnimSequence> Anim` (opcional si definís anim por entidad)
- `FTurboSequence_AnimPlaySettings_Lf AnimSettings`
- `FTurboSequence_MinimalMeshData_Lf Instance` (handle TS)
- `int32 UpdateGroupIndex` (0..N)
- `bool bHasInstance`

> La doc muestra `FTurboSequence_MeshSpawnData_Lf` + `UAnimSequence` + `FTurboSequence_AnimPlaySettings_Lf` como el “setup mínimo”. :contentReference[oaicite:5]{index=5}

**B) `FTurboSequenceDesiredTransformFragment` (opcional)**
- Guardar transform “objetivo” si querés desacoplar del `TransformFragment`.
- En MVP podés leer el `FTransformFragment` directo.

**C) Tags**
- `FTurboSequenceTag` (marca entidades controladas por TS)

---

## 2) Subsystem / Service (bridge mínimo)

### 2.1 `UTurboSequenceECSSubsystem`
Responsabilidad: **solamente** proveer utilidades comunes (no “inventar” un pipeline alternativo).

Funciones:
- `EnsureManager(World)` (si la doc/tu proyecto requiere encontrar al Manager; en TS la API es estática por clase en los ejemplos, pero el solve necesita `World`).
- `SolveGroup(World, DeltaTime, GroupIndex)`  
  Internamente:
  - arma `FTurboSequence_UpdateContext_Lf UpdateContext; UpdateContext.GroupIndex = GroupIndex;`
  - llama `SolveMeshes_GameThread(...)` :contentReference[oaicite:6]{index=6}

> Importante: en el estilo oficial, el “subsystem” no es obligatorio. Es una comodidad tuya para centralizar “solve por grupo” y config. Lo que **sí** es obligatorio es el orden: update-loop → solve. :contentReference[oaicite:7]{index=7}

---

## 3) Processors (la implementación ECS siguiendo la doc)

### 3.1 `UTurboSequenceSpawnProcessor` (GameThread, una vez o cuando haga falta)
Objetivo: “create instance + add to group + play animation” tal como el ejemplo mínimo.

Para entidades con:
- `FTurboSequenceTag`
- `FTurboSequenceFragment`
- (y opcional `TransformFragment`)

Reglas:
1) Si `bHasInstance == false`:
   - `Instance = AddSkinnedMeshInstance_GameThread(SpawnData, Transform, World)` :contentReference[oaicite:8]{index=8}
   - si `Instance.IsMeshDataValid()`:
     - `AddInstanceToUpdateGroup_Concurrent(GroupIndex, Instance)` :contentReference[oaicite:9]{index=9}
     - `PlayAnimation_Concurrent(Instance, Anim, AnimSettings)` :contentReference[oaicite:10]{index=10}
     - set `bHasInstance=true`

> Este es literalmente el “Most Minimal Setup” de la doc, trasladado a un loop ECS. :contentReference[oaicite:11]{index=11}

### 3.2 `UTurboSequenceUpdateProcessor` (ECS loop grande, puede ser multithread)
Objetivo: aplicar “Game Thread Logic / ECS updates” antes del Solve.

Para entidades con:
- `FTurboSequenceTag`
- `FTurboSequenceFragment`
- `TransformFragment`

Reglas (mínimas):
- Si `bHasInstance` y `Instance.IsMeshDataValid()`:
  - “empujar” transform actual al TS instance (la función exacta depende del API de TS que uses en tu versión; el punto es que este loop es el lugar oficial para actualizar todas las instancias). :contentReference[oaicite:12]{index=12}
- (Opcional) cambios de anim, IK, root motion, etc. también se piden aquí (antes del solve)

> La doc no obliga a un nombre de función para transform update en esta página, pero sí fija la **estructura**: loop grande (multithreadable) y luego solve por grupo. :contentReference[oaicite:13]{index=13}

### 3.3 `UTurboSequenceSolveProcessor` (GameThread, PostUpdate)
Objetivo: ejecutar el Solve en el lugar correcto.

Regla:
- Para cada `GroupIndex` activo:
  - `SolveMeshes_GameThread(DeltaTime, World, UpdateContext)` una vez por frame. :contentReference[oaicite:14]{index=14}

Notas:
- Si tenés 1 solo grupo (0): llamás una vez.
- Si tenés Hot/Warm/Cold: llamás para cada grupo que quieras resolver este frame.
- Si implementás lag/budget: recordá que la doc menciona que al usar grupos “lag” frames y podés tener que **acumular DeltaTime** para grupos que no se resuelven cada frame. :contentReference[oaicite:15]{index=15}

### 3.4 `UTurboSequenceDestroyProcessor` (GameThread)
Objetivo: cuando una entidad deja de existir o deja de ser TS-renderable:
- remover de update group (self-managed)
- remover la instancia TS

Reglas:
- Si `bHasInstance && Instance.IsMeshDataValid()`:
  - `RemoveInstanceFromUpdateGroup_Concurrent(GroupIndex, Instance)` :contentReference[oaicite:16]{index=16}
  - llamar a la función de “remove instance” correspondiente a tu versión (en el README se menciona “Remove instances” como feature; en el ejemplo mínimo no aparece el remove, pero el contrato de self-managed update groups sí exige el remove del group). :contentReference[oaicite:17]{index=17}
  - `bHasInstance=false` y invalidar handle

---

## 4) Orden de ejecución recomendado (Mass)

### Fase sugerida (alineada con doc)
- **PostPhysics / PostUpdate**
  1) `TurboSequenceSpawnProcessor` (solo para nuevas)
  2) `TurboSequenceUpdateProcessor` (loop grande)
  3) `TurboSequenceSolveProcessor` (solve por grupo 1 vez)

La doc expresa el concepto “update all → solve group” (y no al revés). :contentReference[oaicite:18]{index=18}

---

## 5) Update Groups: versión mínima “correcta” y extendible

### v1 (mínima)
- `GroupIndex = 0` para todo.
- Solve group 0 una vez por frame. :contentReference[oaicite:19]{index=19}

### v2 (hordas)
- Hot=0, Warm=1, Cold=2
- Warm/Cold resuelven menos frecuente (lag aceptable a distancia), como describe la doc. :contentReference[oaicite:20]{index=20}
- Si un grupo no se resuelve en un frame, **acumular DeltaTime** para ese grupo. :contentReference[oaicite:21]{index=21}

---

## 6) “Oficial” también implica: pipeline de assets / cache
Si ves posiciones de vértices incorrectas, el autor recomienda usar el botón **Invalid Cache** cerca del control panel, cerrar UE y reabrir para regenerar meshes. :contentReference[oaicite:22]{index=22}

---

## 7) Qué NO hacemos en esta arquitectura (porque no está en el “oficial-style”)
- No hacemos “flush global” basado en command-buffers como requisito (podés hacerlo, pero la doc recomienda explícitamente “loop grande → solve”). :contentReference[oaicite:23]{index=23}
- No metemos “pending tags” como parte esencial: la destrucción se modela con “dejar de ser renderable” y el DestroyProcessor.
- No llamamos Solve múltiples veces por frame (prohibido por contrato). :contentReference[oaicite:24]{index=24}

---

## 8) Resumen en 10 líneas (lo que tenés que implementar)
1) Fragment TS con `SpawnData + Anim + Settings + Instance + GroupIndex + bHasInstance`.
2) SpawnProcessor: `AddSkinnedMeshInstance_GameThread` → `AddInstanceToUpdateGroup` → `PlayAnimation`. :contentReference[oaicite:25]{index=25}
3) UpdateProcessor: loop grande actualiza todas las instancias (transforms/anim/IK si aplica). :contentReference[oaicite:26]{index=26}
4) SolveProcessor: `SolveMeshes_GameThread` por grupo, 1 vez por frame. :contentReference[oaicite:27]{index=27}
5) DestroyProcessor: `RemoveInstanceFromUpdateGroup` + remove instance (y limpiar fragment). :contentReference[oaicite:28]{index=28}

