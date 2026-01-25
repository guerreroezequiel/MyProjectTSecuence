# TurboSequence + Mass ECS — Implementación “oficial-style” (clean-room)

Este diseño está construido **directamente** sobre la arquitectura recomendada en la documentación oficial de TurboSequence:

- **Crear / registrar instancias** (GameThread)
- **Actualizar todas las instancias en un loop grande** (ECS loop, potencialmente multithread)
- **Resolver (Solve) por Update Group** **una vez por frame** (GameThread)
- **Update Groups auto-gestionados por el usuario** (self-managed)

> “ECS is totally fine… make sure to call SolveMeshes… one time per update group, one time a frame.”

---

## 0) Contratos “oficiales” (no negociables)

### C0.1 — Solve por grupo, 1 vez por frame (GameThread)
- Llamar `ATurboSequence_Manager_Lf::SolveMeshes_GameThread(DeltaTime, World, UpdateContext)`
- **Exactamente una vez por UpdateGroup** y **una vez por frame**.

### C0.2 — Loop grande de updates antes del Solve
La estructura recomendada por la doc es:

1. Game Thread logic
2. **for loop grande** (todas las instancias, idealmente multithread)
3. Solve Update Group
4. Game Thread logic

El Solve **siempre ocurre después** de haber actualizado todas las instancias.

### C0.3 — Update Groups son self-managed
- Agregar: `AddInstanceToUpdateGroup_Concurrent(GroupIndex, Instance)`
- Remover: `RemoveInstanceFromUpdateGroup_Concurrent(GroupIndex, Instance)`

TurboSequence **no maneja automáticamente** los grupos.  
El juego es responsable de add/remove coherente.

---

## 1) Clean-room: archivos nuevos (mínimos y correctos)

### 1.1 Fragments (por entidad)

### A) `FTurboSequenceFragment`
Contiene **todo el estado mínimo requerido** para que una entidad Mass sea renderizada por TurboSequence.

- `FTurboSequence_MeshSpawnData_Lf SpawnData`
- `TObjectPtr<UAnimSequence> Anim`
- `FTurboSequence_AnimPlaySettings_Lf AnimSettings`
- `FTurboSequence_MinimalMeshData_Lf Instance`
- `int32 UpdateGroupIndex`
- `bool bHasInstance`

> Este conjunto coincide con el “Most Minimal Setup” mostrado en la doc oficial.

### B) `FTurboSequenceDesiredTransformFragment` (opcional)
Permite desacoplar el transform “objetivo” del `TransformFragment`.
En el MVP puede omitirse y leer el `TransformFragment` directamente.

### C) Tags
- `FTurboSequenceTag`  
Marca entidades controladas por TurboSequence.

---

## 2) Subsystem / Service (bridge mínimo)

### 2.1 `UTurboSequenceECSSubsystem`

Este subsystem **no inventa un pipeline nuevo**.  
Solo centraliza responsabilidades que la doc asume implícitas.

#### Responsabilidades
- Garantizar que existe un `ATurboSequence_Manager_Lf` válido en el `World`.
- Proveer helpers para ejecutar `SolveMeshes()` por UpdateGroup.

---

## 3) Creación del `ATurboSequence_Manager_Lf` (runtime Ensure)

La documentación oficial **no explica cómo se crea el Manager**.
Asume que existe.

Para una integración ECS correcta y robusta, se adopta un **Ensure automático**.

### Regla
- Si existe un `ATurboSequence_Manager_Lf` en el `World`, se usa.
- Si no existe, se spawnea uno.
- **Nunca** se crean dos.

### Dónde ocurre
- El Ensure se ejecuta **la primera vez que se llama a Solve**  
  (no en el spawn de entidades, no en constructores, no en editor).

Esto garantiza:
- World válido (Game / PIE)
- ejecución en GameThread
- cero dependencia de mapas o setup manual

### Algoritmo (conceptual)
1. Obtener `UWorld`
2. Buscar actores `ATurboSequence_Manager_Lf`
3. Si existe uno → reutilizar
4. Si no existe → spawn (`AlwaysSpawn`)
5. Cachear la referencia
6. Continuar con Solve normalmente

### Invariante global
> En cualquier frame donde se invoquen funciones de TurboSequence,  
> existe **exactamente un** `ATurboSequence_Manager_Lf` válido en el World.

