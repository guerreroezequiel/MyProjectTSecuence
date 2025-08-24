# Sistema ECS Zombis - Unreal Engine 5.5.4

## 📋 Descripción
Sistema de zombis usando **Mass Entity System** + **TurboSequence** con arquitectura **State Sync**. Diseñado para manejar miles de entidades con optimización LOD por frecuencia.

## 🏗️ Arquitectura

### **Arquitectura Optimizada: Tags por Frecuencia + Estados para TurboSequence**
```
Tags (LOD y Optimización) ←→ Sincronización ←→ Estados (TurboSequence)
```

**¿Por qué esta Arquitectura?**
- **Tags por Frecuencia**: LOD dinámico y optimización de rendimiento
- **Estados**: Claridad para animaciones en TurboSequence
- **Sincronización**: Estado → Tags de Frecuencia → TurboSequence

### **State Sync Pattern**
```
Lógica (Mass Entity) ←→ Sincronización ←→ Visual (TurboSequence)
```

### **Componentes Principales**

#### **Fragmentos (Datos)**
- `FZombiCoreFragment`: Posición, rotación, velocidad
- `FZombiBehaviorFragment`: Estados y datos de comportamiento
- `FZombiTurboSequenceFragment`: Referencias visuales + Estados para animaciones
- `FZombiLODFragment`: Optimización y LOD (datos estables)

#### **Tags (LOD y Optimización)**
- `FActiveTag`: Entidades activas (base para queries)
- `FDeadTag`: Entidades muertas (excluir de procesamiento)
- `FInFrustumTag`: Visible en cámara (frustum culling)
- `FUpdate60FPS`: Frecuencia crítica (TakeDamage, Attack)
- `FUpdate30FPS`: Frecuencia alta (Chase)
- `FUpdate15FPS`: Frecuencia normal (Seek, WalkAround)
- `FUpdate5FPS`: Frecuencia mínima (Idle, Dead)

#### **Procesadores (Lógica)**
- `UZombiBehaviorProcessor`: Lógica de comportamiento y gestión de estados
- `UZombiMovementProcessor`: Movimiento y rotación (optimizado con LOD)
- `UZombiTurboSequenceProcessor`: Sincronización visual + Estados → Animaciones
- `UZombiLODProcessor`: LOD inteligente y sincronización Estado → Tags de Frecuencia

#### **Subsystems (Gestión)**
- `UZombiMassSubsystem`: Gestión de entidades Mass
- `UZombiSpawnerSubsystem`: Spawning optimizado
- `AZombiTestController`: Control desde Blueprint

## 🔄 Flujo de Ejecución

### **1. Spawning**
```
ZombiTestController → SpawnerSubsystem → MassSubsystem → TurboSequence
```

### **2. Por Frame**
```
LOD (Estado → Tags de Frecuencia) → Behavior (Estados) → Movement → TurboSequence
```

### **3. Sincronización Visual**
```
Estado Lógico → Tags de Frecuencia → TurboSequence → Renderizado
```

## 🎮 Uso

### **Configuración**
1. Colocar `AZombiTestController` en el nivel
2. Asignar asset de TurboSequence
3. Configurar parámetros de spawning

### **Funciones Blueprint**
```cpp
SpawnZombiBatch(100);        // Spawn masivo
SpawnSingleZombi();          // Spawn individual
ClearAllZombis();            // Limpiar todos
GetActiveZombiCount();       // Contar activos
```

## 📊 Rendimiento

### **Optimizaciones**
- **Fragmentos Especializados**: Datos por procesador
- **Batch Processing**: Spawning en lotes
- **Concurrent Operations**: Thread-safe
- **Query Optimization**: Filtrado con tags por frecuencia
- **LOD Inteligente**: Frecuencia de update por estado
- **Frustum Culling**: Solo renderizar zombis visibles
- **Sincronización Estado-Tags**: Consistencia automática

### **Escalabilidad**
- **Diseñado para**: Miles de entidades
- **Arquitectura**: State Sync + Tags por Frecuencia
- **Rendimiento**: Procesamiento optimizado con LOD dinámico
- **Objetivo**: 5000-10000 zombis a 30-60 FPS

## 🔧 Configuración Técnica

### **Dependencias**
- `MassEntity` (UE5 nativo)
- `TurboSequence_Lf` (plugin)
- `EnhancedInput` (UE5 nativo)

### **Fases de Procesamiento**
- **PrePhysics**: Lógica (movimiento, AI, LOD)
- **PostPhysics**: Visual (TurboSequence sync)

### **Orden de Ejecución de Procesadores**

#### **Fase PrePhysics:**
1. **LODProcessor** (Grupo: MassLOD)
   - **Prioridad:** PRIMERO - `ExecuteBefore MassBehavior`
   - **Función:** Sincroniza Estado → Tags de Frecuencia
   - **Patrón:** Command (comandos diferidos)

