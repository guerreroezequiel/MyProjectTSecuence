# ECS-V2: Grid + ECS + DOP para 3000+ entidades

## Arquitectura DOP: Grid Discreto + ECS Puro

### Grid System (12-15 Hz)
- **CellSize fijo**: 0.6m (balance granularidad/performance)
- **Dispatcher**: Enqueue → Dispatch → Commit (1 pass por tick)
- **Event-Driven**: Sleep/Wake por Epoch (no CPU en atascos)
- **Costo**: O(celdas activas), NO O(entidades)

### Fragment Unificado DOP (32 bytes)
```cpp
struct GridEntityFragment {
    // Grid (16 bytes)
    int16 CellX, CellY;      // posición discreta
    uint8 FloorId;           // piso actual  
    uint8 DirWish;           // 0=stay, 1-8=dirs
    uint8 SleepTicks;        // countdown sleep
    uint8 Flags;             // Role(2b) + Sleep(1b) + Active(1b)
    
    // Visual (8 bytes)
    uint16 YawQ;             // cuantizado 16 dirs
    uint16 GroupId;          // id de grupo
    uint8 SlotIndex;         // slot en grupo
    uint8 _pad[3];
    
    // TS Interp (8 bytes)
    float WorldX, WorldY;    // para 60fps smooth
};
```

### Grid Globals SOA (fuera ECS)
```cpp
TArray<uint8> CellCapacity[MaxCells];    // capacidad por celda
TArray<uint8> CellOccupancy[MaxCells];   // ocupación actual
TArray<uint32> CellQueues[MaxCells][8];  // colas por dirección
TArray<uint16> CellEpoch[MaxCells];      // para wake/sleep
```

## Procesadores DOP (3 únicos)

### 1. IntentProcessor (12-15 Hz)
```cpp
// Procesa TODOS los roles en 1 query
Query: GridEntityFragment (ALL roles)
- Leaders: heading suave + cap giro 140°/s
- Followers: offset hacia líder (slots blandos)  
- Neutrals: wander/flow → DirWish
```

### 2. GridDispatcher (12-15 Hz) 
```cpp
// Enqueue + Dispatch + Commit en 1 pass
Query: GridEntityFragment WHERE !Sleep
- Enqueue: empuja a CellQueues[dir] por DirWish
- Dispatch: concede por capacidad + prioridad
- Commit: actualiza CellX,CellY + Epoch
- Sleep: marca perdedores (SleepTicks = 8±2)
```

### 3. VisualProcessor (60 FPS)
```cpp  
// Interpolación + Write Buffer
Query: GridEntityFragment WHERE Active
- Yaw: suaviza rotación + cuantización 16 dirs
- World: Cell→World coords para smooth movement
- Write: empaqueta a WriteBuffer (no directamente a TS)
```

## State Sync Pattern (ECS ↔ TurboSequence)

### Participantes del Sync
```cpp
// ECS Side (Game Thread)
- IntentProcessor, GridProcessor, VisualProcessor (12-15 Hz)
- VisualProcessor: escribe a WriteBuffer
- ECS Coordinator: llama SyncPoint.SwapBuffers() al final del tick

// TurboSequence Side (Render Thread / Worker Thread)  
- TS Big Loop: lee ReadBuffer cuando necesita (60 FPS)
- No espera a ECS, usa último snapshot disponible
- TSIntegration: consume ReadBuffer → TS instances
```

### Timeline Detallado
```
Frame N:
├─ ECS Tick (Game Thread):
│  ├─ [12ms] IntentProcessor → GridProcessor → VisualProcessor
│  ├─ [1ms] VisualProcessor → WriteBuffer (nuevos datos)
│  └─ [0.1ms] SyncPoint.SwapBuffers() (WriteBuffer ↔ ReadBuffer)
│
└─ TS Big Loop (Render/Worker Thread):
   ├─ [16ms] TSIntegration consume ReadBuffer (datos del Frame N-1)
   ├─ [2ms] TS procesa animaciones + LOD + culling  
   └─ [14ms] TS rendering/dispatch

Frame N+1:
├─ ECS Tick: usa ReadBuffer como WriteBuffer (swap)
└─ TS Big Loop: usa nuevo ReadBuffer (datos del Frame N)
```

### SyncPoint Responsibilities
```cpp
class SyncPoint {
    // Llamado por ECS Coordinator al final de cada tick
    void SwapBuffers();           // Atomic swap WriteBuffer ↔ ReadBuffer
    
    // Llamado por TS Big Loop cuando necesita datos
    TSSnapshot* GetReadBuffer();  // Thread-safe read access
    
    // Llamado por VisualProcessor durante tick ECS
    TSSnapshot* GetWriteBuffer(); // Exclusive write access
};
```

## Estructura de Carpetas MVP

```
ECS-V2/
├── Core/
│   ├── Grid/
│   │   ├── GridGlobals           # SOA arrays + math utils
│   │   └── GridDispatcher        # Core dispatcher logic
│   └── Config/
│       ├── GridDefaults          # Constantes del sistema
│       └── GridTypes             # Tipos básicos (direcciones, flags)
├── ECS/
│   ├── Fragments/
│   │   └── GridEntityFragment    # Fragment unificado (32 bytes)
│   ├── Processors/
│   │   ├── IntentProcessor       # Decisión movimiento (todos roles)
│   │   ├── GridProcessor         # Dispatch del grid
│   │   └── VisualProcessor       # Interpolación + WriteBuffer
│   └── Coordinator/
│       └── ECSCoordinator        # Orchestration + sync timing
├── Sync/
│   ├── StateSync/
│   │   ├── SyncPoint             # Coordinación ECS ↔ TS
│   │   └── BufferSwap            # Double buffer management
│   └── TurboSequence/
│       ├── TSSnapshot            # Estructura de snapshot
│       ├── TSIntegration         # Bridge ECS → TS big loop
│       └── TSEntityMapping       # EntityId ↔ TSInstanceId
└── Testing/
    ├── GridTests                 # Unit tests para grid
    ├── ProcessorTests            # Tests para processors
    └── IntegrationTests          # Tests end-to-end
```

