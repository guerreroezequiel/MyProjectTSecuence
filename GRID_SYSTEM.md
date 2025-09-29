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


## Roles y comportamiento
- **Líder**
  - Procesa estímulos, decide meta/waypoints y ejecuta pathfinding económico (NavMesh/portales). Publica Goal/Seed: tiles destino, goal bias, portal bias, prioridad y cooldown.
- **Seguidor** (pocos por líder; 8–32)
  - Toma el Goal del líder, traza rutas promedio   121→- Líder nace de responders del estímulo y publica metas sesgadas por portales/regiones.
   122→- Seguidores ⊆ responders del cluster del mismo estímulo; respetan cooldown.
   123→- Horda no usa steering fino: se guía por `FlowField`, `Occupancy`, sub‑slots y portales.
   124→- Ningún agente cambia de grupo > 1 vez por epoch; transiciones controladas por histéresis.
   125→
   126→---
   127→
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

         Systems/                                # orquestación (Mass/ECS)
           Processors/
             FlowFieldUpdateProcessor.h/.cpp     # consume DirtyQueue; respeta presupuesto
             HordeDispatcherProcessor.h/.cpp     # solo lectura de flow/occupancy
             ClusterLeaderSeedingProcessor.h/.cpp# downsample rutas; marca tiles/celdas dirty
             DebugOverlayProcessor.h/.cpp        # dibuja capas/metricas de debug
             PortalStatsProcessor.h/.cpp         # contadores por portal/corredor (liviano)
           Subsystems/
             GridWorldSubsystem.h/.cpp           # acceso único; allocs; registro processors
         Components/                             # datos por entidad/grupo (Mass Fragments/Tags)
           Fragments/
             GridCellFragment.h                  # tileXY + subslot local
             GroupIntentFragment.h               # goalId, role, cooldowns
             FlowReadFragment.h                  # {TileId, FlowDir, FlowCost, VersionRead} cacheados
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
         Utils/
           RingBuffer.h                          # buffers de eventos/time windows
           TokenBucket.h                         # rate‑limiting de rebuilds y ticks
           SpatialIndex.h/.cpp                   # índices espaciales/tiles activos
           SmallVector.h                         # evitar allocs (o usar TInlineAllocator)
         Data/
           DT_GridTuning.h                       # tunables de grilla y presupuestos
           DT_FlowSources.h                      # presets de fuentes/sesgos por escenario
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