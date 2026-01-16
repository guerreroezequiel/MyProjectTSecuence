# MVP Contracts — ECS (Mass) + TurboSequence + FlowField  
**Enfoque simplista, por celda, escalable a decenas de miles de entidades**  
**Multiplayer-ready en diseño (sin red aún)**

---

## 0. Objetivo del MVP

Definir contratos **claros, mínimos y eficientes** para integrar:

- **ECS (Mass)** → lógica, estado y movimiento
- **FlowField** → dirección macro **por celda**
- **TurboSequence** → render y animación masiva

Con foco explícito en:
- **muchísimas entidades (10k–50k)**
- costo **por tile/celda**, no por entidad
- arquitectura determinista y replicable

---

## 1. Principio rector de performance

> **El trabajo pesado se hace por tile o por celda**  
> **El trabajo liviano se hace por entidad**  
> **El render solo se actualiza si es necesario**

Reglas clave:
- El FlowField **ya contiene direcciones por celda**
- Las entidades **no recalculan FlowField**
- ECS solo **lee datos cacheados**
- TurboSequence solo recibe **cambios visuales**

---

## 2. Ownership

### 2.1 ECS / Mass (fuente de verdad)
Dueño de:
- Transform
- Velocity
- Estado lógico
- Intención de movimiento
- Estado de animación (abstracto)
- Lifecycle

> Replicable a futuro.

### 2.2 FlowField / Grid
Dueño de:
- Costos
- DirectionField **por celda**
- Epochs
- HOT / WARM / COLD

Contrato:
- Read-only desde ECS
- Determinista
- Sin estado por entidad

### 2.3 TurboSequence
Dueño de:
- Render
- Animación
- LOD
- Instancing

Contrato:
- No decide gameplay
- No conoce FlowField
- Cliente-only a futuro

---

## 3. Contratos de Datos (ECS Fragments)

### 3.1 Identidad visual

**FTSInstanceHandle**
- InstanceId
- bValid

**FTSRenderArchetype**
- ProfileId
- VariantId (opcional)

---

### 3.2 Movimiento

**FTransformFragment**
- TransformWS

**FSimMovementFragment**
- VelocityWS
- bHasMovement

> Resultado simulado (simple, determinista)

**FMoveIntentFragment**
- DesiredDirWS
- DesiredSpeed
- MoveMode (Idle / Walk / Run)

> Intención lógica, no física

---

### 3.3 FlowField — enfoque por celda (sin sample “costoso”)

**FFlowQueryFragment**
- FlowIntent (Players / Influences / Ambient)

Contrato:
- Solo define *qué* FlowField usar
- No guarda TileXY ni CellXY

**FFlowCellFragment**
- TileXY
- CellIndex

Contrato:
- Derivado desde Transform
- Se actualiza **solo si la entidad cambia de celda**
- Definición de “cambia de celda”: cambia `CellIndex` o `TileXY`

**FFlowDirFragment**
- DirWS
- EpochSeen
- bValid

Contrato:
- Lectura directa de `DirField[CellIndex]`
- Cache liviano por entidad
- NO replicable
- `EpochSeen` siempre proviene del storage (fuente única)
- `bValid=false` si:
  - `TryGetFieldView(...).bValid == false`, o
  - `CellIndex` está fuera de rango

Opcional:
- Puede omitirse y leerse directo del DirField (mantener solo para debug/telemetría)

---

### 3.4 Determinismo

**FDeterminismFragment**
- EntitySeed
- SpawnEpoch / SpawnTick

Contrato:
- Fuente única de seeds
- Prohibido `rand()` runtime (RNG global)

---

### 3.5 Animación (abstracta)

**FAnimStateFragment**
- State (Idle / Walk / Run / Attack / Dead)
- Speed01
- AnimSeed (derivado del EntitySeed)

Contrato:
- Estado lógico
- Replicable
- TurboSequence solo interpreta

---

### 3.6 Visual Sync

**FTSDirtyFragment**
- bTransformDirty
- bAnimDirty

Contrato:
- Controla cuándo se sincroniza con TS
- Evita updates innecesarios

---

## 4. Sistemas (orden determinista)

1. UpdateGridContextSystem *(por tile, opcional)*
2. UpdateCellLocationSystem
3. FlowDirReadSystem
4. MoveIntentSystem
5. IntegrateMovementSystem
6. AnimStateSystem
7. TurboSequenceSyncSystem *(client-only)*

Notas de ejecución (UE/Mass sugerido):
- Systems 1–6 en PrePhysics
- System 7 en PostPhysics (cliente)
- Declarar dependencias explícitas para mantener el orden arriba indicado

---

## 5. Sistemas (responsabilidades)

