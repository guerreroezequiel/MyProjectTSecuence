# 🎯 Roadmap de Desarrollo - Sistema ECS Zombis

## 🚨 Prioridad Crítica - Correcciones del Sistema

### **1. Control de Frecuencia de Update**
- [ ] **Corregir flujo de UpdateFrequency**: El `LastUpdateTime` se actualiza incorrectamente en `UpdateFrequencyInfo()`
- [ ] **Centralizar cálculo de distancia**: Evitar calcular distancia al jugador múltiples veces por frame
- [ ] **Definir orden explícito de procesadores**: Usar `ExecutionOrder.ExecuteBefore/ExecuteAfter`

### **2. Seguridad de Hilos**
- [ ] **Resolver acceso a actores fuera del game thread**: `GetPlayerCharacter()` en procesadores concurrentes
- [ ] **Cachear posición del jugador**: En subsystem del game thread y leer solo `FVector` desde threads
- [ ] **Verificar thread safety**: Revisar todas las operaciones de TurboSequence

### **3. Estado de Muerte y Tags**
- [ ] **Unificar fuente de verdad del estado "dead"**: Decidir si vive en `Combat` o `Behavior`
- [ ] **Implementar uso correcto de DeadTag**: Agregar cuando muere, remover de queries activos
- [ ] **Corregir queries de procesadores**: Asegurar que entidades muertas no se procesen

## 🔧 Prioridad Alta - Mejoras del Sistema

### **4. Lógica de Combate**
- [ ] **Eliminar timers static compartidos**: Usar cooldowns por entidad en `CombatProcessor`
- [ ] **Implementar sistema de daño real**: Detección de colisiones con jugador
- [ ] **Agregar animaciones de ataque**: Estados Attack, TakeDamage, Death
- [ ] **Sistema de respawn**: Reutilizar entidades muertas

### **5. Optimización de Procesadores**
- [ ] **Definir relación entre OptimizedProcessor y MovementProcessor**: ¿Sustituye o complementa?
- [ ] **Parametrizar offset de rotación TS**: Hacer configurable el -90° hardcoded
- [ ] **Optimizar SolveMeshes**: Verificar que no se ejecute doble por frame
- [ ] **Implementar LOD visual**: Diferentes niveles de detalle por distancia

### **6. Limpieza y Organización**
- [ ] **Limpiar includes innecesarios**: Forward declarations en headers
- [ ] **Remover variables no usadas**: `VisualInstances` en SpawnerSubsystem
- [ ] **Reducir logs de hot-path**: Gatear por categorías y niveles
- [ ] **Eliminar comentarios redundantes**: "Log eliminado para optimización"

## 🎮 Prioridad Media - Funcionalidades de Juego

### **7. AI Avanzada**
- [ ] **Implementar pathfinding**: Sistema de navegación para zombis
- [ ] **Comportamiento de horda**: Lógica grupal y coordinación
- [ ] **Detección de obstáculos**: Evitar colisiones con geometría
- [ ] **Sistema de objetivos**: Priorizar targets por distancia/amenaza

### **8. Sistema de Estados Completo**
- [ ] **Transiciones de estado suaves**: Blend entre animaciones
- [ ] **Estados contextuales**: Reaccionar a eventos del entorno
- [ ] **Sistema de memoria**: Recordar eventos recientes
- [ ] **Comportamiento emergente**: Patrones complejos de grupo

### **9. Interacción con Jugador**
- [ ] **Sistema de detección**: Line of sight y audición
- [ ] **Reacción a daño**: Estados de dolor y recuperación
- [ ] **Sistema de alerta**: Propagación de información entre zombis
- [ ] **Comportamiento defensivo**: Huir cuando están débiles

## ⚡ Prioridad Baja - Optimizaciones Avanzadas

### **10. Rendimiento Extremo**
- [ ] **Sistema de pooling**: Reutilizar entidades para evitar alloc/dealloc
- [ ] **Culling avanzado**: Frustum culling y occlusion culling
- [ ] **Particionado espacial**: Grid/Octree para optimizar queries
- [ ] **Compresión de datos**: Reducir tamaño de fragmentos

### **11. Herramientas de Desarrollo**
- [ ] **Debug visual**: Mostrar estados, rutas, y métricas en tiempo real
- [ ] **Profiling tools**: Medir rendimiento de cada procesador
- [ ] **Editor de comportamiento**: Configurar AI desde Blueprint
- [ ] **Sistema de presets**: Diferentes tipos de zombis

### **12. Configuración y Flexibilidad**
- [ ] **Sistema de configuración robusto**: Parámetros por Blueprint
- [ ] **Diferentes tipos de zombis**: Variantes con comportamientos únicos
- [ ] **Sistema de eventos**: Comunicación entre sistemas
- [ ] **Modularidad**: Fácil agregar nuevos fragmentos/procesadores

## 🌐 Futuro - Networking y Escalabilidad

### **13. Multiplayer**
- [ ] **Sincronización en red**: Replicación de estados de entidades
- [ ] **Optimización de red**: Compresión y predicción
- [ ] **Sistema de autoridad**: Servidor vs cliente
- [ ] **LOD de red**: Diferentes niveles de detalle por distancia

### **14. Escalabilidad Extrema**
- [ ] **Miles de entidades**: Optimizaciones para 10,000+ zombis
- [ ] **Distribución de carga**: Multi-threading avanzado
- [ ] **Streaming de entidades**: Cargar/descargar por chunks
- [ ] **GPU compute**: Mover lógica a GPU donde sea posible

