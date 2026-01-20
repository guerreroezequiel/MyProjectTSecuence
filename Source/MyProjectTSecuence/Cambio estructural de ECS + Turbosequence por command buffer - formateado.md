# TurboSequence + ECS (Mass) — Integración correcta y Online-Friendly

> **Objetivo**
> Definir la forma **correcta, robusta y escalable** de integrar **TurboSequence** con **ECS (Mass)**, evitando crashes en PIE, respetando el lifecycle del World y siendo compatible con multiplayer.

Este documento **no depende de tu implementación actual**: define el **patrón correcto** que usan los motores en producción cuando integran un sistema de render/animación externo con ECS.

---

## 1. Estado real del arte (qué existe y qué no)

### ❌ Lo que NO existe en internet
- No hay ejemplo oficial de *TurboSequence + Mass*
- No hay pipeline ECS publicado por el autor del plugin
- No hay soporte multiplayer explícito

### ✅ Lo que SÍ existe (implícito en el diseño del plugin)
- TurboSequence es un **World-level render backend**
- Usa un **Manager Actor** + **singleton global**
- Está pensado para **batching masivo**, no para control por entidad

👉 Conclusión:
**TurboSequence NO es un “component system”**. Debe integrarse como **backend**, no como gameplay logic.

---

## 2. Principio fundamental (regla de oro)

> **ECS describe intención.**  
> **TurboSequence ejecuta en batch.**  
> **Un único owner decide cuándo se llama al plugin.**

Nadie fuera de ese owner debe llamar a TurboSequence.

---

## 3. Arquitectura correcta (visión general)

[ECS / Mass]
↓ (intención, datos)
[Command Build Processor]
↓ (colas de comandos)
[TurboSequence World Subsystem] ← ÚNICO owner
↓ (flush seguro)
[TurboSequence Plugin]

---

## 4. Responsabilidades claras por capa

### 4.1 ECS (Mass)
- Mantiene **solo datos**
- Decide:
  - qué entidades necesitan TS
  - cuándo deben actualizarse
  - cuándo ya no deberían existir
- **Nunca llama al plugin**

#### Fragmentos típicos
- `FTurboSequenceInstanceFragment`
- `FTransformFragment`
- `FTileLODFragment`

#### Tags de intención (no ejecutan lógica)
- `FTSNeedsCreateTag`
- `FTSNeedsDestroyTag`
- (opcional) `FTSDirtyTransformTag`

---

### 4.2 Processors ECS

#### A) Command Build Processor (PostPhysics, GameThread)

Responsabilidad:
- Recorrer entidades
- Traducir estado ECS → **intención**
- Escribir en colas de comandos del Subsystem

Hace:
- `RequestCreate`
- `RequestUpdate`
- `RequestDestroy`

No hace:
- ❌ Llamar a TurboSequence  
- ❌ Resolver Manager  
- ❌ Tocar singletons

---

### 4.3 World Owner (pieza central)

#### `UTurboSequenceWorldSubsystem`
Este es el **único punto de contacto** con TurboSequence.

Contiene:
- Estado del World
- Referencia al Manager
- Colas de comandos
- Política de teardown

```cpp
enum class ETSWorldState
{
  Running,
  ShuttingDown
};
```

#### Colas
- CreateCommands
- UpdateCommands
- DestroyCommands

### 4.4 Flush único (la parte crítica)
`TurboSequenceCommandFlushProcessor`

_(PostPhysics, GameThread, último en orden)_

Es el único lugar donde se llama al plugin.

**Flujo**
- Verificar estado del World (Running)
- Validar World y Manager
- Garantizar coherencia del singleton
- Ejecutar comandos en orden: Destroy → Create → Update
- Solve (1 vez por frame)
- Limpiar colas

Garantiza:
- **Cero llamadas dispersas**
- **Cero efectos colaterales en teardown**

---

## 5. Manejo correcto del lifecycle (PIE-proof)

### 5.1 Estado explícito del World

El Subsystem escucha:
- `OnWorldBeginTearDown`
- `OnWorldCleanup`

Cuando ocurre:
- `State = ShuttingDown`
- Se bloquea cualquier Flush
- No se llama al plugin

Esto evita:
- último frame peligroso
- llamadas tardías al plugin
- crashes por singleton inválido

### 5.2 Guards reales (no cosméticos)

En el Flush (todos deben cumplirse):
- `World->IsGameWorld()`
- `!World->bIsTearingDown`
- `!GIsRequestingExit`
- `IsValid(Manager)`
- `!Manager->IsPendingKill()`
- `!Manager->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed)`

Y solo ahí.

---

## 6. Multiplayer / Online-Friendly

**Qué NO se replica**
- Instancias TurboSequence
- Animaciones
- MeshData
- LOD de render

**Qué SÍ**
- Posición / rotación
- Estado lógico (Idle / Chase / Dead)
- Epochs / seeds deterministas (si aplica)

TurboSequence es 100% client-side.

Este patrón:
- evita divergencias
- es determinista
- escala bien con cientos/miles de entidades

---

## 7. Por qué este patrón es el correcto

Señales claras del plugin:
- Singleton global
- API batch
- Solve explícito
- World-level manager

Todo indica: “Usame como backend, no como componente”.

---

## 8. Beneficios inmediatos

- ❌ No más crashes al cerrar PIE
- ✅ Un solo lugar para debug
- ✅ Menos llamadas por frame
- ✅ Fácil de perfilar
- ✅ Escalable (10k–50k entidades)
- ✅ Multiplayer-safe

---

## 9. Regla final

Si un sistema de render/animación puede romper PIE, no es un bug: es una señal de mala integración.

La solución correcta es arquitectónica, no defensiva.

---

## 10. Próximo paso recomendado

Aplicar esta estructura a tu repo actual:
- fusionar Cleanup / Sync / Solve
- mover lógica al Subsystem
- dejar ECS como declarativo