### 5.1 UpdateCellLocationSystem
**Lee**
- Transform

**Escribe**
- FlowCell

Contrato:
- Convierte Transform → TileXY + CellIndex
- Solo escribe si cambió TileXY o CellIndex

---

### 5.2 FlowDirReadSystem
**Lee**
- FlowCell
- FlowQuery
- FlowFieldStorage (RO)

**Escribe**
- FlowDir (si existe)

Contrato:
- Debe obtener `DirFieldPtr` y `Epoch` en **una única consulta lógica**
- `DirWS = DirFieldPtr[CellIndex]` (lectura directa)
- `EpochSeen = Epoch` devuelto por la consulta
- `bValid = bValidDevuelto && CellIndexEnRango`

---

### 5.3 MoveIntentSystem
**Lee**
- FlowDir (o lectura directa del DirField)

**Escribe**
- MoveIntent

Contrato:
- Si `FlowDir.bValid == false`:
  - `MoveMode=Idle`
  - `DesiredSpeed=0`
  - `DesiredDirWS=(0,0,0)`
- Si es válido:
  - `DesiredDirWS` unitario (si magnitude ~0 → Idle)
  - `DesiredSpeed`/`MoveMode` definidos de forma determinista
- Variaciones (opcional) solo con `EntitySeed` (sin RNG global)

---

### 5.4 IntegrateMovementSystem
**Lee**
- MoveIntent
- Transform

**Escribe**
- Transform
- SimMovement
- TSDirty

Contrato:
- Movimiento simple, sin física
- Recomendado: `FixedDeltaTime` para reproducibilidad
- Si se usa `DeltaSeconds`: loggear/telemetría

**Regla de Dirty (definida)**
- `bTransformDirty=true` solo si:
  - cambió de celda (TileXY/CellIndex), o
  - `|DeltaPos| > EPS_POS`, o
  - `|DeltaYaw| > EPS_YAW`
- `EPS_POS` y `EPS_YAW` son constantes del proyecto (no “a criterio”).
- Gating por LOD:
  - en tiles WARM/COLD no se marca dirty salvo cambio de celda

---

### 5.5 AnimStateSystem
**Lee**
- SimMovement
- MoveIntent
- Determinism

**Escribe**
- AnimState
- TSDirty (bAnimDirty)

Contrato:
- Estado derivado de velocidad/intención
- Variaciones solo con `EntitySeed`
- `bAnimDirty` solo si cambió `State` o `Speed01` cruzó umbral (constante)

---

### 5.6 TurboSequenceSyncSystem
**Lee**
- Transform
- AnimState
- RenderArchetype
- TSDirty
- TileLOD (HOT/WARM/COLD)

Contrato:
- Aplica solo cambios (Dirty)
- Respeta LOD por tile
- No modifica ECS

---

## 6. LOD de simulación (definición cerrada)

**Por tile (no por entidad):**
- HOT  → systems 2–7 corren cada tick
- WARM → systems 2–6 corren cada `N_WARM` ticks; system 7 solo si Dirty
- COLD → system 2 corre solo si cambia de celda/tile; 3–6 se omiten; 7 se omite salvo spawn/despawn

Constantes:
- `N_WARM` es constante del proyecto (ej: 4, 8, 16) y no se decide por entidad.

---

## 7. Contrato mínimo FlowFieldStorage

API requerida:

- TryGetFieldView(TileXY, FlowIntent) → `{ DirFieldPtr, Epoch, bValid }`

Contrato:
- Stateless
- Determinista
- Safe server/client
- `Epoch` centralizado por tile/intención es la fuente de validez
- La lectura de `Epoch` y `DirField` se resuelve en una sola consulta lógica

---

## 8. Enfoque Multiplayer (preparado)

Decisiones ya cerradas:
- ECS = autoridad
- FlowField = servicio compartible
- MoveIntent + AnimState = replicables
- FlowDir = cache local / debug
- TurboSequence = cliente-only

---

## 9. Checklist final

- [ ] No se samplea FlowField con lógica pesada por entidad
- [ ] Dirección se lee por celda
- [ ] Integración simple y batch-friendly
- [ ] Visual sync solo si hay cambios (Dirty + eps)
- [ ] LOD por tile definido (HOT/WARM/COLD con reglas cerradas)
- [ ] Random solo con seeds deterministas
- [ ] Orden y fases de tick fijas (PrePhysics/PostPhysics) con dependencias
- [ ] Fallback seguro si FlowDir no es válido (Idle/0)
- [ ] CVars/telemetría para direcciones, velocidades, estados y epochs

---

## Principio final

> **El tile decide el camino**  
> **La celda da la dirección**  
> **La entidad solo avanza**  
> **TurboSequence solo muestra**
