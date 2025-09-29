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

## Temporización y Epoch global
- **Epoch global**: 200 ms (5 Hz), sincroniza cambios de rol/cluster y ventanas de histéresis.
- **Ámbito**: global y basado en reloj (wall‑clock), no por frame. Se define en `GridEpoch.h` y lo consume el scheduler.
- **Restricción**: ningún agente cambia de grupo (Líder/Seguidor/Horda) más de 1 vez por epoch.
- **Cooldowns**: los triggers de wake tienen cooldown mínimo de 1× epoch por entidad/región salvo que un trigger de mayor prioridad fuerce el wake.

## Capas del Grid
- **Capa 1 – Celda Base**
  - Resolución 1 m; tiling 64×64 celdas (particionado local). Vecindad 8/16 dirs discretas.
- **Capa 2 – Occupancy/Capacity**
  - Capacidad por tipo de espacio (pasillo, abierto, portal). Estados de obstáculo/portal/activo.
  - Sub‑slots 2×2 (50 cm) por celda para naturalidad y micro‑separación.
  - Interacción: la `CapacityGrid` define capacidad dura por celda; los sub‑slots distribuyen ocupación dentro de esa capacidad.
    - HardOnly: no se excede la capacidad; asignación de sub‑slots evita colisiones intra‑celda.
    - SoftAdaptive: se permite un exceso soft (SoftCapacityDelta) que incrementa costo efectivo de la celda; el solver y la Horda lo perciben como congestión.
- **Capa 3 – FlowField (multiobjetivo)**
  - Dirección/costo por celda. Fuentes múltiples con pesos/bias (metas, portales, coste por cuello de botella). Actualización parcial por tiles/celdas "dirty".
- **Capa 4 – Density/Heat (roles y LOD)**
  - Densidad local (p.ej., SAT por tile). Heat como tráfico reciente. Umbrales con histéresis para transiciones Sparse/Dense/Transition. Esta capa define y actualiza los roles:
    - Sparse → más ECS visible (candidatos a seguidores/líderes).
    - Dense → clustering y empuje hacia Horda.
    - Transition → control de cambios con cooldown.
- **Capa 5 – Sleep/Wake & LOD**
  - Frecuencias por LOD con presupuesto por etapa. Despertares por intención, frustum, o cambios fuertes en flujo/heat.
- **Capa 6 – Damage (plan futuro)**
  - Máscara de daño por celda/sub‑slot y soporte de intersección de mesh.
  - Política híbrida según `Heat/Density` y tamaño de grupo: grid‑based vs mesh‑trace.
  - Segmentación vertical normalizada para altura 1.8 m (head/torso/legs) para zombies.
- **Capa 7 – Debug/Observabilidad**
  - Overlays (capacidad, ocupación, flow, LOD, colas) y métricas asíncronas sin tocar el hot path.


## Roles y comportamiento
- **Líder**
  - Procesa estímulos, decide meta/waypoints y ejecuta pathfinding económico (NavMesh/portales).
  - Publica Goal/Seed: tiles destino, goal bias, portal bias, prioridad y cooldown.
- **Seguidor**
  - Pocos por líder; número depende del LOD.
  - Toma el Goal/Waypoints del líder y traza rutas promedio (downsample) para sembrar el `FlowField`.
  - No realiza pathfinding pesado ni mueve entidades: solo marca TilesDirty/CellsDirty con prioridad; siembra flujo.
- **Horda**
  - No usa steering fino ni pathfinding. Se guía por `FlowField` + `Occupancy/Capacity` + sub‑slots y sesgos de portal.

### Política de roles (congruente)
- **Autoridades de decisión**
  - `Cluster C(S)`: conjunto de responders del estímulo S.
  - `Visibilidad/LOD`: {LOD0, LOD1, LOD2, Offscreen} por frustum expandido.
  - `HeatTile`: banda con histéresis {Low, Mid, High} via `H_low/H_high`.
