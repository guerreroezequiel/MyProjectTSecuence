# Flujo de Agrupación SCL con LoD (Grid/ECS)

Patrón operativo para hordas masivas en UE5 (Mass + TurboSequence), basado en **Stimulus → Cluster → Leader (SCL)** y **LoD dual**.

---

## Diagrama de Flujo (simplificado)

```
                           ┌───────────────────────────┐
                           │        ESTÍMULO           │
                           │  (ruido/visión/impacto)   │
                           └─────────────┬─────────────┘
                                         │
                           ┌─────────────▼─────────────┐
                           │  Marcar RESPONDERS (W)    │
                           │  (por celdas de la grilla)│
                           └─────────────┬─────────────┘
                                         │
                           ┌─────────────▼─────────────┐
                           │  CLUSTER por celdas       │
                           │  (solo responders)        │
                           └─────────────┬─────────────┘
                                         │
                           ┌─────────────▼─────────────┐
                           │  Elegir LÍDER (en cluster)│
                           │  + Asignar FOLLOWERS      │
                           └───────┬─────────┬─────────┘
                                   │         │
                     ┌─────────────▼─┐     ┌─▼─────────────┐
                     │   LEADERS     │     │   FOLLOWERS    │
                     └───────┬───────┘     └───────┬────────┘
                             │                    (según cámara/distancia)
               ┌─────────────▼─────────────┐     ┌───────────────┬──────────────┐
               │  LoD = ECS (siempre)      │     │ LoD = ECS      │ LoD = GRID   │
               │  Pathfind + Steering +    │     │ (cerca)        │ (lejos)      │
               │  Obstacle/Local Avoid.    │     │ sigue al líder │ FieldFlow    │
               │  + Portal Navigation      │     │ con ECS        │ + slots      │
               └─────────────┬─────────────┘     └───────────────┴──────────────┘
                             │
                             │    (semilla/objetivo)
                             │
                     ┌───────▼───────────────────────┐
                     │   FlowField/Portales por grupo│
                     │   (generado/actualizado por   │
                     │    líderes; usado por GRID)   │
                     └───────────┬───────────────────┘
                                 │
          ┌──────────────────────▼───────────────────────────┐
          │                 HORDE (GridOnly)                  │
          │   LoD = GRID → FieldFlow + Portales + Slots      │
          │   (sin steering fino)                            │
          └──────────────────────┬───────────────────────────┘
                                 │
          ┌──────────────────────▼───────────────────────────┐
          │                 INACTIVES (Grid)                  │
          │   LoD = GRID → reglas simples por celda          │
          └──────────────────────┬───────────────────────────┘
                                 │
                   (cada Epoch ~5 Hz, commit en SyncPoint)
                                 │
          ┌──────────────────────▼───────────────────────────┐
          │            MERGE / SPLIT de GRUPOS               │
          │  Merge: centros cercanos + rumbo/portal similar  │
          │  Split: elongación alta o bifurcación de portal  │
          └──────────────────────┬───────────────────────────┘
                                 │
                           ┌─────▼─────┐
                           │   LOOP    │
                           │  siguiente│
                           │   Epoch   │
                           └───────────┘
```

---

## Leyenda
- **ECS**: control fino → Pathfinding, Steering, Obstacle/Local Avoidance, Portales.
- **GRID**: control barato → **FieldFlow + slots + portales** (sin steering fino).
- **Leaders**: siempre **ECS**; siembran/actualizan FlowField del grupo.
- **Followers**: **ECS** si cerca/en cámara; **GRID** si lejos/baja prioridad.
- **Horde/Inactives**: siempre **GRID**.
- Cambios de rol/LoD y **Merge/Split** se aplican por **Epoch** (≈5 Hz) con **SyncPoint**.

---

## Checklist de Procesadores (orden sugerido)
1. `StimulusIngestProcessor` — registra/expira estímulos (ring-buffer).
2. `ResponderMarkingProcessor` — marca responders por celdas dentro de W.
3. `ClusterAndLeaderSeedingProcessor` — agrupa responders y elige líder.
4. `FollowerAssignmentProcessor` — asigna followers iniciales y tardíos (cooldown).
5. `HordeDispatcherProcessor` — decide LoD=GRID y asigna FieldFlow/portales.
6. `GroupMergeSplitProcessor` — merge/split por densidad, elongación, portales.
7. `FlowFieldUpdateProcessor` — on‑demand si cambia objetivo/portal.

---

## Invariantes
- Un **Leader** solo nace de **responders** del estímulo.
- **Followers iniciales** ⊆ responders del cluster del mismo estímulo.
- **Horde** no usa steering fino; sólo Grid/slots/portales.
- Ningún agente cambia de grupo más de **1 vez por Epoch**.

---

## Parámetros recomendados (tuning inicial)
- `W (ventana responders)`: **600–900 ms**  
- `Epoch`: **5 Hz**  
- `MAX_LEADERS`: **20**  
- `GROUP_TARGET`: **≈40** (min **6**, max **80**)  
- `MERGE_DIST`: **2× CohesionRadius**  
- `FOLLOW_REASSIGN_COOLDOWN`: **3 s**  
- `Split` por elongación P95 > **1.5× CohesionRadius** o bifurcación de portal ≥ **30%**

---

## QA rápido
- `Stim.Fire type=Noise radius=12 pos=…` — dispara estímulo.
- `Group.DebugDraw` — centros de masa, radios, objetivos.
- `Group.ForceMerge A B` / `Group.ForceSplit A` — valida rutinas.
- Validación: al cerrar **W**, **líder ∈ responders** y **followers ⊆ responders**.

