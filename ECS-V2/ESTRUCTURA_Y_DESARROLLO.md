# ECS-V2: Sistema Grid + ECS + TurboSequence para 3000+ entidades

## Arquitectura Core: Grid-Based Movement System

### 1. Núcleo del Grid (Dispatcher)
- **Grilla discreta por piso**: FloorId con CellSize fijo (0.6m)
- **Capacidad por celda**: CellCapacity y presupuesto por portal (PortalCap)
- **Aristas**: EdgeBits N/E/S/O para bloquear pasos y diagonales
- **Dispatcher central**: tick lógico (12-15 Hz)
  - Encola intenciones por celda/dirección (N, NE, ...)
  - Despacha ganadores por destino según capacidad y prioridad
  - Commit: actualiza celda de ganadores (único lugar de movimiento lógico)
  - Wake/Sleep: perdedores duermen hasta cambio de Epoch o heartbeat

### 2. Fairness y Performance
- **Token-Bucket + Ticket Round-Robin** por celda (evita starvation)
- **Tiling**: procesar por tiles (16×16 celdas) para paralelismo
- **Event-Driven**: en atascos, mayoría duerme (no consume CPU)
- **Resolución por destino**: costo escala con #celdas activas, no #entidades

## Datos ECS (Fragments/Tags Mínimos)

### Grid Fragments
- **GridPositionFragment**: Cell{x,y}, FloorId
- **MovementIntentFragment**: DirWish (0=Stay, 1..8=N..NW), SleepTicks
- **OrientationFragment**: YawQ (cuantizable 16 dirs)
- **GroupFragment**: RoleTag (Leader|Follower|Neutral), GroupId, SlotIndex

### Globales SOA (no por entidad)
- Walkable[], CellCapacity[], PortalCap[], EdgeBits[]
- VertLinks[], Occupants[], Queues[dir], Epoch[], TicketRR[]

### TurboSequence Fragments
- **TSTransformFragment**: datos listos para snapshot (interpolación 60 FPS)
- **TSAnimationFragment**: clip/time/flags mínimos

**Regla**: Fragments estables, cambios mínimos en estructuras

## Pipeline de Procesadores ECS (orden crítico)

### 1. Decisión de Intención (12-15 Hz)
- **LeaderWishProcessor** (≤20 grupos): fija DirWish del líder con suavizado
- **FollowerWishProcessor**: DirWish hacia offset relativo al líder (slots blandos)
- **NeutralWishProcessor**: wander/flow local → DirWish

### 2. Grid Dispatch (core del sistema)
- **GridEnqueueProcessor**: empuja índices a colas por celda/dirección (respeta EdgeBits)
- **GridDispatchProcessor**: por celda destino concede hasta min(FreeSlots, PortalBudget)
  - Prioridad: Leader > Follower > Neutral, luego ticket
- **GridCommitProcessor**: aplica grants → cambia Cell, actualiza Occupants y Epoch
- **GridWakeProcessor**: despierta bloqueados por Epoch distinto o cooldown vencido

### 3. Interpolación Visual (60 FPS)
- **YawIntegrateProcessor**: suaviza rotación + cuantización opcional (16 dirs)
- **TSSnapshotProcessor**: empaqueta datos → TurboSequence

## Estructura de Carpetas MVP

ECS-V2/
├── Grid/                          # Sistema de grilla (PRIORIDAD 1)
│   ├── Core/
│   │   ├── GridDispatcher         # Dispatcher central del grid
│   │   ├── CellManager           # Manejo de celdas y capacidades
│   │   └── PortalManager         # Manejo de portales y EdgeBits
│   ├── Data/
│   │   ├── GridGlobals           # SOA: Walkable[], CellCapacity[], etc.
│   │   ├── OccupancyTables       # Occupants[], Queues[dir], Epoch[]
│   │   └── FairnessSystem        # Token-Bucket + Ticket Round-Robin
│   └── Utils/
│       ├── TileProcessor         # Procesamiento por tiles (16×16)
│       └── GridMath              # Conversiones, direcciones, etc.
├── ECS/
│   ├── Fragments/
│   │   ├── Grid/
│   │   │   ├── GridPositionFragment
│   │   │   ├── MovementIntentFragment
│   │   │   ├── OrientationFragment
│   │   │   └── GroupFragment
│   │   └── TurboSequence/
│   │       ├── TSTransformFragment
│   │       └── TSAnimationFragment
│   ├── Processors/
│   │   ├── Intent/               # Fase 1: Decisión
│   │   │   ├── LeaderWishProcessor
│   │   │   ├── FollowerWishProcessor
│   │   │   └── NeutralWishProcessor
│   │   ├── Grid/                 # Fase 2: Grid Dispatch
│   │   │   ├── GridEnqueueProcessor
│   │   │   ├── GridDispatchProcessor
│   │   │   ├── GridCommitProcessor
│   │   │   └── GridWakeProcessor
│   │   └── Visual/               # Fase 3: Interpolación
│   │       ├── YawIntegrateProcessor
│   │       └── TSSnapshotProcessor
│   └── Tags/
│       ├── RoleTag               # Leader/Follower/Neutral
│       ├── SleepTag              # Para entidades dormidas
│       └── ActiveTag             # Para procesamiento
├── Groups/                        # Sistema de grupos (PRIORIDAD 3)
│   ├── GroupManager              # Merge/Split determinista
│   ├── LeaderElection            # Líder virtual (ancla lógica)
│   └── SlotAssignment            # Slots blandos para followers
├── TurboSequence/
│   ├── Integration/
│   │   ├── SnapshotBuffer        # Double-buffer para datos TS
│   │   └── EntityMapping         # EntityId ↔ TSInstanceId
│   └── Rendering/
│       ├── CullingLOD
│       └── BatchRenderer
└── Config/
    ├── GridDefaults              # CellSize=0.6m, TickHz=12, etc.
    └── PerformanceLimits         # Caps y presupuestos

  