- **Cupo por LOD (Seguidores)**: LOD0=16, LOD1=8, LOD2=4, Offscreen=0.
- **Política por combinación (por cluster y frame)**
  - Onscreen & Heat ∈ {Low, Mid}: 1 Líder en C, hasta `CupoLOD` Seguidores, resto Horda.
  - Onscreen & Heat = High: 1 Líder en C, 0 Seguidores, resto Horda (colapso por densidad).
  - Offscreen: 0 Líder, 0 Seguidores, todo Horda (proxy).
- **Transiciones (máx 1 por entidad y por epoch=200 ms)**
  - Horda → Seguidor: entra a Onscreen AND hay cupo AND `CooldownFollower == 0` AND `Heat != High`.
  - Seguidor → Horda: pasa a Offscreen OR `Heat = High` sostenido ≥ 1× epoch OR fin de seeding.
  - Seguidor → Líder: líder inválido y `CooldownLeader == 0` y supera umbral de score definido en “Selección…”.
  - Líder → Horda: Offscreen OR `GroupSize < MinLeaderSize` sostenido ≥ 1× epoch.
- **Coherencia**
  - Si no hay candidato válido, se mantiene el rol actual ≥ 1× epoch.
  - Horda nunca hace pathfinding; sólo lee `FlowField/Occupancy`.
  - La selección concreta (scores/muestreo) se define en “Selección de Líderes y Seguidores (algoritmo)”.

### Selección de Líderes y Seguidores (algoritmo)
- **Entrada**: `Cluster` de responders a un estímulo S, `Goal`/sesgos del estímulo, `Heat/Density` locales, `Cooldown` por entidad, y LOD actual.
- **Líder**
  - Candidatos: entidades del cluster con `CooldownLeader == 0` y visibilidad/posición válida.
  - Score por entidad i: `score_i = w_d*DistToGoal^-1 + w_c*Centrality + w_h*(1 - NormHeat) + w_v*VisibilityBias - w_cd*CooldownPenalty`.
  - Selección: `Leader = argmax(score_i)` sujeto a distancia mínima al objetivo o representatividad del cluster.
  - Cooldown: set `CooldownLeader = 1× epoch` para evitar thrash.
- **Seguidores**
  - Cupo por LOD: ver "Seguidores por Líder (por LOD)".
  - Candidatos: cluster \ Leader con `CooldownFollower == 0`.
  - Muestreo espacial tipo blue‑noise/poisson disc dentro del cluster priorizando
    - menor `Heat`, mejor cobertura direccional alrededor del líder, y proximidad a vías/portales.
  - Selección incremental hasta cubrir el cupo; set `CooldownFollower = 1× epoch`.
- **Notas**
  - Si el cluster es denso y el Heat alto, se reduce el cupo efectivo (min{cupo, factor_heat}).
  - Si no hay candidatos válidos, se mantiene el líder/seguidores actuales (histéresis) hasta el próximo epoch.

---

   ### Notas
   - **Resolución base**: 1 m/celda; tiles 64×64 celdas.
   - **Nomenclatura**: prefijo `Grid` para clases núcleo; `FlowField*` para flow; `*Processor` para Mass/ECS; `*Subsystem` para orquestación global.
   - **Integraciones**: bridges a `Navmesh` y a colisiones/alturas de Building Hider (solo lectura) para poblar `Occupancy/Portals`.
   - **Debug**: overlays y métricas deben ser opt‑in y baratos (no tocar hot path).
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
  - No mueve entidades ni resuelve colisiones: solo siembra.
- **FlowField.Rebuild()** (token bucket)
  - Recalcula N tiles/frame y M celdas/frame según marcas.
  - Mezcla costos y aplica inflación por congestión: Cost' = Cost + α*Heat + β*QueuePressure.
  - Aplica histéresis y portal bias del líder.
- **Horde.Move()**
  - Entidades leen FlowDir + Occupancy/Density.
  - Micro‑avoid mínimo por sub‑slot; sin pathfind.

---

