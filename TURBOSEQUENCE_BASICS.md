# TurboSequence — Integración Correcta (Post Asset Setup)

Este documento describe **la forma exacta de integrar TurboSequence** una vez creado el
`TurboSequence Mesh Asset`, siguiendo el patrón recomendado por el plugin y validado en prototipos previos.

El objetivo es:
- Integrar TurboSequence con un pipeline ECS / processors
- Mantener batching correcto
- Evitar lógica de estado innecesaria
- Ser escalable y multiplayer-friendly

---

## 0. Requisito obligatorio: TurboSequence Manager

TurboSequence requiere **un Manager actor singleton** en el mundo:

- `ATurboSequence_Manager_Lf`

### Opciones válidas
- Colocarlo manualmente en el level  
- Spawnearlo automáticamente en `GameMode::BeginPlay` si no existe

Recomendación:
> Spawneo automático para evitar errores al cambiar de mapa.

Sin este actor, **ninguna instancia se renderiza ni anima**.

---

## 1. Setup mínimo por entidad (una sola vez)

Esto ocurre **al spawnear la entidad visual** (zombie, NPC, etc).

### Orden correcto (obligatorio)

1. **Crear la instancia TurboSequence**
   - `AddSkinnedMeshInstance_GameThread(...)`
   - Devuelve un handle (`RootMotionMeshID` / MeshData)

2. **Asignar la instancia a un Update Group**
   - `AddInstanceToUpdateGroup_Concurrent(GroupIndex, MeshData)`

3. **Reproducir una animación inicial**
   - `PlayAnimation_Concurrent(MeshData, AnimSequence, Settings)`

### Reglas MVP
- Usar siempre `GroupIndex = 0`
- No redistribuir grupos todavía
- Guardar el `MeshData` en un fragment/struct por entidad

---

## 2. Patrón de ejecución por frame (clave del sistema)

TurboSequence **no se actualiza automáticamente**.
Debe resolverse explícitamente cada frame.

### Regla fundamental
> `SolveMeshes_GameThread(...)`  
> debe llamarse **una vez por frame y por update group**.

---

## 3. Separación correcta de responsabilidades

### Fase A — ECS / Processors (multithread-friendly)
Los processors:
- Calculan movimiento / rotación
- Deciden animación (si aplica)
- **NO llaman a TurboSequence**

Solo marcan datos pendientes:

- `PendingTransform`
- `bNeedsTransformUpdate`
- `bNeedsAnimationUpdate`

---

### Fase B — TurboSequence Apply + Solve (GameThread)

Un único processor o subsystem al final del frame hace:

1. Iterar entidades con flags pendientes
2. Aplicar cambios en batch:
   - `SetMeshWorldSpaceTransform_Concurrent(...)`
   - `PlayAnimation_Concurrent(...)` (si corresponde)
3. Limpiar flags
4. Ejecutar:
   - `SolveMeshes_GameThread(DeltaTime, World, UpdateContext)`

---

## 4. Update Groups (concepto, no MVP)

Los Update Groups permiten:
- Actualizar unidades lejanas menos seguido
- Reducir carga de CPU

### Importante
- El sistema **NO gestiona grupos automáticamente**
- Vos debés:
  - mover instancias entre grupos
  - acumular `DeltaTime` si un grupo no se resuelve cada frame

### Estado actual
> ❌ No usar en MVP  
> ✔ Todos los zombies en Group 0

---

## 5. Assets y materiales (condiciones necesarias)

### TurboSequence Mesh Asset
Debe tener:
- Skeletal Mesh Archetype válido
- Static Mesh generado
- Animation Library configurada (Idle / Walk mínimo)

### Material
- Nodo TurboSequence conectado a `World Position Offset`
- Normal recalculada correctamente

Si esto falla:
- Instancias invisibles
- Deformaciones
- SolveMeshes sin efecto visible

---

## 6. Qué NO hacer

- ❌ Llamar `SolveMeshes_GameThread` desde múltiples lugares
- ❌ Mezclar lógica de estados con TurboSequence
- ❌ Actualizar TurboSequence por entidad
- ❌ Distribuir grupos prematuramente

---

## 7. Resultado esperado del MVP

- Instancias visibles
- Movimiento aplicado correctamente
- Animaciones reproducidas
- Un solo SolveMeshes por frame
- Pipeline limpio y escalable

---

## 8. Archivos clave a consultar
- `TurboSequence_Manager_Lf.h`
- `TurboSequence_MinimalData_Lf.h`

Ahí está la API completa del sistema.