## Plan de Desarrollo MVP (Grid → Entidades → Grupos)

### **FASE 1: Grid System Foundation** 🔥 PRIORIDAD MÁXIMA
**Objetivo**: Sistema de grilla funcional sin entidades
- **GridDispatcher**: tick lógico (12-15 Hz), encolado/despacho básico
- **CellManager**: CellSize=0.6m, CellCapacity, EdgeBits básicos
- **GridGlobals**: SOA arrays (Walkable[], Occupants[], Queues[])
- **GridMath**: conversiones coord→cell, direcciones (0-8), validaciones
- **Testing**: Grid vacío + simulación de intenciones mock

### **FASE 2: ECS Integration** 
**Objetivo**: Entidades básicas moviéndose en el grid
- **Grid Fragments**: GridPositionFragment, MovementIntentFragment
- **Core Processors**: GridEnqueueProcessor, GridDispatchProcessor, GridCommitProcessor
- **FairnessSystem**: Token-Bucket básico + Round-Robin por celda
- **GridWakeProcessor**: sistema Sleep/Wake por Epoch
- **Testing**: 100 entidades neutral moviéndose random

### **FASE 3: TurboSequence Visual**
**Objetivo**: Renderizado fluido 60 FPS con interpolación
- **TSTransformFragment + TSAnimationFragment**
- **YawIntegrateProcessor**: suavizado de rotación + cuantización 16 dirs
- **TSSnapshotProcessor**: empaquetado para TurboSequence
- **SnapshotBuffer**: double-buffer para sincronización
- **Testing**: 500 entidades visibles con animaciones

### **FASE 4: Group System**
**Objetivo**: Líderes y seguidores cohesivos
- **GroupFragment**: RoleTag, GroupId, SlotIndex
- **LeaderWishProcessor**: decisiones de líder con suavizado
- **FollowerWishProcessor**: slots blandos hacia líder
- **GroupManager**: merge/split determinista (≤20 grupos activos)
- **Testing**: 5 grupos de 100 entidades c/u (500 total)

### **FASE 5: Performance & Scale**
**Objetivo**: 3000+ entidades a 60 FPS
- **TileProcessor**: paralelismo por tiles (16×16 celdas)
- **PortalManager**: embudos con PortalCap (1-3)
- **Performance profiling**: costo por #celdas activas vs #entidades
- **Stress testing**: 3000 entidades, múltiples grupos, embudos
- **MVP completion**: validación de criterios de éxito

## Patrones de Rendimiento Clave

### Event-Driven Performance
- **Epoch & Sleep**: en atascos, mayoría duerme (no consume CPU)
- **Resolución por destino**: costo escala con #celdas activas, no #entidades totales
- **Double-Buffer + Command Buffer**: grants/commits sin data races

### Determinismo y Persistencia
- **Dispatcher determinista**: mismos inputs → mismos outputs
- **Persistencia por región**: snapshot SOA + catch-up discreto
- **Network lockstep**: replicar inputs (DirWish/Target) y seeds

### Extensiones Futuras (plugs preparados)
- **AOE celular**: reducir por celda/tile, aplicar a Occupants
- **Sector Graph**: macro-ruta de líderes por sectores/portales
- **LoS/Combate discreto**: usar EdgeBits para evitar interacción a través de muros
- **Verticalidad (2.5D)**: VertLinks con PortalCap, cambio de FloorId

## Configuración por Defecto (que "enciende")

### Grid Defaults
- **CellSize**: 0.6m (balance entre granularidad y performance)
- **TickHz**: 12-15 Hz (lógica de movimiento)
- **TurnRateMax**: 140°/s con Deadband=6°
- **Cuantización**: 16 direcciones ON

### Capacidades
- **CellCapacity**: pasillo 1m=1; 1.6m=2; sala=3-4
- **PortalCap**: 1-3 según ancho del embudo
- **Cooldown (Sleep)**: 8 ± 2 ticks
- **Wake**: por Epoch del destino

## Criterios de Éxito (lo que DEBE pasar)

### Embudos
✅ **Throughput estable** = PortalCap  
✅ **Masa atrás no gasta CPU** (sleep masivo)

### Persecución
✅ **Frente coherente**, sin serpenteo  
✅ **Followers reenganchan** sin pops visuales

### Campo Abierto  
✅ **Costo por tick** depende de celdas activas, NO de cuántos spawns
✅ **Escalabilidad lineal** con área activa

### Persistencia/Net
✅ **Reactivar/sincronizar** no altera resultados (determinismo)
✅ **Catch-up discreto** funciona correctamente

## Hardware Target & Optimizaciones

### Target Specs
- **CPU**: Intel i5/AMD Ryzen 5 (últimas generaciones)
- **RAM**: 32GB DDR4/DDR5  
- **GPU**: 8GB VRAM (RTX 3060/4060, RX 6600 XT+)
- **Storage**: SSD/NVMe

### Performance Strategy
- **Grid-based**: O(celdas activas) no O(entidades)
- **SOA**: cache locality para fragments
- **Tiling**: paralelismo por tiles (16×16)
- **Event-driven**: sleep masivo en atascos
- **Double-buffer**: sync sin locks

## Anti-patrones Críticos ❌
- ❌ **Data races** entre ECS y Grid
- ❌ **Reagrupar cada frame** (solo on-change)  
- ❌ **O(n·m) loops** sin límites
- ❌ **Solve antes de Cull/LOD**
- ❌ **Cambios frecuentes** en estructuras de fragments