### Definición de Componentes MVP

#### **Core/Grid/**
- **GridGlobals**: SOA arrays globales (CellCapacity[], CellOccupancy[], CellQueues[][8], CellEpoch[]). Math utilities (coord→cell, direcciones, validaciones)
- **GridDispatcher**: Lógica central de enqueue intenciones por celda/dir, dispatch con prioridad Leader>Follower>Neutral, commit updates + sleep/wake por Epoch

#### **Core/Config/**
- **GridDefaults**: Constantes (CELL_SIZE=0.6f, TICK_HZ=12, SLEEP_TICKS=8, CELL_CAP_CORRIDOR=1, CELL_CAP_ROOM=3, TurnRate=140°/s)
- **GridTypes**: Enums (Direction 0-8, Role Leader/Follower/Neutral, Flags bits, FloorId type, basic grid math types)

#### **ECS/Fragments/**
- **GridEntityFragment**: Fragment unificado 32 bytes (CellX/Y, FloorId, DirWish, SleepTicks, Flags, YawQ, GroupId, SlotIndex, WorldX/Y)

#### **ECS/Processors/**
- **IntentProcessor**: Procesa TODOS los roles en 1 query (Leaders: heading suave + cap giro, Followers: offset hacia líder con slots blandos, Neutrals: wander/flow → DirWish)
- **GridProcessor**: Enqueue + Dispatch + Commit en 1 pass (empuja a CellQueues por DirWish, concede por capacidad, actualiza CellX/Y + Epoch, marca perdedores como Sleep)
- **VisualProcessor**: Interpolación 60fps (suaviza YawQ + cuantización 16 dirs, Cell→World coords, empaqueta WriteBuffer para TS)

#### **ECS/Coordinator/**
- **ECSCoordinator**: Orchestration (ejecuta pipeline Intent→Grid→Visual, timing de tick 12-15Hz, llama SyncPoint.SwapBuffers())

#### **Sync/StateSync/**
- **SyncPoint**: Coordinación thread-safe ECS↔TS (SwapBuffers() atomic, GetReadBuffer() para TS, GetWriteBuffer() para ECS)
- **BufferSwap**: Double buffer (WriteBuffer para ECS escribe, ReadBuffer para TS lee, swap sin data races, 1 frame delay aceptable)

#### **Sync/TurboSequence/**
- **TSSnapshot**: Estructura SOA para snapshot (transforms[], animations[], flags[], groupKeys[] optimizada para TS big loop)
- **TSIntegration**: Bridge final (consume ReadBuffer en TS big loop 60fps, no bloquea ECS, usa último snapshot disponible)
- **TSEntityMapping**: Mapeo bidireccional EntityId↔TSInstanceId, pool de instancias TS, spawn/despawn coordination

#### **Testing/**
- **GridTests**: Unit tests (GridGlobals math, GridDispatcher logic, capacity limits, sleep/wake cycles, determinismo)
- **ProcessorTests**: Tests (cada processor individual, pipeline completo, roles behavior, interpolación, snapshot building)
- **IntegrationTests**: End-to-end (100 entidades random, 500 con animaciones, 5 grupos×100, stress test 3000+ entidades, timing)

  

## Plan DOP (5 fases)

### **FASE 1: Grid Foundation** 🔥
- **GridGlobals**: SOA arrays + math utils
- **GridDispatcher**: enqueue/dispatch/commit básico
- **Testing**: grid vacío + intenciones mock

### **FASE 2: ECS Integration**
- **GridEntityFragment**: fragment unificado (32 bytes)
- **IntentProcessor**: todos los roles en 1 query
- **Testing**: 100 entidades neutral random

### **FASE 3: Visual Pipeline**
- **VisualProcessor**: interpolación + TS snapshot
- **Testing**: 500 entidades con animaciones

### **FASE 4: Groups**
- **Group logic**: en IntentProcessor (Leader/Follower)
- **Testing**: 5 grupos × 100 entidades

### **FASE 5: Scale**
- **Tiling**: paralelismo 16×16 celdas
- **Testing**: 3000+ entidades a 60 FPS

## Principios DOP

### Performance Core
- **1 Fragment**: 32 bytes, 1 archetype, máxima cache locality
- **3 Processors**: mínimo scheduling overhead ECS
- **Event-Driven**: Sleep/Wake por Epoch (no CPU en atascos)
- **O(celdas activas)**: costo independiente de #entidades

### Defaults que Funcionan
```cpp
constexpr float CELL_SIZE = 0.6f;        // granularidad/performance
constexpr uint8 TICK_HZ = 12;            // lógica movement
constexpr uint8 SLEEP_TICKS = 8;         // cooldown base
constexpr uint8 CELL_CAP_CORRIDOR = 1;   // pasillo 1m
constexpr uint8 CELL_CAP_ROOM = 3;       // sala abierta
```

### Criterios de Éxito ✅
- **Embudos**: throughput estable, masa dormida
- **Campo abierto**: costo O(celdas), no O(entidades)  
- **Grupos**: frente coherente sin serpenteo
- **Determinismo**: mismos inputs → mismos outputs

### Anti-patrones ❌
- ❌ Múltiples fragments relacionados
- ❌ Processors sobre-especializados
- ❌ Tags como flags (usar bits en fragment)
- ❌ O(n·m) loops sin caps
