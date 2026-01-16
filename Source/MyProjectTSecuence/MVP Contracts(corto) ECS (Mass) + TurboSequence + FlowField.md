# MVP — Resumen Ejecutivo y Orden de Implementación  
**ECS (Mass) + FlowField + TurboSequence**  
**Escalable a 10k–50k entidades | Enfoque por celda | Multiplayer-ready (diseño)**

---

## 1. Qué es este sistema (resumen corto)

Este MVP define un **pipeline claro y eficiente** para manejar hordas masivas usando:

- **ECS (Mass)** como fuente de verdad (lógica y movimiento)
- **FlowField** como guía de movimiento **por celda**
- **TurboSequence** solo para render y animación masiva

El diseño prioriza:
- costo **por tile/celda**, no por entidad
- determinismo (pensado para multiplayer)
- desacople total entre gameplay y render

---

## 2. Principios clave (no negociables)

- **ECS decide**  
  Estados, intención, movimiento y animación lógica.

- **FlowField orienta**  
  Da direcciones macro por celda. No conoce entidades.

- **TurboSequence muestra**  
  Solo visual. No toma decisiones de gameplay.

Reglas:
- ❌ No sample caro por entidad  
- ❌ No RNG global  
- ❌ No sync visual innecesario  
- ✅ Dirty flags + LOD por tile  
- ✅ Seeds deterministas  

---

## 3. Flujo real de una entidad (paso a paso)

1. **Transform → TileXY + CellIndex**  
   Solo si la entidad cambia de celda.

2. **Lectura de FlowField (por celda)**  
   - `Dir = DirField[cellIndex]`
   - Validación por `Epoch`
   - Sin cálculos pesados

3. **Dirección → Intención**
   - Idle / Walk / Run
   - Velocidad y dirección deterministas

4. **Integración de movimiento**
   - Simple
   - Sin física
   - Preferible FixedDeltaTime

5. **Derivación de estado de animación**
   - Estado abstracto
   - Seed determinista

6. **Sync visual**
   - Solo si cambió algo (Dirty)
   - Respetando LOD del tile

---

## 4. Qué vive dónde (ultra resumido)

### ECS (Mass)
- Transform
- MoveIntent
- SimMovement
- AnimState
- Determinism
- DirtyFlags

### FlowField / Grid
- DirectionField por celda
- Epochs
- HOT / WARM / COLD

### TurboSequence
- Render
- Animación
- Instancing
- LOD visual

---

## 5. Orden recomendado de implementación

### 🥇 Paso 1 — Grid + FlowField (sin entidades)
Antes de tocar ECS:

- TileXY y CellIndex bien definidos
- DirectionField por celda funcionando
- Epoch por tile/intención
- API: `TryGetFieldView(TileXY, Intent)`

**Objetivo:**  
Poder pedir *“dame la dirección de esta celda”* de forma determinista.

---

### 🥈 Paso 2 — UpdateCellLocationSystem
Primer system ECS real:

- Lee Transform
- Calcula TileXY + CellIndex
- Escribe solo si cambió de celda

**Objetivo:**  
Las entidades saben **dónde están en el grid**.

---

### 🥉 Paso 3 — FlowDirReadSystem
- Lee FlowCell + FlowQuery
- Lee FlowField (RO)
- Obtiene Dir + Epoch + bValid

**Objetivo:**  
Dirección **por celda**, sin lógica extra.

---

### 🏃 Paso 4 — MoveIntentSystem
- Convierte dirección → intención
- Maneja fallback (Idle si inválido)
- No mueve entidades

**Objetivo:**  
Separar **intención** de **resultado**.

---

### 🧱 Paso 5 — IntegrateMovementSystem
- Aplica movimiento simple
- Actualiza Transform y Velocity
- Marca Dirty con reglas claras

**Objetivo:**  
Movimiento barato, estable y reproducible.

---

### 🎭 Paso 6 — AnimStateSystem
- Deriva Idle / Walk / Run
- Usa seeds deterministas
- Marca Dirty solo si cambia

**Objetivo:**  
Animación coherente sin depender de TurboSequence.

---

### 🎨 Paso 7 — TurboSequenceSyncSystem
- Lee Transform + AnimState
- Respeta Dirty + LOD
- **No modifica ECS**

**Objetivo:**  
Render masivo sin romper performance.

---

## 6. Regla de oro para no desviarse

Cuando aparezca una duda, preguntarse:

> **¿Esto depende de la entidad o de la celda/tile?**

- Si es **por entidad** → debe ser liviano.
- Si es **por celda/tile** → ahí puede vivir lo pesado.
- Si es **visual** → va a TurboSequence, nunca a ECS.

---

## 7. En qué NO pensar todavía (fuera del MVP)

- Avoidance fino
- Colisiones físicas
- Steering complejo
- Replicación real
- Combate

Todo eso **se apoya sobre este sistema**, no forma parte del MVP.

---

## 8. Cierre

Este MVP define una base:

- simple
- determinista
- altamente escalable
- sin refactors futuros grandes

Una vez implementado, cualquier feature nueva
(hordas inteligentes, multiplayer, avoidance, combate)
**se enchufa arriba**, no rompe contratos.

---

### Próximo paso lógico (cuando estés listo)
Definir el **contrato de decisión de `FlowIntent`**
(Players / Ambient / Influences)  
sin introducir random per-entity.
