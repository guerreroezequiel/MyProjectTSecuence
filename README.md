# Sistema ECS Zombis - Unreal Engine 5.5.4

## 📋 Descripción
Sistema de zombis usando **Mass Entity System** + **TurboSequence** con arquitectura **State Sync**. Diseñado para manejar cientos de entidades con buen rendimiento.

## 🏗️ Arquitectura

### **Arquitectura Optimizada: Tags para Lógica + Estados para TurboSequence**
```
Tags (Lógica y Optimización) ←→ Sincronización ←→ Estados (TurboSequence)
```

**¿Por qué esta Arquitectura?**
- **Tags**: Rendimiento máximo para queries y procesamiento
- **Estados**: Claridad para animaciones en TurboSequence
- **Sincronización**: Tags → Estados Visuales → TurboSequence

### **State Sync Pattern**
```
Lógica (Mass Entity) ←→ Sincronización ←→ Visual (TurboSequence)
```

### **Componentes Principales**

#### **Fragmentos (Datos)**
- `FZombiCoreFragment`: Posición, rotación, velocidad
- `FZombiBehaviorFragment`: Timers y datos de comportamiento
- `FZombiTurboSequenceFragment`: Referencias visuales + Estados para animaciones
- `FZombiLODFragment`: Optimización y LOD

#### **Tags (Lógica y Optimización)**
- `FActiveTag`: Entidades activas (base para queries)
- `FDeadTag`: Entidades muertas (excluir de procesamiento)
- `FChasingTag`: Persiguiendo al jugador
- `FAttackingTag`: Atacando al jugador
- `FSeekingTag`: Buscando al jugador
- `FWalkingTag`: Caminando aleatoriamente
- `FIdleTag`: Esperando en idle
- `FInFrustumTag`: Visible en cámara (frustum culling)
- `FHighPriorityTag`: Necesita 60 FPS (LOD crítico)

#### **Procesadores (Lógica)**
- `UZombiBehaviorProcessor`: Lógica de comportamiento y gestión de tags
- `UZombiMovementProcessor`: Movimiento y rotación (optimizado con LOD)
- `UZombiTurboSequenceProcessor`: Sincronización visual + Tags → Estados Visuales
- `UZombiLODProcessor`: LOD inteligente y frustum culling

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
Behavior (Tags) → Movement → TurboSequence (Tags → Estados Visuales)
```

### **3. Sincronización Visual**
```
Estado Lógico → Transformación → TurboSequence → Renderizado
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
- **Query Optimization**: Filtrado con tags
- **LOD Inteligente**: Frecuencia de update por prioridad
- **Frustum Culling**: Solo renderizar zombis visibles
- **Sincronización Estados-Tags**: Consistencia automática

### **Escalabilidad**
- **Diseñado para**: Miles de entidades
- **Arquitectura**: State Sync + Enfoque Híbrido
- **Rendimiento**: Procesamiento optimizado con LOD
- **Objetivo**: 5000-10000 zombis a 30-60 FPS

## 🔧 Configuración Técnica

### **Dependencias**
- `MassEntity` (UE5 nativo)
- `TurboSequence_Lf` (plugin)
- `EnhancedInput` (UE5 nativo)

### **Fases de Procesamiento**
- **PrePhysics**: Lógica (movimiento, AI, estímulos)
- **PostPhysics**: Visual (TurboSequence sync)

### **Tags de Filtrado**
- `FActiveTag`: Entidades activas
- `FDeadTag`: Entidades muertas (excluidas)
- `FChasingTag`: Zombis persiguiendo
- `FAttackingTag`: Zombis atacando
- `FInFrustumTag`: Zombis visibles en cámara
- `FHighPriorityTag`: Zombis que necesitan 60 FPS

## 📁 Estructura de Archivos

```
Source/MyProjectTSecuence/
├── Public/Systems/Zombies/ECS/
│   ├── Fragments/           # Datos
│   ├── Processors/          # Lógica
│   ├── Subsystems/          # Gestión
│   ├── Controllers/         # Control
│   └── Tags/               # Filtrado
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
- **✅ LOD Inteligente**: Frecuencia de update por prioridad
- **✅ Frustum Culling**: Solo renderizar zombis visibles
- **✅ Control Blueprint**: Interfaz desde editor

## 🎯 Arquitectura Optimizada - Tags para Lógica + Estados para TurboSequence

### **¿Por qué esta Arquitectura?**

#### **Problema Tradicional:**
- **Solo Estados**: Queries menos eficientes, filtrado manual
- **Solo Tags**: Estados complejos, debugging difícil, inconsistencias

#### **Solución Optimizada:**
- **Tags**: Rendimiento máximo para queries y procesamiento
- **Estados**: Claridad para animaciones en TurboSequence
- **Sincronización**: Tags → Estados Visuales → TurboSequence

### **Beneficios de la Arquitectura Optimizada:**

#### **Rendimiento:**
- **Queries Nativas**: Tags permiten filtrado directo en chunks
- **LOD Inteligente**: Prioridad por estado + distancia + estímulos
- **Culling Eficiente**: Solo procesar zombis visibles
- **Batch Processing**: Mejor cache locality

#### **Mantenibilidad:**
- **Tags Claros**: Comportamiento definido por tags específicos
- **Estados Visuales**: Solo para animaciones en TurboSequence
- **Lógica Centralizada**: Transiciones de tags en BehaviorProcessor
- **Consistencia**: Tags → Estados Visuales sincronizados

#### **Escalabilidad:**
- **Extensibilidad**: Fácil agregar nuevos tags sin cambiar lógica
- **Flexibilidad**: Combinaciones de tags cuando sea necesario
- **Optimización Incremental**: Agregar optimizaciones sin romper código

### **Implementación:**
```cpp
// Tags para comportamiento y optimización
FActiveTag, FDeadTag, FChasingTag, FAttackingTag, FSeekingTag, FWalkingTag, FIdleTag, FInFrustumTag, FHighPriorityTag

// Estados solo para TurboSequence
enum class EZombiState : uint8 { Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead };

// Sincronización automática
void SyncTagsToVisualState() {
    // Tags → Estados Visuales → TurboSequence
}
```

## 🔮 Próximos Pasos
Ver `HAZME.md` para roadmap detallado de desarrollo.

---

**Sistema optimizado para miles de entidades con arquitectura State Sync + Tags para Lógica + Estados para TurboSequence.** 