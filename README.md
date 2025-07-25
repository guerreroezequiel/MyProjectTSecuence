# Sistema de Zombis Masivos - Unreal Engine 5.5.4

## 📋 Descripción General

Este proyecto implementa un sistema de zombis masivos utilizando **Unreal Engine 5.5.4** con una arquitectura **State Sync** que separa completamente la lógica de juego (Mass Entity System) de la representación visual (TurboSequence). El sistema está diseñado para manejar miles de entidades con máximo rendimiento.

## 🏗️ Arquitectura del Sistema

### **State Sync Architecture**
- **Lógica (Mass Entity)**: Maneja AI, movimiento, estado, daño
- **Visual (TurboSequence)**: Maneja renderizado, animaciones, transformaciones
- **Separación completa**: No hay acoplamiento entre lógica y representación visual

### **Componentes Principales**

#### 1. **Mass Entity System (Lógica)**
- **Fragments**: Datos puros sin lógica
- **Processors**: Lógica que opera sobre fragments
- **Queries**: Define qué entidades procesa cada processor

#### 2. **TurboSequence (Visual)**
- **Mesh Assets**: Assets optimizados para renderizado masivo
- **Update Groups**: Distribución de carga de animación
- **Concurrent Operations**: Operaciones thread-safe

## 📁 Estructura de Archivos

### **Fragments (Datos)**
```
Source/MyProjectTSecuence/
├── ZombiStateFragment.h          # Estado lógico (Idle, Walk, Chase, etc.)
├── ZombiMovementFragment.h       # Posición, rotación, velocidad, dirección
└── ZombiTurboSequenceFragment.h  # Referencias visuales (MeshData, Asset)
```

### **Processors (Lógica)**
```
Source/MyProjectTSecuence/
├── ZombiMovementProcessor.h/.cpp     # Movimiento y AI básica
├── ZombiTurboSequenceProcessor.h/.cpp # Sincronización visual
└── ZombiUpdateProcessor.h/.cpp       # Update Groups de TurboSequence
```

### **Subsystems (Gestión)**
```
Source/MyProjectTSecuence/
├── ZombiMassSubsystem.h/.cpp         # Gestión de entidades Mass
└── ZombiSpawnerSubsystem.h/.cpp      # Spawning optimizado
```

### **Control y Testing**
```
Source/MyProjectTSecuence/
├── ZombiTestController.h/.cpp        # Control desde Blueprint
└── MyTurboSequenceAnimComponent.h/.cpp # Componente de animación (legacy)
```

## 🔧 Fragmentos del Sistema

### **FZombiStateFragment**
```cpp
struct FZombiStateFragment : public FMassFragment
{
    EZombiState State = EZombiState::Idle; // Idle, Walk, Chase, Attack, Hit, Death
};
```

### **FZombiMovementFragment**
```cpp
struct FZombiMovementFragment : public FMassFragment
{
    FVector Position;                    // Posición actual
    FRotator Rotation;                   // Rotación actual
    float MovementSpeed = 100.0f;        // Velocidad de movimiento
    float RotationSpeed = 90.0f;         // Velocidad de rotación
    FVector MovementDirection;           // Dirección actual
    float DirectionChangeTimer;          // Timer para cambio de dirección
    float DirectionChangeInterval = 3.0f; // Intervalo de cambio
    float MovementRadius = 500.0f;       // Radio de movimiento
    FVector MovementCenter;              // Centro del área
};
```

### **FZombiTurboSequenceFragment**
```cpp
struct FZombiTurboSequenceFragment : public FMassFragment
{
    FTurboSequence_MinimalMeshData_Lf MeshData;           // Handle visual
    FTurboSequence_AnimMinimalBlendSpaceCollection_Lf BlendSpaceData; // Blend Space para animaciones
    bool bIsVisualInstanceValid = false;                  // Estado de validación
    int32 UpdateGroupIndex = 0;                           // Grupo de actualización
    UTurboSequence_MeshAsset_Lf* TurboSequenceAsset;      // Asset de referencia
};
```

## ⚙️ Procesadores del Sistema

### **UZombiMovementProcessor**
- **Función**: Maneja movimiento, AI básica y cambios de estado
- **Fase**: `EMassProcessingPhase::PrePhysics`
- **Grupo**: `"MassBehavior"`
- **Características**:
  - Movimiento aleatorio con cambio de dirección
  - Rotación suave hacia la dirección de movimiento
  - Confinamiento en área circular
  - Transición automática entre estados Idle/Walk

