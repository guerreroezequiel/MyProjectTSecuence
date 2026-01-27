## PARTE TRES

# MVP Animaciones — TurboSequence + ECS
## Idle / Walk (loop) basado en movimiento

## Objetivo
Implementar un sistema de animaciones mínimo pero sólido que:

- reproduzca **Walk (loop)** mientras la entidad se mueve
- vuelva a **Idle (loop)** cuando está quieta
- evite flickering (Idle ↔ Walk)
- minimice llamadas a TurboSequence
- sea escalable a futuro (ataques, hit, LOD, blendspaces)

---

## 1) Contratos del sistema

### 1.1 Fragments requeridos

#### Movimiento
**FMoveFragment**
- `FVector Velocity`  
  *(o Speed directo si ya lo calculás)*

#### Estado de locomoción (nuevo, mínimo)
**FAnimLocomotionFragment**
- `EAnimLocomotionState CurrentState`
  - `Idle`
  - `Walk`
- `float LastPlayRate`

#### Request de animación
**FAnimRequestFragment**
- `UAnimSequence* DesiredAnim`
- `float PlayRate`
- `bool bLoop`
- `bool bDirty`

#### TurboSequence
**FTurboSequenceFragment**
- `int32 InstanceIndex`
- `int32 UpdateGroupIndex`
- `bool bHasInstance`

---

## 2) Tabla de animaciones (AnimDB MVP)

Definición mínima (struct, data asset o hardcodeado):

| Key  | Asset      |
|-----|------------|
| Idle | IdleAnim   |
| Walk | WalkAnim   |

Sin lógica. Solo referencias.

---

## 3) Parámetros y constantes del MVP

### 3.1 Umbrales de velocidad (histeresis)

```cpp
ENTER_WALK_SPEED = 10.0f;   // uu/s
EXIT_WALK_SPEED  = 6.0f;    // uu/s
```

Reglas:

- Idle → Walk si `Speed >= ENTER_WALK_SPEED`
- Walk → Idle si `Speed <= EXIT_WALK_SPEED`

Esto evita parpadeo.

### 3.2 PlayRate

```cpp
WalkSpeedRef = 150.0f;   // velocidad base de la anim
MinRate = 0.8f;
MaxRate = 1.25f;

PlayRate = clamp(Speed / WalkSpeedRef, MinRate, MaxRate);
```

Idle siempre:

```cpp
PlayRate = 1.0f;
```

### 3.3 Epsilon para evitar spam

```cpp
EPS_PLAYRATE = 0.05f;
```

Solo marcar dirty si:

```cpp
abs(NewRate - LastRate) >= EPS_PLAYRATE
```

---

## 4) Processor: ZombieAnimSelectProcessor

### Responsabilidad

- Decidir qué animación debería estar activa.
- No habla con TurboSequence.

### Inputs

- `FMoveFragment`
- `FAnimLocomotionFragment`
- AnimDB
- `FAnimRequestFragment`

### Outputs

- Escribe `FAnimRequestFragment`
- Actualiza `FAnimLocomotionFragment`

### Lógica

Calcular velocidad:

```cpp
Speed = Velocity.Size();
```

Determinar estado usando histeresis:

- Si `Current == Idle` y `Speed >= ENTER_WALK_SPEED` → Walk
- Si `Current == Walk` y `Speed <= EXIT_WALK_SPEED` → Idle
- Si no → mantener estado

Resolver animación:

| State | Anim     | Loop | PlayRate   |
|------:|----------|:----:|------------|
| Idle  | IdleAnim  | true | 1.0        |
| Walk  | WalkAnim  | true | calculado  |

Marcar Dirty si:

- cambió la anim (Idle ↔ Walk)
- o el playrate cambió más de EPS

Guardar:

- `CurrentState`
- `LastPlayRate`

---

## 5) Processor: TurboSequenceAnimApplyProcessor

### Responsabilidad

Aplicar requests dirty a TurboSequence.

### Reglas

Ejecutar solo si:

- `TS.bHasInstance == true`
- `AnimRequest.bDirty == true`
- `DesiredAnim != nullptr`

### Acción

Llamar a `PlayAnimation_Concurrent(...)` con:

- Anim
- Loop
- PlayRate

`StartTime = 0` solo si cambió la anim.

Limpiar:

```cpp
AnimRequest.bDirty = false;
```

Este processor no decide lógica.

---

## 6) Orden correcto de processors

- Movimiento (actualiza Velocity)
- `ZombieAnimSelectProcessor`
- `TurboSequenceAnimApplyProcessor`
- `TurboSequenceSolveProcessor` (1 vez por frame por UpdateGroup)

---

## 7) Fuera de alcance del MVP

- BlendSpaces
- Ataques / Hit / Death
- Upper-body layers
- ForceRestart
- LOD de animación
- Transiciones complejas

---

## 8) Checklist MVP

- Walk loop mientras Speed > threshold
- Idle estable cuando está quieto
- Sin flicker Idle ↔ Walk
- PlayRate acompaña la velocidad
- No spam de llamadas a TurboSequence
- Solve corre 1 vez por frame