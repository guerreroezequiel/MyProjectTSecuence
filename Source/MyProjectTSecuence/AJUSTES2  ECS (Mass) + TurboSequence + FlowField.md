# Revisión integración ECS (Mass) + TurboSequence — estado actual y ajustes recomendados (online-friendly)

> Objetivo: validar si el “pipeline” ECS→(FlowField)→Move→TurboSequence está bien encaminado y marcar **cambios concretos** para que escale y sea “online-friendly”.

---

## 0) Foto rápida del pipeline (cómo está hoy)

### PrePhysics (sim / lógica)
1) `UpdateCellLocationProcessor`
- Lee `Transform`
- Escribe `CellLocation (TileXY + CellIndex + bValid)` **solo si cambió** ✅

2) `TileLODUpdateProcessor`
- Lee `CellLocation`
- Escribe `TileLOD (Hot/Warm/Cold)` ✅  
- **Hoy usa** `World->GetFirstPlayerController()` (single-player) ⚠️

3) `FlowDirReadPlayersProcessor`
- Lee `CellLocation`
- Pide a `FlowFieldStorageSubsystem::TryGetFieldView(TileXY, Players)`
- Escribe `FlowRead.DirWS + EpochSeen` con fallback de N frames ✅

4) `MoveIntegrateProcessor`
- Aplica `DirWS * Speed * dt` (opcional dt fijo) ✅

### PostPhysics (visual TS / render)
5) `TurboSequenceCleanupProcessor` (PostPhysics)
- Remueve instancias TS marcadas con `TSPendingCleanupTag` ✅

6) `TurboSequenceSyncProcessor` (PostPhysics)
- Crea instancias TS si faltan ✅
- **Gating Hot/Warm/Cold + eps** ✅
- **PERO: hoy no aplica el “sync” del transform cuando bDoSync=true** ❌ (bug crítico)

7) `TurboSequenceSolveProcessor` (PostPhysics)
- Llama `SolveMeshes_GameThread` 1 vez por frame/World con guard en `UTurboSequenceWorldSubsystem` ✅

---

## 1) Lo más importante: BUG crítico en `TurboSequenceSyncProcessor`

**Archivo:** `TurboSequenceSyncProcessor.cpp`

En el branch `else` (cuando `TS.bInstanceCreated == true`) se calcula:

- `bDirty` (eps pos/yaw)
- `bDoSync` según `Hot/Warm/Cold`

👉 **pero no se ejecuta ningún update** del mesh TS.

### Qué tenés que agregar (sin code, contrato)
Cuando `bDoSync == true`:
- llamar a TS para setear transform world space de la instancia (`SetMeshWorldSpaceTransform...`)
- actualizar el estado de gating:
  - `LastSyncedLocation = Loc`
  - `LastSyncedYaw = Yaw`
  - `LastSyncedFrame = GFrameCounter` (o frame id equivalente)

Y si tu plugin requiere que la instancia esté en un “update group”:
- asegurar que esté agregada al grupo al menos 1 vez, o re-validar cuando cambie LOD.

> Si no agregás esto, vas a ver: instancias creadas pero “congeladas” en el primer transform.

### Detalles menores en el mismo archivo
- `bColdEnabled` hoy se lee pero **no se usa** → o implementás el modo COLD o lo sacás por ahora.
- `DistSquared2D` está OK para isométrico, pero dejá comentado que es intencional (Z ignorado).

---

## 2) Riesgo de “doble storage”: `FlowFieldStorage.cpp` vs `FlowFieldStorageSubsystem`

Hoy conviven:
- `FlowFieldStorageSubsystem` con snapshots (`TSharedPtr<const FFieldSnapshot>`)
- `FlowFieldStorage.cpp` con `GStorage` global (`TMap` global)

**Problema:** es muy fácil que parte del proyecto escriba en un lado y ECS lea del otro (divergencia silenciosa).

### Ajuste recomendado (para escalar y para online)
Elegí **un solo backend runtime**:

**Opción A (recomendada):** Subsystem como “fuente de verdad”
- Deprecá `Grid::Flow::GStorage` (dejalo solo como compat o removelo)
- Toda publicación pasa por `UFlowFieldStorageSubsystem::Publish(...)`
- Toda lectura (ECS y no-ECS) pasa por `TryGetFieldView(...)`

**Opción B:** mantener `FlowFieldStorage` pero sin global
- Convertir `GStorage` en un miembro de un Subsystem/World object.
- Evitar `static` global para multiplayer/PIE/multiworld.

> Para “online-friendly”, evitá globals: el owner debe ser el `World` (o el `GameState` en el futuro).

---

## 3) `TileLODUpdateProcessor`: hoy es single-player (y te va a bloquear multiplayer)

