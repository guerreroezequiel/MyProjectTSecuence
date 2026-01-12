# FlowField System — Player Integration (PR1)

## Objetivo
Conectar el jugador con el FlowFieldSystem para que:
- el sistema conozca la posición del jugador,
- genere flowfields hacia él (intent `Players`),
- y las entidades naveguen sin conocer al jugador directamente.

El jugador no ejecuta lógica de navegación.

---

## Principio Fundamental
El jugador **no conoce** el FlowField.  
El FlowField **conoce al jugador** como una fuente de objetivos (goals).

Esto evita acoplamientos y permite escalar a multiplayer e influencias futuras.

---

## Roles y Responsabilidades

### Player (Pawn / Character)
- Existe en el mundo y se mueve.
- Expone su posición en coordenadas Unreal.
- No:
  - marca tiles dirty,
  - crea goals,
  - llama al FlowFieldSystem.

---

### FlowFieldSystem
Es el orquestador central.

Responsabilidades:
- Obtener referencia al jugador.
- Leer su posición.
- Convertir posición → Cell → Tile.
- Mantener HOT/WARM.
- Construir el GoalSet para el intent `Players`.
- Ejecutar el pipeline:
  - UpdateEpochs
  - Bake (si corresponde)
  - IsValid / Rebuild
- Escribir resultados en FlowFieldStorage.

---

### Entidades (AI / crowd)
- No conocen al jugador.
- No conocen goals.
- Solo leen FlowFieldStorage para decidir dirección.

---

## Obtención del Jugador

### Momento
- `FlowFieldSystem::BeginPlay()`

### Acción
- Obtener y cachear referencia al jugador:
  - Player 0 (single-player MVP),
  - o Actor con Tag `"Player"`.

La referencia se usa solo para lectura de posición.

---

## Conversión de Posición del Jugador

### Proceso
1. Leer `PlayerWorldPosition`.
2. Convertir:
   - World → Cell
   - Cell → Tile
3. Obtener `PlayerTileXY`.

Este valor define:
- el tile HOT,
- el origen de los goals.

---

## HOT / WARM Update (ligado al jugador)

### Regla
- HOT = `PlayerTileXY`
- WARM = vecinos Chebyshev 1 del HOT

### Detección de cambio
- Si `PlayerTileXY` cambia:
  - actualizar HOT/WARM,
  - marcar `GoalsDirty(Players)` en tiles ACTIVE (HOT + WARM).

Esto fuerza la invalidación y rebuild del flowfield.

---

## Construcción del GoalSet (intent Players)

### Responsable
- FlowFieldSystem.

### Contenido (PR1)
- Un solo goal:
  - la celda donde se encuentra el jugador.

### Reglas
- El GoalSet:
  - no vive en el Player,
  - no se cachea globalmente,
  - se reconstruye cuando el jugador cambia de tile.

---

## Integración con el Pipeline

Para cada tile ACTIVE:

1. `TileContext.UpdateEpochs()`
2. Si cambió `StaticCostEpoch`:
   - bake de `FinalCost_Static`
3. Para intent `Players`:
   - si `!FlowField.IsValid()`:
     - ejecutar `Rebuild` usando el GoalSet del jugador

---

## Escritura y Consumo del Resultado

### Escritura
- El resultado del solver se escribe en:
  - `FlowFieldStorage (TileXY, Players)`

### Consumo
- Las entidades:
  - convierten su posición a Cell,
  - leen dirección desde FlowFieldStorage,
  - aplican movimiento.

No hay comunicación directa con el jugador.

---

## Invalidez por Movimiento del Jugador

### Regla PR1
- El flowfield se invalida cuando:
  - el jugador cambia de tile.

No se invalida por cada frame de movimiento.

---

## Checklist de Implementación (PR1)

- [ ] FlowFieldSystem obtiene referencia al jugador.
- [ ] PlayerWorldPosition → PlayerTileXY funciona.
- [ ] HOT/WARM se recalculan al cambiar de tile.
- [ ] GoalsDirty(Players) se marca correctamente.
- [ ] GoalSet se arma dentro de FlowFieldSystem.
- [ ] Pipeline ejecuta rebuild hacia el jugador.
- [ ] Entidades navegan leyendo FlowFieldStorage.

---

## Alcance
Este documento define completamente la integración jugador–flowfield para PR1.
Multiplayer, múltiples goals, radios de influencia y prioridades se abordan en PR2/PR3.
