# ECS + TurboSequence + FlowField  
## Contrato Escalable y Online-Friendly

Este documento define los **cambios estructurales correctos** para que el sistema actual
(ECS + FlowField + TurboSequence) sea:

- thread-safe
- Mass-parallel friendly
- multi-World / PIE safe
- preparado para multiplayer (host-authority / determinismo)
- escalable a miles de entidades

El objetivo **NO** es refactorizar lógica de gameplay, sino **corregir ownership, lifetime y concurrencia**.

---

## 1. Principio base (Online-friendly)

> **Las entidades no generan decisiones.  
Las entidades consumen datos del mundo.**

- No hay random per-entity.
- No hay estado global compartido.
- Toda lectura ECS es **read-only, inmutable y determinista**.
- El World es la unidad de autoridad (no globals).

---

## 2. Problema actual (resumen)

### 2.1 Storage global
- `FlowFieldStorage` usa un `TMap` global.
- Mezcla Worlds (PIE / multiplayer).
- No es thread-safe.

### 2.2 Lecturas en paralelo
- `FlowDirReadPlayersProcessor` puede correr en worker threads.
- Lee estructuras mutables → data race.

### 2.3 TurboSequence
- Solve protegido con `static LastSolvedFrame` (global).
- Sync por entidad sin LOD (no escala).

---

## 3. Cambio estructural clave: **Storage por World**

### 3.1 Nuevo owner
Crear:

- `UFlowFieldStorageSubsystem : UWorldSubsystem`

Responsabilidad:
- Ser el **único owner** de los FlowFields runtime de ese World.
- Garantizar lifetime, concurrencia y coherencia.

Resultado:
- Un storage por World.
- PIE y multiplayer seguros.
- No hay estado global compartido.

---

## 4. Modelo de datos escalable: **Snapshots inmutables**

### 4.1 Regla de oro
> **Lo que lee Mass NO se puede modificar.**

### 4.2 Snapshot
Cada `(TileXY, Intent)` publica un snapshot completo:

- DistField (costos)
- DirField (direcciones)
- Epochs asociados

Características:
- Inmutable
- Cache-friendly
- Publicado por swap atómico

### 4.3 Patrón recomendado
- El solver construye un snapshot nuevo.
- Cuando termina:
  - `Publish(TileXY, Intent, Snapshot)`
  - swap atómico
- Los readers solo leen memoria estable.

Resultado:
- Lecturas lock-free
- Mass puede correr en paralelo
- Determinismo (todos leen el mismo snapshot ese frame)

---

## 5. API formal del Storage

### 5.1 Escritura (solver)
Responsabilidad exclusiva del sistema de rebuild.

Contrato:
- Construye snapshot completo fuera del storage.
- Publica snapshot completo (nunca parcial).

No:
- No escribir por celda.
- No mutar datos visibles a readers.

### 5.2 Lectura (Mass)
Función única y atómica:

TryGetFieldView(TileXY, Intent)
→ { DirPtr, Epoch, bValid }


Garantías:
- `DirPtr` apunta a memoria inmutable.
- Válido al menos durante el tick actual.
- Thread-safe.
- World-scoped.

---

## 6. Implicancias para ECS (Mass)

### 6.1 Processors paralelizables
Una vez aplicado el snapshot:

- UpdateCellLocationProcessor ✅
- FlowDirReadPlayersProcessor ✅ (sin GameThread)
- MoveIntegrateProcessor ✅

Regla:
- Ninguno toca GameThread.
- Ninguno escribe en sistemas compartidos.

---

## 7. TurboSequence — Cambios para escalar

### 7.1 Solve once per frame (World-safe)

Problema:
- Guard global por frame.

Solución:
- Guard **por World**, no estático global.
- Cada World resuelve TS una vez por frame.

Resultado:
- PIE / multiplayer seguros.

---

### 7.2 Sync con LOD (HOT / WARM / COLD)

Problema:
- Sync por entidad cada frame no escala.

Contrato:
- HOT → sync cada frame
- WARM → sync cada N frames
- COLD → no sync / culled

Fuente:
- TileLOD (ya existente en el grid system).

Resultado:
- TS escala a miles de instancias.

---

## 8. Cleanup robusto de instancias TurboSequence

### Problema actual
- Cleanup manual desde spawner.

Cambio correcto:
- Usar `UMassObserverProcessor`
- Escuchar:
  - Entity destroyed
  - Fragment `FTurboSequenceInstanceFragment` removido

Responsabilidad:
- Remover instancia TS automáticamente.
- Evitar leaks en gameplay real.

---

## 9. Enfoque Multiplayer (futuro)

Este diseño permite:

- Host-authority:
  - El host calcula FlowFields.
  - Publica snapshots deterministas.
- Clients:
  - Solo consumen snapshots.
  - Movimiento coherente y reproducible.
- Rewind / debug:
  - Epochs por snapshot permiten inspección histórica.

No hay:
- Random local
- Estado oculto por entidad
- Dependencia del frame rate

---

## 10. Orden recomendado de implementación

1. Crear `UFlowFieldStorageSubsystem`
2. Eliminar storage global
3. Implementar snapshots inmutables + publish
4. Migrar readers ECS al subsystem
5. Hacer TurboSequence Solve por World
6. Agregar LOD al Sync
7. Agregar Observer de cleanup TS

---

## 11. Resultado final

Con estos cambios:

- Mass ECS corre en paralelo sin riesgos
- FlowField es determinista y online-ready
- TurboSequence escala correctamente
- El sistema queda preparado para multiplayer real

Este es el **mínimo diseño correcto** para un juego de hordas grande.