### **UZombiTurboSequenceProcessor**
- **Función**: Sincroniza transformaciones y animaciones visuales
- **Fase**: `EMassProcessingPhase::PrePhysics`
- **Grupo**: `"MassBehavior"`
- **Características**:
  - ✅ **Sincronización de transformaciones** (FUNCIONAL)
  - ❌ **Animaciones Blend Space** (TEMPORALMENTE DESHABILITADAS)
  - Gestión de instancias visuales

### **UZombiUpdateProcessor**
- **Función**: Ejecuta Update Groups de TurboSequence
- **Fase**: `EMassProcessingPhase::PrePhysics`
- **Grupo**: `"MassBehavior"`
- **Características**:
  - ✅ **Distribución de carga** en 4 grupos
  - ❌ **SolveMeshes_GameThread** (CAUSA CRASH - DESHABILITADO)

## 🚀 Subsystems

### **UZombiMassSubsystem**
- **Función**: Gestión centralizada de entidades Mass
- **Características**:
  - Registro/desregistro de entidades
  - Gestión de procesadores automática
  - Limpieza de entidades

### **UZombiSpawnerSubsystem**
- **Función**: Spawning optimizado de zombis
- **Características**:
  - Spawning en lotes para rendimiento
  - ✅ **Creación de instancias visuales TurboSequence** (FUNCIONAL)
  - ✅ **Distribución en Update Groups** (FUNCIONAL)
  - ❌ **Configuración de animaciones** (TEMPORALMENTE DESHABILITADA)

## 🎮 Control desde Blueprint

### **AZombiTestController**
```cpp
// Funciones disponibles en Blueprint:
void SpawnZombiBatch(int32 Count = 100);           // Spawn masivo
void SpawnSingleZombi();                           // Spawn individual
void ClearAllZombis();                             // Limpiar todos
int32 GetActiveZombiCount();                       // Contar activos
void SetTurboSequenceAsset(UTurboSequence_MeshAsset_Lf* Asset); // Configurar asset
```

## 🔄 Flujo de Ejecución

### **1. Inicialización**
```
GameMode → ZombiTestController → ZombiSpawnerSubsystem → ZombiMassSubsystem
```

### **2. Spawning**
```
SpawnZombiBatch() → CreateZombiMassEntity() → CreateTurboSequenceVisualInstance()
```

### **3. Ejecución por Frame**
```
ZombiMovementProcessor → ZombiTurboSequenceProcessor → ZombiUpdateProcessor
```

### **4. Sincronización Visual**
```
Estado Lógico → Sincronización de Transformaciones → TurboSequence → Renderizado
```

## 🎯 Estados del Zombi

```cpp
enum class EZombiState : uint8
{
    Idle,    // Quieto - Animación: "Idle"
    Walk,    // Caminando - Animación: "Walk"
    Chase,   // Persiguiendo - Animación: "Run"
    Attack,  // Atacando - Animación: "Attack"
    Hit,     // Recibiendo golpe - Animación: "Hit"
    Death    // Muerto - Animación: "Death"
};
```

## 🔧 Configuración Requerida

### **1. Asset de TurboSequence**
- Crear asset de TurboSequence con animaciones nombradas:
  - `Idle`
  - `Walk`
  - `Run`
  - `Attack`
  - `Hit`
  - `Death`

### **2. Configuración en Blueprint**
- Colocar `AZombiTestController` en el nivel
- Asignar el asset de TurboSequence
- Configurar parámetros de spawning

### **3. Configuración de Procesadores**
```cpp
// Configuración automática en constructores:
ProcessingPhase = EMassProcessingPhase::PrePhysics;
ExecutionOrder.ExecuteInGroup = TEXT("MassBehavior");
bRequiresGameThreadExecution = false;
bAutoRegisterWithProcessingPhases = true;
```

## 🚨 Problemas Resueltos

### **1. ContextType Mismatch**
- **Problema**: `ExecutionContext.ExecutionType != ExpectedContextType`
- **Solución**: Uso correcto de constructores `FMassEntityQuery{*this}` y registro automático

### **2. Registro Manual de Procesadores**
- **Problema**: `RegisterProcessor()` no existe en UE5.5.4
- **Solución**: Uso de `bAutoRegisterWithProcessingPhases = true`

