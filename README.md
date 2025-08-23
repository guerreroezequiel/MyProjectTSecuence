# Sistema ECS Zombis - Unreal Engine 5.5.4

## 📋 Descripción
Sistema de zombis usando **Mass Entity System** + **TurboSequence** con arquitectura **State Sync**. Diseñado para manejar cientos de entidades con buen rendimiento.

## 🏗️ Arquitectura

### **Enfoque Híbrido: Estados + Tags**
```
Estados (Comportamiento) ←→ Sincronización ←→ Tags (Optimización)
```

**¿Por qué Híbrido?**
- **Estados**: Claridad y mantenibilidad para lógica de comportamiento
- **Tags**: Rendimiento máximo para queries y procesamiento
- **Sincronización**: Garantiza consistencia entre ambos sistemas

### **State Sync Pattern**
```
Lógica (Mass Entity) ←→ Sincronización ←→ Visual (TurboSequence)
```

### **Componentes Principales**

#### **Fragmentos (Datos)**
- `FZombiCoreFragment`: Posición, rotación, velocidad
- `FZombiBehaviorFragment`: Estados (Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead)
- `FZombiStimuliFragment`: Estímulos y respuestas
- `FZombiTurboSequenceFragment`: Referencias visuales
- `FZombiLODFragment`: Optimización y LOD (nuevo)

#### **Tags (Optimización)**
- `FActiveTag`: Zombis vivos y activos
- `FDeadTag`: Zombis muertos (no procesar)
- `FChasingTag`: Zombis persiguiendo
- `FAttackingTag`: Zombis atacando
- `FInFrustumTag`: Zombis visibles en cámara
- `FHighPriorityTag`: Zombis que necesitan 60 FPS

#### **Procesadores (Lógica)**
- `UZombiBehaviorProcessor`: Estados y AI (modificado para usar tags)
- `UZombiMovementProcessor`: Movimiento y rotación (optimizado con LOD)
- `UZombiStimulusProcessor`: Procesamiento de estímulos
- `UZombiTurboSequenceProcessor`: Sincronización visual (optimizado con culling)
- `UZombiLODProcessor`: LOD inteligente y sincronización estados-tags (nuevo)

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
Stimulus → Behavior → Movement → TurboSequence (State Sync)
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
- **✅ Sincronización Visual**: Transformaciones básicas
- **✅ Animaciones**: Estados por entidad
- **✅ Optimización**: Fragmentos y procesadores especializados
- **✅ Control Blueprint**: Interfaz desde editor

## 🎯 Enfoque Híbrido - Estados + Tags

### **¿Por qué esta Arquitectura?**

#### **Problema Tradicional:**
- **Solo Estados**: Queries menos eficientes, filtrado manual
- **Solo Tags**: Estados complejos, debugging difícil, inconsistencias

#### **Solución Híbrida:**
- **Estados**: Claridad y mantenibilidad para lógica de comportamiento
- **Tags**: Rendimiento máximo para queries y procesamiento
- **Sincronización**: Garantiza consistencia entre ambos sistemas

### **Beneficios del Enfoque Híbrido:**

#### **Rendimiento:**
- **Queries Nativas**: Tags permiten filtrado directo en chunks
- **LOD Inteligente**: Prioridad por estado + distancia + estímulos
- **Culling Eficiente**: Solo procesar zombis visibles
- **Batch Processing**: Mejor cache locality

#### **Mantenibilidad:**
- **Estado Claro**: BehaviorFragment tiene estado único y obvio
- **Debugging Fácil**: Estado actual es inmediatamente visible
- **Lógica Centralizada**: Transiciones en un lugar
- **Consistencia**: No hay estados contradictorios

#### **Escalabilidad:**
- **Extensibilidad**: Fácil agregar nuevos tags sin cambiar lógica
- **Flexibilidad**: Combinaciones de tags cuando sea necesario
- **Optimización Incremental**: Agregar optimizaciones sin romper código

### **Implementación:**
```cpp
// Estados para comportamiento claro
enum class EZombiState : uint8 { Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead };

// Tags para optimización de queries
FActiveTag, FDeadTag, FChasingTag, FAttackingTag, FInFrustumTag, FHighPriorityTag

// Sincronización automática
void SetZombieState(EZombiState NewState) {
    // Cambiar estado + sincronizar tags automáticamente
}
```

## 🔮 Próximos Pasos
Ver `HAZME.md` para roadmap detallado de desarrollo.

---

**Sistema optimizado para miles de entidades con arquitectura State Sync + Enfoque Híbrido.** 