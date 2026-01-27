### PARTE DOS

# ECS + TurboSequence — Estados + Animaciones (MVP online-friendly)

## Objetivo
Definir un **pipeline mínimo y escalable** para que cada entidad:
1) compute su **estado gameplay** (Idle/Walk/Run/…),
2) seleccione una **animación deseada** en base a ese estado,
3) aplique el cambio a **TurboSequence** solo cuando haga falta (sin spamear `PlayAnimation`),
4) quede listo para futuro **multiplayer** (determinismo / replicación por IDs, no por random local).

---

## Principios (no negociables)

### P0 — Gameplay State ≠ Animación
- El estado gameplay es “la verdad” (determinista).
- La animación es una **representación** (puede variar por LOD, por calidad, etc.).
- Nunca mezclar lógica de decisiones en el código de TurboSequence.

### P1 — Cambios de animación solo por “dirty”
- Cambiar anim **solo** cuando `DesiredAnim` / `DesiredPlayRate` cambian.
- Evitar llamar `PlayAnimation` cada frame.

### P2 — Online-friendly desde el diseño
- Nada de `Rand()` por entidad para elegir anim o estado.
- Variación futura:
  - o viene de “seed” determinista (tile/epoch/ID),
  - o se replica como “AnimVariantId”.

### P3 — Solve una vez por frame por UpdateGroup (GameThread)
- `Solve` por grupo: 1 vez por frame (o por schedule si haces Warm/Cold).
- El pipeline de animación debe quedar **antes** del Solve.

---

## Estructura ECS (Fragmentos)

### 1) `FZombieStateFragment` (nuevo)
**Responsabilidad:** estado gameplay mínimo.
- `ELocoState Loco` : `Idle | Walk | Run`
- `EActionState Action` : `None | Attack | HitReact | Dead`
- (opcional) `uint8 bHasTarget` / `uint8 bAggro`

> Nota: En MVP podés empezar solo con `Loco` y `Dead`.

---

### 2) `FAnimRequestFragment` (nuevo)
**Responsabilidad:** “pedido” de animación para la capa TS.
- `UAnimSequence* DesiredAnim`
- `float DesiredPlayRate`
- `bool bDirty` (true solo cuando cambió algo)

Opcional (para futuro online/replicación):
- `uint16 DesiredAnimId` (en vez de puntero)
- `uint8 AnimVariantId` (para variedad determinista)

---

### 3) `FVelocityFragment` (opcional pero recomendado)
**Responsabilidad:** métricas para decidir locomoción sin hacks.
- `FVector VelWS`
- `float SpeedWS`

> Si hoy ya tenés velocidad en otro fragment, reusalo. Lo importante es que el “state update” lea de un dato claro.

---

### 4) Reusar existentes (ya están)
- `FMoveFragment` (posición/rotación/velocidad según tu implementación)
- `FFlowReadFragment` (DirWS + bValid + EpochSeen)
- `FTileLODFragment` (Hot/Warm/Cold)
- `FTurboSequenceFragment` (Instance + Anim/Settings + UpdateGroupIndex)

---

## Processors (Pipeline propuesto)

> Orden recomendado con tus fases actuales (PrePhysics / PostPhysics).

### PRE-PHYSICS (Gameplay / Movimiento)
1) `UpdateCellLocationProcessor`
2) `TileLODUpdateProcessor`
3) `FlowDirReadPlayersProcessor`
4) `MoveIntegrateProcessor`
5) **(nuevo)** `ZombieStateUpdateProcessor`
6) **(opcional)** `VelocityUpdateProcessor` (si no lo calculás dentro de MoveIntegrate)

---

### POST-PHYSICS (TurboSequence + Anim)
1) `TurboSequenceSpawnProcessor`
2) `TurboSequenceUpdateProcessor` (sync transforms a TS)
3) **(nuevo)** `ZombieAnimSelectProcessor`
4) **(nuevo)** `TurboSequenceAnimApplyProcessor` (aplica anim si bDirty)
5) `TurboSequenceSolveProcessor` (Solve por grupo)
6) `TurboSequenceDestroyProcessor`

---

## Contratos por Processor

### A) `ZombieStateUpdateProcessor` (nuevo) — PrePhysics
**Lee:**
- `FFlowReadFragment` (bValidFlow, DirWS opcional)
- `FVelocityFragment` o `FMoveFragment` (SpeedWS)
- `FZombiCoreFragment` (Health/flags si aplica)

**Escribe:**
- `FZombieStateFragment`

**Reglas MVP sugeridas (deterministas):**
- Si `Health <= 0` → `Action = Dead`, `Loco = Idle`
- Sino:
  - Si `SpeedWS <= EPS_SPEED` → `Loco = Idle`
  - Si `SpeedWS > EPS_SPEED` y `SpeedWS < RUN_THRESHOLD` → `Loco = Walk`
  - Si `SpeedWS >= RUN_THRESHOLD` → `Loco = Run`
- Si querés ligar al flow:
  - Si `!bValidFlow` → forzar `Idle` (o “slow walk” si preferís)

