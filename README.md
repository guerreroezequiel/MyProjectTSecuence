# Sistema ECS Zombis - Unreal Engine 5.5.4

## 📋 Descripción
Sistema de zombis usando **Mass Entity System** + **TurboSequence** con arquitectura **State Sync**. Diseñado para manejar cientos de entidades con buen rendimiento.

## 🏗️ Arquitectura

### **State Sync Pattern**
```
Lógica (Mass Entity) ←→ Sincronización ←→ Visual (TurboSequence)
```

### **Componentes Principales**

#### **Fragmentos (Datos)**
- `FZombiCoreFragment`: Posición, rotación, velocidad
- `FZombiBehaviorFragment`: Estados, AI, timers
- `FZombiStimuliFragment`: Estímulos y respuestas
- `FZombiTurboSequenceFragment`: Referencias visuales

#### **Procesadores (Lógica)**
- `UZombiBehaviorProcessor`: Estados y AI
- `UZombiMovementProcessor`: Movimiento y rotación
- `UZombiStimulusProcessor`: Procesamiento de estímulos
- `UZombiTurboSequenceProcessor`: Sincronización visual

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
- **Query Optimization**: Filtrado

### **Escalabilidad**
- **Diseñado para**: Cientos de entidades
- **Arquitectura**: State Sync
- **Rendimiento**: Procesamiento optimizado

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
    Idle, WalkAround, Chase
}
```

## ✅ Funcionalidades Implementadas

- **✅ Spawning**: Lotes de zombis
- **✅ Movimiento**: AI básica con rotación
- **✅ Sincronización Visual**: Transformaciones básicas
- **✅ Animaciones**: Estados por entidad
- **✅ Optimización**: Fragmentos y procesadores especializados
- **✅ Control Blueprint**: Interfaz desde editor

## 🔮 Próximos Pasos
Ver `HAZME.md` para roadmap detallado de desarrollo.

---

**Sistema funcional para cientos de entidades con arquitectura State Sync.** 