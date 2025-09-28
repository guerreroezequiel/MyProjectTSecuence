# Grid System Merged – SCL con FlowField Multiobjetivo y Presupuesto 15 ms

## Objetivo
- **Escala**: 10k+ entidades con navegación y colisión eficientes.
- **Enfoque**: SCL (Stimulus → Cluster → Leader) + capas de grid + capa de heat/densidad para roles + flowfield multiobjetivo + particionado local de colisiones + behaviors por instancia en CPU.
- **Presupuesto**: ciclo total ≤ 15 ms, con presupuestos duros por etapa y token-bucket para reconstrucciones.

---

## Principios del sistema
- **Una sola verdad de navegación**: el `FlowField` es la autoridad. Líder/seguidores publican metas/sesgos; la horda lee y se mueve.
- **O(celdas activas)**: todo procesamiento (colisión, flow, LOD) se limita a tiles/celdas activas.
- **LoD dual y sleep/wake**: en cámara: Líder + pocos Seguidores + Horda. Offscreen: colapsar a Horda.
- **Histéresis**: cambios de metas/estados con ventanas/cooldowns para evitar ping‑pong.
- **Sin pathfinding masivo**: solo líderes (y seguidores con downsample para sembrar). La horda no hace pathfind.
- **Behaviors paralelos por instancia (CPU)**: ejecución por lotes, minimizando branching y maximizando cache/SIMD.

---

## Capas del Grid
- **Capa 1 – Celda Base**
  - Resolución 1 m; tiling 64×64 celdas (particionado local). Vecindad 8/16 dirs discretas.
- **Capa 2 – Occupancy/Capacity**
  - Capacidad por tipo de espacio (pasillo, abierto, portal). Estados de obstáculo/portal/activo. Sub‑slots 2×2 a 50 cm para naturalidad y separación en horda.
- **Capa 3 – FlowField (multiobjetivo)**
  - Dirección/costo por celda. Fuentes múltiples con pesos/bias (metas, portales, coste por cuello de botella). Actualización parcial por tiles/celdas "dirty".
- **Capa 4 – Density/Heat (roles y LOD)**
  - Densidad local (p.ej., SAT por tile). Heat como tráfico reciente. Umbrales con histéresis para transiciones Sparse/Dense/Transition. Esta capa define y actualiza los roles:
    - Sparse → más ECS visible (candidatos a seguidores/líderes).
    - Dense → clustering y empuje hacia Horda.
    - Transition → control de cambios con cooldown.
- **Capa 5 – Sleep/Wake & LOD**
  - Frecuencias por LOD con presupuesto por etapa. Despertares por intención, frustum, o cambios fuertes en flujo/heat.
- **Capa 6 – Debug/Observabilidad**
  - Overlays (capacidad, ocupación, flow, LOD, colas) y métricas asíncronas sin tocar el hot path.

---

## Roles y comportamiento
- **Líder**
  - Procesa estímulos, decide meta/waypoints y ejecuta pathfinding económico (NavMesh/portales). Publica Goal/Seed: tiles destino, goal bias, portal bias, prioridad y cooldown.
- **Seguidor** (pocos por líder; 8–32)
  - Toma el Goal del líder, traza rutas promedio (downsample) y siembra el `FlowField` marcando tiles/celdas "dirty" con prioridad. No mueve entidades directamente.
- **Horda**
  - Solo lee `FlowField` + Occupancy/Density. Movimiento masivo con micro‑avoid mínimo por sub‑slots. Sin pathfinding, sin estímulos directos.

---

## Reglas clave de escalado
- **Pocos seguidores**: mantener 8–32 por líder con presupuesto duro (p.ej., ≤ 0.2–0.5 ms total). Son proxies de la marea, no mini‑líderes.
- **Semilla → FlowField**: seguidores no recalculan todo; solo etiquetan tiles/celdas para rebuild parcial con token‑bucket.
- **Una sola verdad**: el `FlowField` guía a todos. Líder/seguidores no empujan posiciones de la horda; publican waypoints/costos/sesgos.
- **Histéresis en metas**: líder con cooldown; seguidores respetan ventana.
- **Offscreen LOD**: fuera de cámara, colapsar Seguidores → Horda proxy; en cámara, activar Líder + pocos Seguidores.

---

## Pipeline operativo (copiar/pegar)
- **Leader.Step()**
  - Lee estímulos → elige GoalTile/Waypoints.
  - Corre pathfind barato (portales/navcells).
  - Publica: GoalBias, PortalBias, Priority, Cooldown.