## Navegación y flow multiobjetivo
- **Fuentes combinadas**: metas del líder, sesgos de portales y penalizaciones por cuello de botella integrados en un solo vector por celda.
- **Rebuild parcial**: disparado por cambios en metas, portales o densidad que superen umbrales (con cooldown). Límite estricto por frame mediante token‑bucket.
- **Lectura O(1)**: la horda consulta dirección/costo ya resueltos.
 - **Aplicación de Heat/QueuePressure**: la inflación de costos se aplica dentro de `FlowField.Rebuild()`/solver durante la escritura de costos por celda; nunca en la lectura (`Horde.Move()` solo lee valores cacheados).

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
- **Seguidores por Líder (por LOD)**:
  - LOD0 (muy cerca/en foco): 16 seguidores.
  - LOD1 (cerca): 8 seguidores.
  - LOD2 (medio/lejos en cámara): 4 seguidores.
  - Offscreen: 0 seguidores (colapsa a Horda proxy).
- **Wake triggers (prioridad y cooldown)**:
  1. Intención directa del jugador/IA sobre el grupo (máxima prioridad, ignora cooldown local).
  2. Entrada al frustum expandido (FOV + margen): requiere que el último sleep tenga ≥ 1× epoch.
  3. Cambio fuerte de flow en tile adyacente (Δcost o Δdir > umbral): respeta cooldown ≥ 1× epoch.
  4. Pico de Heat/QueuePressure sostenido (ventana móvil ≥ 1× epoch): respeta cooldown y exige histéresis High→Low.
  - Si múltiples triggers compiten, gana el de mayor prioridad; el cooldown se reinicia tras wake y se aplica histéresis para evitar ping‑pong.
  - Solo el trigger 1 puede anular cooldowns previos. Los triggers 2–4 nunca lo anulan: requieren haber cumplido ≥ 1× epoch desde el último cambio.

---

## Damage System (diseño futuro)
- **Granularidad**
  - Grid‑based: aplicar daño por celda o sub‑slot cuando `Heat/Density` o tamaño de grupo superan umbrales.
  - Mesh‑based: para unidades individuales o grupos pequeños, usar intersección de trace vs mesh/esqueleto.
- **Segmentación vertical (altura base 1.8 m, zombies)**
  - Bandas normalizadas basadas en proporciones aportadas: Head ~ 0.20, Torso ~ 0.33, Legs ~ 0.47. Se normaliza a suma 1.0 para cualquier altura.
  - Ejemplo 1.8 m: Head ≈ 36 cm; Torso ≈ 59 cm; Legs ≈ 85 cm. Límites acumulados: 0.20 / 0.53 / 1.00.
- **Política de selección (heurística inicial)**
  - Si `GroupSize ≥ G_large` o `Heat ≥ H_high` → Grid‑based por celda/sub‑slot, ponderando cobertura y densidad.
  - Si `GroupSize ≤ G_small` y `Heat ≤ H_low` → Mesh‑based con trace y mapeo a banda vertical.
  - En transición, mezclar: `Damage = lerp(D_grid, D_mesh, k)` con `k` derivado de Heat/Density.
- **Observabilidad**
  - Overlay futuro: pintar celdas/sub‑slots con probabilidad/daño esperado y mostrar límites de bandas.

## Orden completo y coherencia (qué se llama y cuándo)
- **1) Scheduler/Epoch**
  - Llama `GridWorldSubsystem` para tick global y sincroniza `Epoch (200 ms)` y versiones. Usa `GridEpoch.h`.
  - Garantiza: máx 1 cambio de rol por epoch; cooldowns de wake basados en epoch.

- **2) Stimulus → Cluster → Leader**
  - `ClusterLeaderSeedingProcessor` evalúa estímulos y clusters.
  - Selección de líder según algoritmo formalizado (scores, cooldown). Publica `Goal/PortalBias/Priority`.

- **3) Followers (seeding por LOD)**
  - Selecciona seguidores por LOD (16/8/4/0). Solo siembran: marcan `TilesDirty/CellsDirty` con prioridad.
  - No mueven entidades ni hacen pathfinding pesado.

- **4) FlowField.Rebuild() (token‑bucket)**
  - `FlowFieldUpdateProcessor` consume `DirtyQueue` y recalcula N tiles + M celdas por frame.
  - Mezcla costos y aplica inflación: `Cost' = Cost + α·Heat + β·QueuePressure`.
  - Aplica sesgos de portales e histéresis. Escritura es versionada para lecturas O(1).