Este paso **no contradice** la doc oficial: simplemente cubre una suposición implícita.

---

## 4) Processors (implementación ECS siguiendo la doc)

### 4.1 `UTurboSequenceSpawnProcessor` (GameThread)

Objetivo:  
**Create instance + add to update group + play animation**, tal como en el ejemplo mínimo oficial.

#### Requisitos
Entidades con:
- `FTurboSequenceTag`
- `FTurboSequenceFragment`
- `TransformFragment`

#### Reglas
1. Si `bHasInstance == false`:
   - `Instance = AddSkinnedMeshInstance_GameThread(SpawnData, Transform, World)`
   - Si `Instance.IsMeshDataValid()`:
     - `AddInstanceToUpdateGroup_Concurrent(UpdateGroupIndex, Instance)`
     - `PlayAnimation_Concurrent(Instance, Anim, AnimSettings)`
     - `bHasInstance = true`

Este processor **solo crea**.  
No hace updates ni destroys.

---

### 4.2 `UTurboSequenceUpdateProcessor` (loop grande ECS)

Objetivo:  
Actualizar **todas** las instancias antes del Solve.

#### Requisitos
Entidades con:
- `FTurboSequenceTag`
- `FTurboSequenceFragment`
- `TransformFragment`

#### Reglas
- Si `bHasInstance == true` y `Instance.IsMeshDataValid()`:
  - enviar transform actual a TurboSequence
- (Opcional) solicitar cambios de anim, IK, root motion, etc.

Este es el loop que la doc describe como:
> “big ECS loop, potentially multithreaded”

---

### 4.3 `UTurboSequenceSolveProcessor` (GameThread)

Objetivo:  
Ejecutar el Solve **una vez por grupo y por frame**.

#### Regla
Para cada `UpdateGroupIndex` activo:
- Llamar `SolveMeshes_GameThread(DeltaTime, World, UpdateContext)`

Notas:
- Si hay un solo grupo (0), se llama una vez.
- Si hay Hot/Warm/Cold, se decide qué grupos resolver este frame.
- Si un grupo no se resuelve, **acumular DeltaTime** (según la doc).

---

### 4.4 `UTurboSequenceDestroyProcessor` (GameThread)

Objetivo:  
Eliminar correctamente instancias TS cuando la entidad deja de ser renderizable.

#### Reglas
Si `bHasInstance == true` y `Instance.IsMeshDataValid()`:
1. `RemoveInstanceFromUpdateGroup_Concurrent(UpdateGroupIndex, Instance)`
2. Llamar a la API de “Remove Instance” correspondiente
3. Invalidar `Instance`
4. `bHasInstance = false`

Esto cumple el contrato **self-managed** de Update Groups.

---

## 5) Orden de ejecución recomendado (Mass)

### Fase sugerida
**PostPhysics / PostUpdate**

1. `TurboSequenceSpawnProcessor`
2. `TurboSequenceUpdateProcessor`
3. `TurboSequenceSolveProcessor`
4. `TurboSequenceDestroyProcessor` (si aplica)

Siempre:
> Update all → Solve → (cleanup)

---

## 6) Update Groups

### v1 (mínimo correcto)
- Todos usan `UpdateGroupIndex = 0`
- Solve grupo 0 una vez por frame

### v2 (hordas)
- Hot = 0, Warm = 1, Cold = 2
- Warm/Cold se resuelven menos frecuente
- DeltaTime se acumula si un grupo “laggea”

---

## 7) Pipeline de assets / cache (oficial)

Si aparecen vértices corruptos:
- usar **Invalid Cache** en el control panel
- cerrar UE
- reabrir y regenerar meshes

Esto es explícitamente recomendado por el autor.

---

## 8) Qué NO hace esta arquitectura

- No usa command buffers globales
- No usa tags de cleanup
- No llama Solve múltiples veces por frame
- No toca TurboSequence fuera de los processors designados
- No depende de colocar el Manager a mano en mapas

---

## 9) Resumen operativo

1. Fragment TS con SpawnData + Instance + GroupIndex
2. SpawnProcessor crea instancias
3. UpdateProcessor actualiza todas
4. SolveProcessor resuelve por grupo
5. DestroyProcessor limpia correctamente
6. Manager se asegura automáticamente

Esta es la implementación **más fiel**, **simple** y **correcta** respecto a la documentación oficial.
