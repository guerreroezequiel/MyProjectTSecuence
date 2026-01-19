
# ECS + TurboSequence + FlowField  
## Contrato Escalable y Online-Friendly — Progreso Implementado

Este documento resume los cambios aplicados para alinear el sistema (ECS + FlowField + TurboSequence) con el contrato escalable, thread-safe y multi-World.

---

## 1) Storage por World con snapshots inmutables

- Creado `UFlowFieldStorageSubsystem : UWorldSubsystem`.
- API pública:
  - `TryGetFieldView(TileXY, Intent)` devuelve `{ DirPtr, Epochs, bValid }` de un snapshot inmutable.
  - `Publish(TileXY, Intent, Dir, StaticEpoch, GoalsEpoch)` y overload con `Dist` opcional.
- Internamente mantiene `Storage` per-World: `(TileXY -> Intent -> Snapshot)`.
- `TryGetFieldView` lee primero del snapshot per-World y, temporalmente, tiene fallback al storage global.

## 2) FlowFieldStorage (global) desacoplado

- Creado `FlowFieldStorage.cpp` y migradas todas las implementaciones desde el header.
- `GStorage` definido en el .cpp. La API pública no cambió.

## 3) Publicación tras Rebuild

- `UFlowFieldSystem::RebuildFlowField` publica el snapshot al Subsystem per-World luego de reconstruir el campo.

## 4) Lectores migrados al Subsystem

- `FlowDirReadPlayersProcessor` lee solo desde `UFlowFieldStorageSubsystem` (sin fallback). Si no hay snapshot válido, deja `bValid=false`.
- `UFlowFieldMovementComponent` migra la lectura de dirección al Subsystem; si no hay snapshot, retorna `ZeroVector`.

## 5) TurboSequence: Solve per-World

- Creado `UTurboSequenceWorldSubsystem` con guard por World (frame counter por World).
- `TurboSequenceSolveProcessor` usa el guard per-World y deja de usar guard global estático.

---

## Estado actual

- Compila correctamente.
- Lectores principales usan snapshots per-World.
- Snapshots incluyen `Dir` y meta (epochs). Overload de Publish permite `Dist` si se necesita.

## Próximos pasos

- Remover definitivamente el fallback al storage global y eliminar `GStorage` una vez que los writers publiquen snapshots directamente.
- Exponer una vista para `Dist` si algún lector lo requiere (par a `TryGetFieldView`).
- Implementar LOD del Sync en TurboSequence (HOT/WARM/COLD) y `UMassObserverProcessor` para cleanup automático de instancias TS.