**Notas online:**
- `RUN_THRESHOLD` y `EPS_SPEED` deben ser constantes globales para consistencia.
- Nada de random en esta etapa.

---

### B) `ZombieAnimSelectProcessor` (nuevo) — PostPhysics (antes de Solve)
**Lee:**
- `FZombieStateFragment`
- `FTileLODFragment` (Hot/Warm/Cold)
- (opcional) `FAnimRequestFragment` para comparar “current desired” y detectar cambios

**Escribe:**
- `FAnimRequestFragment` (setea DesiredAnim/PlayRate + bDirty)

**Tabla de mapeo (MVP):**
- `Dead` → `Anim_Dead` (si existe)
- `Loco=Idle` → `Anim_Idle`
- `Loco=Walk` → `Anim_Walk`
- `Loco=Run` → `Anim_Run`

**LOD-aware (opcional desde el día 1):**
- Hot:
  - `DesiredPlayRate = 1.0`
- Warm:
  - `DesiredPlayRate = 0.9` (o igual, pero podés simplificar)
- Cold:
  - `DesiredPlayRate = 0.8` o “freeze pose”
  - o directamente marcar “no anim changes” si querés ultra barato

**Dirty contract:**
- `bDirty = true` solo si:
  - `DesiredAnim` cambió, o
  - `DesiredPlayRate` cambió (más de un epsilon)

---

### C) `TurboSequenceAnimApplyProcessor` (nuevo) — PostPhysics (antes de Solve)
**Lee:**
- `FTurboSequenceFragment` (Instance/Manager handle + bHasInstance)
- `FAnimRequestFragment` (DesiredAnim/Rate/bDirty)

**Escribe:**
- `FAnimRequestFragment.bDirty = false` (si aplicó)
- (opcional) `FTurboSequenceFragment.Anim = DesiredAnim` como “current anim” cache

**Regla:**
- Si `bHasInstance && AnimRequest.bDirty`:
  - Llamar `PlayAnimation_Concurrent(Instance, DesiredAnim, Settings)`
  - Setear play rate en settings (si corresponde)
  - `bDirty = false`

**Importante:**
- No tomar decisiones acá.
- No llamar `PlayAnimation` sin dirty.

---

## UpdateGroups + LOD (escalabilidad)

### Objetivo
Que el costo de `Solve` y/o updates baje en WARM/COLD.

### Contrato recomendado
- `FTurboSequenceFragment.UpdateGroupIndex` se deriva de `FTileLODFragment`:
  - Hot  → Group 0
  - Warm → Group 1
  - Cold → Group 2

### Solve scheduling (futuro inmediato)
- Group 0: Solve cada frame
- Group 1: Solve cada 2 frames (o N)
- Group 2: Solve cada 4–8 frames (o M)

> Esto se integra perfecto con el “anim apply dirty”: si Cold no solvea siempre, igual no vas a spamear cambios.

---

## Checklist de implementación (paso a paso)

### Paso 1 — Fragmentos
- Agregar `FZombieStateFragment`
- Agregar `FAnimRequestFragment`
- (opcional) `FVelocityFragment`

### Paso 2 — Processor de estado
- Implementar `ZombieStateUpdateProcessor` en PrePhysics
- Verificar con debug:
  - `SpeedWS` y `LocoState` cambian como esperás

### Paso 3 — Selección de anim
- Implementar `ZombieAnimSelectProcessor` en PostPhysics
- Hardcodear 3 anims (Idle/Walk/Run) al principio
- Confirmar que `bDirty` solo se prende cuando cambias de estado

### Paso 4 — Apply a TS
- Implementar `TurboSequenceAnimApplyProcessor`
- Confirmar:
  - Si no hay cambios, no se llama `PlayAnimation`
  - Si cambia Walk→Run, se llama exactamente 1 vez

### Paso 5 — Integrar con LOD/UpdateGroups (opcional inmediato)
- Setear `UpdateGroupIndex` desde `TileLOD`
- Luego evolucionar `SolveProcessor` para schedule por grupo

---

## Constantes recomendadas (MVP)
- `EPS_SPEED = 1.0` (uu/s)
- `RUN_THRESHOLD = 250.0` (ajustalo al movimiento real)
- `EPS_PLAYRATE = 0.01`

> Mantenerlas en un header global / config para que sean consistentes.

---

## Notas para futuro Multiplayer (sin hacerlo ahora)
- Cambiar `DesiredAnim` (puntero) por `DesiredAnimId`:
  - una tabla fija `AnimId -> UAnimSequence*` cargada en runtime
- Replicar solo:
  - `LocoState`, `ActionState`, `AnimVariantId` (si querés variedad)
- La selección final de anim puede ser:
  - “determinista” en cliente (mismo mapeo),
  - o “autoritaria” si el servidor manda `AnimId`.

---

## Resultado esperado (MVP)
- Zombis con locomoción:
  - Idle cuando están parados / sin movimiento,
  - Walk cuando se mueven lento,
  - Run cuando están en chase (o superan umbral),
- Cambios de animación sin overhead por frame,
- Pipeline listo para:
  - Attack/Hit/Dead,
  - LOD por grupos,
  - replicación por IDs.