- **Follower.SeedFlow()** (presupuesto fijo)
  - Toma Goal/Waypoints del líder.
  - Traza rutas promedio (downsample); marca TilesDirty/CellsDirty con prioridad.
  - No mueve entidades: solo siembra.
- **FlowField.Rebuild()** (token bucket)
  - Recalcula N tiles/frame y M celdas/frame según marcas.
  - Aplica histéresis y portal bias del líder.
- **Horde.Move()**
  - Entidades leen FlowDir + Occupancy/Density.
  - Micro‑avoid mínimo por sub‑slot; sin pathfind.

---

## Navegación y flow multiobjetivo
- **Fuentes combinadas**: metas del líder, sesgos de portales y penalizaciones por cuello de botella integrados en un solo vector por celda.
- **Rebuild parcial**: disparado por cambios en metas, portales o densidad que superen umbrales (con cooldown). Límite estricto por frame mediante token‑bucket.
- **Lectura O(1)**: la horda consulta dirección/costo ya resueltos.

---

## Particionado local de colisiones (estática y dinámica)
- **Tiles activos**: listas separadas para estática y dinámica; solo se procesan tiles activos/cercanos.
- **Colisión estática**: precomputada por tile; consultas O(1).
- **Colisión dinámica**: limitada a entidades en tiles activos; evita O(total entidades).
- **Prevención de deadlocks**: capacidad por tipo de espacio, sesgos de portal y tie‑breakers con histéresis.

---

## LOD y visibilidad
- **En cámara**: Líder en LOD alto; unos pocos Seguidores visibles; Horda con anim simplificada pero lectura de flow a 60/30 Hz según proximidad.
- **Fuera de cámara**: Seguidores colapsan a Horda proxy; actualización más laxa del flow; mantenimiento por densidad/heat.
- **Wake triggers**: entrada a frustum expandido, cambio fuerte de flow en tile adyacente, picos de heat.

---

## Behaviors CPU por instancia
- **Paralelismo**: procesamiento por lotes grandes, evitando branching; hot fragments contiguos.
- **Rate limiting**: límites explícitos de frecuencia por subsistema (p.ej., Flow a 5–10 Hz efectivo por región) y stride derivado por presupuesto, no por número mágico.
- **Memoria**: estructuras compactas y versiones por tile para validar lecturas.

---

## Presupuesto de 15 ms (objetivo por frame)
- **Líder (estímulos + pathfind + publicación)**: 1.0–2.0 ms (máx líderes activos en cámara, cooldown/histéresis).
- **Seguidores (siembra, downsample, marcas)**: 0.2–0.5 ms totales (8–32 por líder, colapsar offscreen).
- **FlowField (rebuild parcial token‑bucket)**: 3.0–5.0 ms (N tiles + M celdas por frame; prioridad por proximidad/impacto visual).
- **Particionado/colisión (estática/dinámica en tiles activos)**: 1.0–2.0 ms.
- **Horda.Move (lectura flow + micro‑avoid en slots)**: 3.0–4.0 ms.
- **Scheduler/LOD/Sleep‑Wake**: 0.3–0.7 ms (batch por región, umbrales por heat/densidad).
- **Observabilidad/telemetría**: ≤ 0.3 ms (asíncrono, frame‑budgeted).
- **Reserva/variabilidad**: 1.0–2.0 ms.

Notas:
- Los límites son orientativos; priorizar calidad visual en cámara y estabilidad de frame. Sub-sistemas deben degradar de forma "graciosa" al agotar presupuesto (menos tiles de flow, menor frecuencia, colapso de followers, etc.).

---

## Reglas de consistencia e invariantes
- Líder nace de responders del estímulo y publica metas sesgadas por portales/regiones.
- Seguidores ⊆ responders del cluster del mismo estímulo; respetan cooldown.
- Horda no usa steering fino: se guía por `FlowField`, `Occupancy`, sub‑slots y portales.
- Ningún agente cambia de grupo > 1 vez por epoch; transiciones controladas por histéresis.

---

## Métricas esenciales
- ActiveTiles (% del total), CellsPerLOD, FlowRebuildTiles/Celdas por frame, Intent queues (largo, bloqueos), Portal usage, Horde merges/splits, Sleep/Wake transitions, AverageHeat, tiempo por subsistema y headroom.

---

## Operatividad y roadmap
- Implementar primero hot path (intenciones/arbitraje/commit) y scheduler reactivo.
- Integrar flow multiobjetivo con rebuild parcial y token‑bucket.
- Activar roles por densidad/heat y LOD reactivo (on/offscreen).
- Optimizar behaviors por instancia (batch grande, cache‑friendly) y colisión local por tiles.
- Añadir observabilidad asíncrona y pruebas sintéticas sin tocar hot path.