2. **BehaviorProcessor** (Grupo: MassBehavior)
   - **Prioridad:** DESPUÉS de LOD
   - **Función:** IA y decisiones de comportamiento
   - **Queries:** Por frecuencia (60FPS → 30FPS → 15FPS → 5FPS)

3. **MovementProcessor** (Grupo: MassBehavior)
   - **Prioridad:** DESPUÉS de Behavior
   - **Función:** Movimiento y rotación
   - **Queries:** Por frecuencia (60FPS → 30FPS → 15FPS → 5FPS)

#### **Fase PostPhysics:**
4. **TurboSequenceProcessor** (Grupo: MassVisual)
   - **Prioridad:** DESPUÉS de MassBehavior
   - **Función:** State Sync (Mass Entity → TurboSequence)
   - **Dependencias:** `ExecuteAfter MassBehavior`

### **Tags de Filtrado por Frecuencia**
- `FActiveTag`: Entidades activas
- `FDeadTag`: Entidades muertas (excluidas)
- `FInFrustumTag`: Zombis visibles en cámara
- `FUpdate60FPS`: Zombis críticos (60 FPS)
- `FUpdate30FPS`: Zombis alta prioridad (30 FPS)
- `FUpdate15FPS`: Zombis normal (15 FPS)
- `FUpdate5FPS`: Zombis mínima prioridad (5 FPS)

## 📁 Estructura de Archivos

```
Source/MyProjectTSecuence/
├── Public/Systems/Zombies/ECS/
│   ├── Fragments/           # Datos
│   ├── Processors/          # Lógica
│   ├── Subsystems/          # Gestión
│   ├── Controllers/         # Control
│   └── Tags/               # LOD y Optimización
└── Private/Systems/Zombies/ECS/
    └── [Implementaciones .cpp]
```

## 🎯 Estados del Zombi

```cpp
enum EZombiState {
    Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead
}
```

## ✅ Funcionalidades Implementadas

- **✅ Spawning**: Lotes de zombis
- **✅ Movimiento**: AI básica con rotación
- **✅ Seek y Chase**: Comportamiento de persecución funcional
- **✅ Sincronización Visual**: Transformaciones básicas
- **✅ Animaciones**: Estados por entidad
- **✅ Optimización**: Fragmentos y procesadores especializados
- **✅ LOD Inteligente**: Frecuencia de update por estado
- **✅ Frustum Culling**: Solo renderizar zombis visibles
- **✅ Control Blueprint**: Interfaz desde editor

## 🎯 Arquitectura Optimizada - Tags por Frecuencia + Estados para TurboSequence

### **¿Por qué esta Arquitectura?**

#### **Problema Tradicional:**
- **Solo Estados**: Queries menos eficientes, filtrado manual
- **Solo Tags**: Estados complejos, debugging difícil, inconsistencias
- **LOD Complejo**: Fragmentos con datos que cambian frecuentemente

#### **Solución Optimizada:**
- **Tags por Frecuencia**: LOD dinámico y optimización de rendimiento
- **Estados**: Claridad para animaciones en TurboSequence
- **Sincronización**: Estado → Tags de Frecuencia → TurboSequence

### **Beneficios de la Arquitectura Optimizada:**

#### **Rendimiento:**
- **Queries por Frecuencia**: Tags permiten filtrado directo por LOD
- **LOD Dinámico**: Frecuencia basada en estado actual
- **Culling Eficiente**: Solo procesar zombis visibles
- **Batch Processing**: Mejor cache locality

#### **Mantenibilidad:**
- **Tags Claros**: FUpdate60FPS es más claro que FHighPriorityTag
- **Estados Visuales**: Solo para animaciones en TurboSequence
- **Lógica Centralizada**: Transiciones de estado en BehaviorProcessor
- **Consistencia**: Estado → Tags de Frecuencia sincronizados

#### **Escalabilidad:**
- **Extensibilidad**: Fácil agregar nuevas frecuencias
- **Flexibilidad**: Cambiar mapeo estado → frecuencia sin cambiar código
- **Optimización Incremental**: Agregar optimizaciones sin romper código

### **Implementación:**
```cpp
// Tags por frecuencia de actualización
FUpdate60FPS        // TakeDamage, Attack - Crítico
FUpdate30FPS        // Chase - Alta prioridad
FUpdate15FPS        // Seek, WalkAround - Normal
FUpdate5FPS         // Idle, Dead - Mínima

// Estados solo para TurboSequence
enum class EZombiState : uint8 { Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead };

// Sincronización automática
void SyncStateToFrequency() {
    // Estado → Tags de Frecuencia → TurboSequence
}
```

## 🔮 Próximos Pasos
Ver `HAZME.md` para roadmap detallado de desarrollo.

---

**Sistema optimizado para miles de entidades con arquitectura State Sync + Tags por Frecuencia + Estados para TurboSequence.** 