### **3. Crashes en TurboSequence**
- **Problema**: `SolveMeshes_GameThread` causa crash `GetShadowIndex() == 0`
- **Solución**: Deshabilitación temporal de animaciones, mantenimiento de sincronización de transformaciones

### **4. Logs Excesivos**
- **Problema**: Logs constantes que afectaban rendimiento
- **Solución**: Limpieza de logs y uso de timers para logs informativos

## ✅ Estado Actual del Sistema

### **✅ Funcionalidades Completamente Operativas**
- ✅ **Sistema Mass Entity** completamente funcional
- ✅ **Arquitectura State Sync** implementada correctamente
- ✅ **TurboSequence** integrado para visualización
- ✅ **Zombis se mueven** con sincronización perfecta de transformaciones
- ✅ **Rendimiento optimizado** con grupos de actualización
- ✅ **Sistema estable** sin crashes

### **❌ Funcionalidades Temporalmente Deshabilitadas**
- ❌ **Animaciones** (zombis en T-pose)
- ❌ **Blend Space** (causa crash en `SolveMeshes_GameThread`)
- ❌ **Reproducción de animaciones** (problema de threading)

### **🎯 Próximo Objetivo**
- 🔍 **Resolver sistema de animaciones** sin usar `SolveMeshes_GameThread`
- 🔍 **Implementar animaciones alternativas** compatibles con TurboSequence
- 🔍 **Mantener estabilidad** del sistema actual

## 📊 Rendimiento

### **Optimizaciones Implementadas**
- **Update Groups**: Distribución de carga de animación en 4 grupos
- **Concurrent Operations**: Operaciones thread-safe de TurboSequence
- **Batch Processing**: Spawning en lotes de 100 entidades
- **Query Optimization**: Queries registrados automáticamente

### **Escalabilidad**
- **Diseñado para**: Miles de entidades
- **Arquitectura**: State Sync para máxima separación
- **Rendimiento**: Procesamiento paralelo y optimizado

## 🎮 Uso del Sistema

### **1. Configuración Inicial**
```cpp
// En Blueprint o C++:
AZombiTestController* Controller = GetWorld()->SpawnActor<AZombiTestController>();
Controller->SetTurboSequenceAsset(MyTurboSequenceAsset);
```

### **2. Spawning de Zombis**
```cpp
// Spawn masivo
Controller->SpawnZombiBatch(1000);

// Spawn individual
Controller->SpawnSingleZombi();
```

### **3. Control en Tiempo Real**
```cpp
// Limpiar todos
Controller->ClearAllZombis();

// Obtener conteo
int32 Count = Controller->GetActiveZombiCount();
```

## 🔮 Próximos Pasos

### **Prioridad Alta - Animaciones**
1. **Investigar alternativas** a `SolveMeshes_GameThread`
2. **Implementar animaciones básicas** sin Blend Space
3. **Usar animaciones directas** de TurboSequence
4. **Mantener estabilidad** del sistema actual

### **Mejoras Futuras**
1. **AI Avanzada**: Implementar pathfinding y comportamiento más complejo
2. **Sistema de Daño**: Agregar fragmentos de salud y daño
3. **Optimización Visual**: LOD y culling para miles de entidades
4. **Networking**: Sincronización en red para multiplayer
5. **Configuración**: Sistema de configuración por Blueprint más robusto

### **Extensibilidad**
- **Nuevos Estados**: Fácil agregar nuevos estados de zombi
- **Nuevos Fragmentos**: Extensión simple para nuevas funcionalidades
- **Nuevos Procesadores**: Arquitectura modular para nuevas lógicas

## 📝 Notas Técnicas

### **Versiones Utilizadas**
- **Unreal Engine**: 5.5.4
- **TurboSequence**: Plugin de terceros
- **Mass Entity**: Sistema nativo de UE5

### **Dependencias**
- `MassEntity`
- `TurboSequence_Lf`
- `CoreMinimal`
- `Engine`

### **Compatibilidad**
- **Plataforma**: Windows
- **Configuración**: Development_Editor
- **Arquitectura**: x64

### **Problemas Conocidos**
- **TurboSequence SolveMeshes**: Causa crash `GetShadowIndex() == 0`
- **Animaciones**: Temporalmente deshabilitadas por estabilidad
- **Threading**: Conflictos entre Mass Entity y TurboSequence

---

**Sistema funcional con arquitectura State Sync completa. Próximo objetivo: resolver sistema de animaciones manteniendo la estabilidad actual.** 