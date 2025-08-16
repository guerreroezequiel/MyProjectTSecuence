# Sistema ECS Zombis - Unreal Engine 5.5.4

## 📋 Descripción
Sistema de zombis masivos usando **Mass Entity System** + **TurboSequence** con arquitectura **State Sync**. Maneja miles de entidades con máximo rendimiento.

## ✅ Estado Actual
- **✅ Sistema Funcional**: Zombis se mueven, rotan y sincronizan visualmente
- **✅ State Sync**: Lógica (Mass) separada de visual (TurboSequence)
- **✅ Optimizado**: Fragmentos especializados y procesadores eficientes
- **✅ Escalable**: Preparado para miles de entidades

## 🏗️ Arquitectura

### **State Sync Pattern**
```
Lógica (Mass Entity) ←→ Sincronización ←→ Visual (TurboSequence)
```

### **Componentes Principales**

#### **Fragmentos (Datos)**
- `FZombiCoreFragment`: Posición, rotación, velocidad
- `FZombiBehaviorFragment`: Estados, AI, timers
- `FZombiCombatFragment`: Salud, daño, cooldowns
- `FZombiTurboSequenceFragment`: Referencias visuales
- `FZombiUpdateFrequencyFragment`: Control de frecuencia de update

#### **Procesadores (Lógica)**
- `UZombiMovementProcessor`: Movimiento y rotación
- `UZombiBehaviorProcessor`: Estados y AI
- `UZombiCombatProcessor`: Combate y daño
- `UZombiTurboSequenceProcessor`: Sincronización visual
- `UZombiPeriodicChaseProcessor`: Persecución periódica

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
Behavior → Combat → Movement → TurboSequence (State Sync)
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
- **Fragmentos Especializados**: Solo datos necesarios por procesador
- **Update Frequency Control**: Diferentes frecuencias por distancia
- **Batch Processing**: Spawning en lotes
- **Concurrent Operations**: Thread-safe
- **Query Optimization**: Filtrado inteligente

### **Escalabilidad**
- **Diseñado para**: Miles de entidades
- **Arquitectura**: State Sync para máxima separación
- **Rendimiento**: Procesamiento paralelo optimizado

## 🔧 Configuración Técnica

### **Dependencias**
- `MassEntity` (UE5 nativo)
- `TurboSequence_Lf` (plugin)
- `EnhancedInput` (UE5 nativo)

### **Fases de Procesamiento**
- **PrePhysics**: Lógica (movimiento, AI, combate)
- **PostPhysics**: Visual (TurboSequence sync)

### **Tags de Filtrado**
- `FActiveTag`: Entidades activas
- `FDeadTag`: Entidades muertas (excluidas)

## 📁 Estructura de Archivos

```
Source/MyProjectTSecuence/
├── Public/Systems/Zombies/ECS/
│   ├── Fragments/           # Datos especializados
│   ├── Processors/          # Lógica por dominio
│   ├── Subsystems/          # Gestión centralizada
│   ├── Controllers/         # Control Blueprint
│   └── Tags/               # Filtrado inteligente
└── Private/Systems/Zombies/ECS/
    └── [Implementaciones .cpp]
```

## 🎯 Estados del Zombi

```cpp
enum EZombiState {
    Stand, WalkAround, Chase, Attack, TakeDamage, Dead
}
```

## ✅ Funcionalidades Implementadas

- **✅ Spawning Masivo**: Lotes de 100+ zombis
- **✅ Movimiento**: AI básica con rotación suave
- **✅ Sincronización Visual**: Transformaciones perfectas
- **✅ Animaciones**: Estados individuales por entidad
- **✅ Optimización**: Fragmentos y procesadores especializados
- **✅ Control Blueprint**: Interfaz completa desde editor

## 🔮 Próximos Pasos
Ver `HAZME.md` para roadmap detallado de desarrollo.

---

**🎉 Sistema completamente funcional y optimizado para miles de entidades con arquitectura State Sync.** 