- **5) Particionado/Occupancy**
  - Actualiza ocupación dinámica por tiles activos; mantiene `sub‑slots` y listas de ocupantes por sub‑slot.

- **6) LOD / Sleep‑Wake**
  - Evalúa triggers en orden de prioridad: intención directa > frustum expandido > Δflow fuerte > pico de Heat.
  - Solo la intención directa puede saltar cooldown; el resto exige ≥ 1× epoch.

- **7) Horde.Move()**
  - `HordeDispatcherProcessor` lee `FlowReadFragment` (dir/costo) + `Occupancy/Density`. Micro‑avoid en sub‑slots.
  - No hace pathfinding ni recalcula flow.

- **8) Damage (budgeted, snapshot por epoch)**
  - Punto de snapshot: fija `CellOccupants[sub‑slot]` para elegibilidad del epoch.
  - Modo Grid‑based (Heat/Group grande):
    - Por celda/sub‑slot calcula hits esperados y selecciona O(Hits) con `RotIndex` round‑robin; aplica caps por celda y token‑bucket global en `DamageApplyProcessor`.
  - Modo Mesh‑based (grupos chicos/Heat bajo):
    - Selecciona cupo pequeño round‑robin y realiza intersección trace→mesh, mapeando a bandas verticales (head/torso/legs).
  - Offscreen: colapsa a proxy por celda; Onscreen: distribuye al promover.

- **9) Observabilidad/Telemetría**
  - `DebugOverlayProcessor` renderiza overlays (Flow/Heat/Portals/Damage) y métricas asíncronas con budget.

- **10) Cierre de epoch (cada 200 ms)**
  - Materializa ventanas (si aplica), rota `RotIndex` por sub‑slot, actualiza cooldowns/histéresis y limpia colas diferidas.
  - Garantiza coherencia temporal: ningún agente cambia de rol > 1× por epoch; lecturas/escrituras versionadas.

### Notas de coherencia
- Lecturas de la Horda son siempre de `FlowFieldStorage` versionado; sin recomputar.
- Damage Grid‑based no requiere integración por entidad: selección round‑robin evita escaneo y mantiene expectativa estadística.
- Caps por celda y token‑bucket global previenen picos; colas diferidas mantienen orden por prioridad (Heat/visibilidad).

---

## Behaviors CPU por instancia
- **Paralelismo**: procesamiento por lotes grandes, evitando branching; hot fragments contiguos.
- **Rate limiting**: límites explícitos de frecuencia por subsistema (p.ej., Flow a 5–10 Hz efectivo por región) y stride derivado por presupuesto, no por número mágico.
- **Memoria**: estructuras compactas y versiones por tile para validar lecturas.

---

## Presupuesto de 15 ms (objetivo por frame)
- **Líder (estímulos + pathfind + publicación)**: 1.0–2.0 ms (máx líderes activos en cámara, cooldown/histéresis).
- **Seguidores (siembra, downsample, marcas)**: 0.2–0.5 ms totales (8–32 por líder, colapsar offscreen).
- **FlowField (rebuild parcial token‑bucket)**: 3.0–4.0 ms (N tiles + M celdas por frame; prioridad por proximidad/impacto visual).
- **Particionado/colisión (estática/dinámica en tiles activos)**: 1.0–2.0 ms.
- **Horda.Move (lectura flow + micro‑avoid en slots)**: 2.5–3.5 ms.
- **Scheduler/LOD/Sleep‑Wake**: 0.3–0.7 ms (batch por región, umbrales por heat/densidad).
- **Observabilidad/telemetría**: ≤ 0.3 ms (asíncrono, frame‑budgeted).
- **Reserva/variabilidad**: 1.0–2.0 ms.

---