**Archivo:** `TileLODUpdateProcessor.cpp`

Hoy decide Hot/Warm/Cold mirando **solo**:
- `World->GetFirstPlayerController()->GetPawn()`

### Contrato recomendado (online-friendly)
- El LOD debería depender de “players relevantes”:
  - servidor: lista de players (authoritative)
  - cliente: su pawn local (para render) + opcional otros (para coherencia visual)

Mínimo MVP online-ready:
- reemplazar “first player controller” por un **provider**:
  - `ITileLODSource` o un Subsystem que te entregue `TArray<FIntPoint> PlayerTiles`
- regla:
  - Hot si está en tile de algún player relevante
  - Warm si está dentro de N tiles (hoy N=1) de alguno

---

## 4) `UTurboSequenceWorldSubsystem`: buen enfoque, pero ojo con `ATurboSequence_Manager_Lf::Instance`

**Archivo:** `TurboSequenceWorldSubsystem.h`

✅ Bien:
- cachea Manager por World
- `ShouldSolveThisFrame` evita Solve duplicado

⚠️ Riesgo:
- `ATurboSequence_Manager_Lf::Instance = *It;` (global) puede ser un problema en PIE con múltiples Worlds.

### Ajuste recomendado
- Solo setear `Instance` si el plugin **lo exige** y documentarlo:
  - “TS usa singleton global, por eso lo seteamos desde el WorldSubsystem”.
- Si hay multiworld, evitar pisarlo:
  - loggear si ya está seteado a otro Manager
  - ideal: pedir al plugin una API “per world” si existe.

---

## 5) Spawner debug: bien para prototipar, pero para escalar conviene “Shared data”

**Archivo:** `EntityDebugSpawnerSubsystem.cpp`

✅ Bien:
- asegura que el entity tenga `FTurboSequenceInstanceFragment` y `FTileLODFragment`
- asigna `MeshAsset` para que Sync pueda crear instancia

Para escalar:
- `TS.MeshAsset = LoadSynchronous()` por entidad es caro si lo hacés masivo.
- Mejor:
  - usar un “shared fragment” o “const shared data” (un asset pointer común)
  - o cargar una vez y reutilizar el puntero.

---

## 6) Orden de processors: está casi bien, pero recomendación final

### PrePhysics
- `UpdateCellLocation` ✅ antes de LOD/Read
- `TileLODUpdate` ✅ antes de FlowDirRead
- `FlowDirReadPlayers` ✅ antes de MoveIntegrate
- `MoveIntegrate` ✅

### PostPhysics
- `TurboSequenceCleanup` ✅ antes de Solve (ya lo hiciste)
- `TurboSequenceSync` ✅ antes de Solve (ya lo hiciste)
- `TurboSequenceSolve` ✅ último

Recomendación: también hacer que `Cleanup` ejecute **antes** de `Sync`
- así evitás sincronizar instancias que vas a remover en el mismo frame.

---

azonamiento: Cleanup limpia; Sync crea/actualiza; Solve procesa lo vivo.

---

## 7) Checklist “online-friendly” (sin reescribir todo)

1) **Eliminar globals de runtime** (FlowFieldStorage global / TS singleton sin control).
2) **LOD source** basado en lista de players (no “first controller”).
3) **Movimiento determinista / replicable**:
   - tu `MoveIntegrate` con `FixedDT` es un buen paso ✅
   - a futuro: autoridad server, cliente solo interpolación/visual.
4) **Snapshots con epoch**:
   - `TryGetFieldView` ya devuelve `Epoch` combinado ✅
   - mantené la regla: “no hay invalidaciones mientras corre el tick”.

---

## 8) Acciones concretas (lo que yo cambiaría ya)

### Crítico (rompe visual)
- [ ] **TurboSequenceSyncProcessor:** aplicar transform cuando `bDoSync` es true (y actualizar LastSynced*).

### Para evitar deuda técnica (escala / coherencia)
- [ ] Definir **un solo storage** runtime (ideal: `FlowFieldStorageSubsystem`) y deprecar `FlowFieldStorage.cpp` global.
- [ ] Cambiar `TileLODUpdateProcessor` a un provider de player tiles (preparado para 4 jugadores).

### Pulido (performance)
- [ ] Evitar `LoadSynchronous()` por entidad en el spawner: asset compartido/cargado una vez.
- [ ] Reordenar PostPhysics: `Cleanup -> Sync -> Solve`.

---

Si querés, en el próximo paso te armo un `.md` “contratos finales” (sin código) para:
- `TryGetFieldView` (lifetime/thread safety)
- `TS Sync Gating` (Hot/Warm/Cold + eps + periodic)
- `TileLOD Source` (single player hoy, multiplayer mañana)
con una tablita de responsabilidades por módulo.