## 📊 Métricas de Éxito

### **Rendimiento Objetivo**
- [ ] **1000 zombis**: 60 FPS estable
- [ ] **5000 zombis**: 30 FPS mínimo
- [ ] **Memoria**: < 100MB para 1000 entidades
- [ ] **CPU**: < 5ms por frame para lógica

### **Funcionalidad Objetivo**
- [ ] **AI realista**: Comportamiento emergente observable
- [ ] **Interacción completa**: Daño, muerte, respawn
- [ ] **Estados fluidos**: Transiciones suaves entre animaciones
- [ ] **Escalabilidad**: Sistema estable con miles de entidades

## 🔄 Proceso de Desarrollo

### **Sprint 1: Correcciones Críticas**
1. Corregir UpdateFrequency
2. Resolver thread safety
3. Implementar DeadTag correctamente

### **Sprint 2: Combate Básico**
1. Sistema de daño real
2. Animaciones de ataque
3. Sistema de respawn

### **Sprint 3: AI Mejorada**
1. Pathfinding básico
2. Comportamiento de horda
3. Detección de obstáculos

### **Sprint 4: Optimización**
1. LOD visual
2. Pooling de entidades
3. Herramientas de debug

## 📝 Notas de Implementación

### **Consideraciones Técnicas**
- **Mantener State Sync**: No acoplar lógica con visual
- **Cache locality**: Fragmentos especializados por procesador
- **Thread safety**: Verificar todas las operaciones concurrentes
- **Extensibilidad**: Diseño modular para futuras funcionalidades

### **Patrones de Diseño**
- **State Sync**: Separación lógica/visual
- **ECS**: Entidades, componentes, sistemas
- **Observer**: Eventos entre sistemas
- **Factory**: Creación de entidades
- **Pool**: Reutilización de objetos

## 🎯 ARQUITECTURA FINAL - SISTEMA DE ESTÍMULOS Y DAÑO

### **Objetivo**
Crear sistema optimizado para 10,000 entidades con separación clara de responsabilidades y orden de procesamiento crítico.

### **Arquitectura de Componentes**

#### **Game Thread (60 FPS)**
```
├── PlayerSignals (emite señales del jugador)
├── DamageSystem (detecta colisiones y daño)
├── StimulusSubsystem (pre-procesa estímulos)
└── DamageSubsystem (pre-procesa daño)
```

#### **Batch Processing (Orden Crítico)**
```
1. DamageTakenProcessor (PRIMERO - vida/muerte)
2. StimulusProcessor (estímulos y señales)
3. BehaviorProcessor (IA y comportamiento)
4. DamageDealtProcessor (daño que hace el zombie)
5. MovementProcessor (movimiento)
6. TurboSequenceProcessor (sincronización visual)
```

### **Separación de Responsabilidades**

#### **StimulusSubsystem (Game Thread)**
- Pre-procesa señales del jugador
- Cachea información de estímulos
- Thread-safe para game thread

#### **StimulusProcessor (Batch Processing)**
- Distribuye estímulos a zombis
- Procesamiento en chunks optimizado
- Batch-safe (sin acceso a actores)

#### **DamageSubsystem (Game Thread)**
- Pre-procesa información de daño
- Cachea colisiones y eventos
- Thread-safe para game thread

#### **DamageTakenProcessor (Batch Processing)**
- Procesa daño recibido por zombis
- Aplica efectos de daño
- Early exit para zombis muertos

#### **DamageDealtProcessor (Batch Processing)**
- Procesa daño hecho por zombis
- Detección de ataques
- Aplicación de daño al jugador/objetivos

#### **BehaviorProcessor (Batch Processing)**
- Lógica de IA y comportamiento
- Estados de ataque y decisión
- Consume estímulos del StimulusProcessor

### **Tipos de Señales**
- **PlayerSignals**: Posición, movimiento, sonidos, visibilidad
- **DamageSignals**: Daño recibido, explosiones, balas
- **EnvironmentalSignals**: Sonidos del entorno, eventos
- **FutureSignals**: Otros jugadores, NPCs, vehículos

### **Optimizaciones para 10,000 Entidades**
- **Memory per entity**: < 50 bytes total
- **CPU per frame**: < 2ms para lógica
- **Batch processing**: > 95% en chunks
- **Early exit**: Zombis muertos no se procesan
- **Spatial queries**: < 1ms para búsquedas

### **Flujo de Datos Optimizado**
```
Game Thread:
├── PlayerSignals → StimulusSubsystem (pre-procesa)
├── DamageSystem → DamageSubsystem (pre-procesa)
└── Ambos subsystems → Cache de datos

Batch Processing (ORDEN CRÍTICO):
1. DamageTakenProcessor (vida/muerte - PRIMERO)
2. StimulusProcessor (estímulos)
3. BehaviorProcessor (IA y comportamiento)
4. DamageDealtProcessor (daño hecho por zombie)
5. MovementProcessor (movimiento)
6. TurboSequenceProcessor (visual)
```

### **Reglas de Desarrollo**
- **Data-Oriented Design**: Fragmentos simples y eficientes
- **No allocaciones dinámicas**: Solo datos primitivos
- **Batch processing estricto**: No romper chunks
- **Thread safety**: Separación game thread / batch
- **Cache locality**: Datos contiguos en memoria
- **Early exit**: Optimizar procesamiento

---

**🎯 Objetivo: Sistema de zombis masivos completamente funcional, optimizado y escalable con AI avanzada y rendimiento extremo.**