## Estructura de carpetas (propuesta)
   
   Esta es la estructura mínima sugerida para implementar el sistema de grilla descrito en este documento. Es solo organización y naming; no crea archivos automáticamente.
   
   ### Código C++
   `Source/MyProjectTSecuence/GridSystem/`
   
   ```text
   Source/
     MyProjectTSecuence/
       GridSystem/
         Core/                                   # Tipos base, config global, constantes
           GridTypes.h                           # typedefs, enums, ids, helpers comunes
           GridConfig.h/.cpp                     # tunables/constantes de grilla/tiles
           GridWorld.h/.cpp                      # bounds, resolución, tiles/celdas
           GridPortal.h/.cpp                     # definición de portales y links
           GridEpoch.h                           # epoch global para sync (uint32)
           TileVersioning.h                      # versión por tile/flow para validar lecturas
         FlowField/                              # cálculo/almacenamiento flow multi‑fuente
           FlowField.h/.cpp                      # interfaz de lectura/escritura de flow
           FlowFieldRebuilder.h/.cpp             # orquesta rebuild parcial (token bucket)
           FlowFieldSources.h/.cpp               # metas, sesgos, costos por fuente
           FlowFieldStorage.h/.cpp               # SOA; versiones/epochs por celda
           FlowFieldSolver.h/.cpp                # D* Lite / BFS multi‑fuente / mezcla costos; ESolverMode { BFS, MultiSource, DStarLite }
           FlowFieldDirtyQueue.h/.cpp            # colas High/Normal; priority=f(heat, dist cámara, impacto)
         Occupancy/                              # ocupación/capacidad estática y dinámica
           OccupancyGrid.h/.cpp                  # mapa binario/estados por celda
           CapacityGrid.h/.cpp                   # capacidad dura + SoftCapacity efectiva; SoftCapacityDelta; CapacityMode{HardOnly, SoftAdaptive}
         Density/                                # densidad/heat y métricas por tile
           DensityHeatGrid.h/.cpp                # AccumulateTraffic, Decay, GetHeat, IsStalled, GetQueuePressure
         Damage/                                 # (futuro) daño por celda/slot/mesh y políticas
           DamageGrid.h/.cpp                     # máscara por celda/subslot; vertical bands normalizadas
           DamagePolicy.h/.cpp                   # reglas por LOD/Heat/Density y tamaño de grupo

         Systems/                                # orquestación (Mass/ECS)
           Processors/
             FlowFieldUpdateProcessor.h/.cpp     # consume DirtyQueue; respeta presupuesto
             HordeDispatcherProcessor.h/.cpp     # solo lectura de flow/occupancy
             ClusterLeaderSeedingProcessor.h/.cpp# downsample rutas; marca tiles/celdas dirty
             DebugOverlayProcessor.h/.cpp        # dibuja capas/metricas de debug
             PortalStatsProcessor.h/.cpp         # contadores por portal/corredor (liviano)
             DamageApplyProcessor.h/.cpp         # (futuro) aplica daño según política activa
           Subsystems/
             GridWorldSubsystem.h/.cpp           # acceso único; allocs; registro processors
         Components/                             # datos por entidad/grupo (Mass Fragments/Tags)
           Fragments/
             GridCellFragment.h                  # tileXY + subslot local
             GroupIntentFragment.h               # goalId, role, cooldowns
             FlowReadFragment.h                  # {TileId, FlowDir, FlowCost, VersionRead} cacheados
             DamageAccumulatorFragment.h         # (futuro) acumulación/mediate hit por entidad
           Tags/
             HordeTag.h                          # marca entidades de horda
             LeaderTag.h                         # marca líderes
             FollowerTag.h                       # marca seguidores
         Interfaces/                             # Bridges a NavMesh/BuildingHider/Portales
           NavmeshBridge.h/.cpp                  # lecturas NavMesh (portales/navcells/goals)
           BuildingHiderBridge.h/.cpp            # alturas/colisiones por piso (solo lectura)
           PortalBridge.h/.cpp                   # (experimental) portales desde level design; devolver passthrough si no hay authoring
         Debug/
           GridDebugDraw.h/.cpp                  # helpers de dibujo de celdas/tiles
           GridStats.h/.cpp                      # métricas agregadas y sampling
           HeatMapOverlay.h/.cpp                 # overlay de heat/density
           PortalOverlay.h/.cpp                  # overlay de portales/costos
           DamageOverlay.h/.cpp                  # (futuro) overlay de daño por celda/subslot
         Utils/
           RingBuffer.h                          # buffers de eventos/time windows
           TokenBucket.h                         # rate‑limiting de rebuilds y ticks
           SpatialIndex.h/.cpp                   # índices espaciales/tiles activos
           SmallVector.h                         # evitar allocs (o usar TInlineAllocator)
         Data/
           DT_GridTuning.h                       # tunables de grilla y presupuestos
           DT_FlowSources.h                      # presets de fuentes/sesgos por escenario
           DT_DamageTuning.h                     # (futuro) pesos/políticas de daño y bandas verticales
   ```
   
   ### Contenido y Blueprints
   `Content/GridSystem/`
   
   ```text
   Content/
     GridSystem/
       Blueprints/
         BP_GridWorld          # actor helper para bounds/visual/debug
         BP_GridOverlay        # toggle de overlays y settings en runtime
       Materials/
         M_Grid_Debug
         MI_Grid_Debug_1m
       Textures/
         T_Grid_1m
       UI/
         WB_GridDebug
       Data/
          DT_GridTuning        # tunables (resolución, límites por etapa)
          DT_FlowSources       # presets de fuentes/sesgos por escenario
       Debug/
         Profiles/             # colecciones de overlays y métricas
   ```
   
   ### Editor/Tests/Opcional Plugin
   
   ```text
   Content/Editor/GridSystem/   # íconos, utilidades de editor
   Content/Tests/GridSystem/    # mapas de prueba sintéticos
   
   Plugins/GridSystem/          # (opcional) versión plugin si se independiza
     Source/GridSystem/...      # espeja la estructura de C++ anterior
     Content/GridSystem/...     # espeja la estructura de contenido
   ```
   
---

#### Checklist de arranque (paso a paso)
- Core + Subsystem
  - GridWorldSubsystem crea storages (Occupancy/Flow/Density) y expone getters.
  - BP_GridWorld dibuja bounds y celdas de 1 m (material de debug).
- Occupancy estática
  - Cargar paredes/portales en OccupancyGrid y CapacityGrid.
  - Queries O(1) con SOA (sin mapas dinámicos).
- FlowFieldStorage + DirtyQueue
  - FlowFieldStorage: solo arrays y versiones; sin solver aún.
  - FlowFieldDirtyQueue: dos colas (High/Normal).
- Solver + Rebuilder
  - FlowFieldSolver (BFS multi‑fuente) con mezcla simple de costos.
  - FlowFieldRebuilder: token‑bucket N tiles + M celdas/frame.
  - Overlay para ver qué se reconstruye hoy.
- Followers (siembra)
  - ClusterLeaderSeedingProcessor marca tiles/celdas dirty (downsample).
  - Priorizar High si IsStalled o Heat > H_high.
- Horda dummy
  - HordeDispatcherProcessor lee FlowReadFragment (dir/costo) y mueve cubos.
  - Saltar actualización si VersionRead no cambió.
- Heat y anti‑atascos
  - DensityHeatGrid (accumulate + decay).
  - Coste inflado: Cost' = Cost + α*Heat + β*QueuePressure.
- UI/Blueprints
  - BP_GridOverlay + WB_GridDebug sliders: HeatDecay, H_high/H_low, FlowBudget, SoftCapacityDelta, PortalBiasScale.
- Consola mínima
  - grid.flow.rebuild_now, grid.heat.inject x y v, grid.debug.toggle, grid.portal.bias name s.
- Pruebas sintéticas
  - Embudo con un portal vs dos → observar desvío por Heat.
  - Corredor capacity 1 con SoftAdaptive → ver “semáforo” natural.
  - Stress de goals → el rebuild parcial mantiene FPS.
- Damage (futuro)
  - DamageGrid stub con bands normalizadas y toggles de política.
  - Overlay de Damage con heatmap de celda/sub‑slot y bandas verticales.
  - Pruebas: grupos grandes (grid‑based) vs individual (mesh‑trace) en escenarios sintéticos.