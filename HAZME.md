# 🎯 Roadmap de Desarrollo - Sistema ECS Zombis

## 📊 Estado Actual del Sistema

### **✅ Lo que ya funciona:**
- **4 procesadores estables**: Stimulus, Behavior, Movement, TurboSequence
- **State Sync**: Separación lógica/visual con TurboSequence
- **Batch processing**: Procesamiento en chunks optimizado
- **Fragmentos especializados**: Solo datos necesarios por procesador
- **Sistema de estímulos básico**: Detección de jugador funcional

### **❌ Limitaciones actuales:**
- **Sin LOD**: Todas las entidades se procesan cada frame
- **Sin culling**: Todas las entidades se renderizan
- **Sin pooling**: Alloc/dealloc constante
- **Sin optimización espacial**: Queries O(N) para estímulos
- **Rendimiento limitado**: ~500-1000 zombis máximo

## 🎯 Objetivos Realistas

### **Fase 1: Optimización Básica (2-3 semanas)**
- **1000 zombis**: 60 FPS estable
- **Memoria**: < 200MB para 1000 entidades
- **CPU**: < 8ms por frame para lógica

### **Fase 2: Escalabilidad (2-3 semanas)**
- **5000 zombis**: 30 FPS estable
- **Sistema de LOD**: Diferentes frecuencias de update
- **Culling básico**: Solo renderizar zombis visibles

### **Fase 3: Funcionalidad (2-3 semanas)**
- **Sistema de daño**: Colisiones y muerte
- **Comportamiento de horda**: Lógica grupal básica
- **Pathfinding simple**: Navegación básica

## 🚀 Prioridad Crítica - Optimizaciones de Rendimiento

### **1. Update Frequency Control (ALTA)**
```cpp
// LOD inteligente por estado + distancia
- CRÍTICO (60 FPS): Zombis Attack, TakeDamage, o muy cercanos (< 200u)
- ALTO (30 FPS): Zombis Chase, Seek, o medios (200-500u) con estímulos
- NORMAL (15 FPS): Zombis WalkAround, o lejanos (500-800u) con estímulos
- BAJO (5 FPS): Zombis Idle lejanos (> 800u) sin estímulos
```
**Impacto esperado**: Reducir procesamiento de CPU en ~70% manteniendo reactividad

### **2. Frustum Culling (ALTA)**
```cpp
// Solo procesar zombis visibles en cámara isométrica
- Calcular frustum de cámara isométrica
- Cull entidades fuera de vista
- Integrar con TurboSequence para culling visual
```
**Impacto esperado**: Reducir overhead de render en ~60%

### **3. Entity Pooling (MEDIA)**
```cpp
// Reutilizar entidades en lugar de crear/destruir
- Pool de entidades pre-allocadas
- Activar/desactivar en lugar de spawn/despawn
- Reducir alloc/dealloc overhead
```
**Impacto esperado**: Reducir overhead de memoria en ~50%

### **4. Spatial Partitioning (MEDIA)**
```cpp
// Grid system para optimizar queries de estímulos
- Dividir mundo en grid de 100x100 unidades
- Solo procesar zombis en celdas adyacentes al jugador
- Reducir complejidad de O(N*M) a O(N+M)
```
**Impacto esperado**: Reducir overhead de estímulos en ~90%

## 🔧 Implementación Técnica - Enfoque Híbrido

### **Arquitectura Híbrida: Estados + Tags**

#### **Estados para Comportamiento (BehaviorFragment):**
```cpp
enum class EZombiState : uint8
{
    Idle,
    WalkAround,
    Seek, 
    Chase,
    TakeDamage,
    Attack,
    Dead
};

struct FZombiBehaviorFragment : public FMassFragment
{
    EZombiState CurrentState;    // Estado principal del zombie
    float StateTimer;            // Tiempo en estado actual
    float ActionTimer;           // Timer para acciones específicas
    // ... otros datos de comportamiento
};
```

#### **Tags para Optimización:**
```cpp
// Tags para queries eficientes y LOD
FActiveTag        // Zombis vivos y activos
FDeadTag          // Zombis muertos (no procesar)
FChasingTag       // Zombis persiguiendo (para queries específicas)
FAttackingTag     // Zombis atacando (para queries específicas)
FInFrustumTag     // Zombis visibles en cámara
FHighPriorityTag  // Zombis que necesitan 60 FPS
```

#### **Fragmento LOD para Optimización:**
```cpp
struct FZombiLODFragment : public FMassFragment
{
    uint8 UpdateFrequency : 2;  // 0=60fps, 1=30fps, 2=15fps, 3=5fps
    uint8 VisualLOD : 2;        // 0=Full, 1=Reduced, 2=Simple, 3=Sprite
    uint8 bInFrustum : 1;       // Visible en cámara
    uint8 bNeedsUpdate : 1;     // Debe procesarse este frame
    uint8 PriorityLevel : 3;    // 0=Idle, 1=WalkAround, 2=Seek, 3=Chase, 4=TakeDamage, 5=Attack, 6=Dead
    float DistanceToPlayer;     // Distancia al jugador
    float LastUpdateTime;       // Último tiempo de update
    float StimulusIntensity;    // Intensidad del estímulo (daño, etc.)
};
```

### **Procesadores Optimizados:**

#### **UZombiLODProcessor (Nuevo):**
```cpp
// Procesador que maneja LOD inteligente y sincronización
- Calcular distancia al jugador
- Evaluar estado actual del BehaviorFragment
- Determinar intensidad de estímulos
- Calcular prioridad combinada: Estado + Distancia + Estímulos
- Sincronizar tags con estado actual
- Aplicar frustum culling
- Marcar entidades para procesamiento según prioridad
```

#### **UZombiBehaviorProcessor (Modificado):**
```cpp
// Procesador que maneja lógica de comportamiento
- Usar queries optimizadas con tags
- Procesar solo entidades activas (FActiveTag)
- Manejar transiciones de estado
- Sincronizar tags automáticamente al cambiar estado
- Lógica de IA y toma de decisiones
```

#### **UZombiMovementProcessor (Modificado):**
```cpp
// Procesador optimizado para movimiento
- Query para entidades activas y en frustum
- Procesar según frecuencia de update (LOD)
- Aplicar movimiento basado en estado
- Optimizar cálculos por prioridad
```

#### **UZombiTurboSequenceProcessor (Modificado):**
```cpp
// Procesador optimizado para render
- Query para entidades visibles (FInFrustumTag)
- Aplicar LOD visual según distancia
- Sincronizar transformaciones eficientemente
- Optimizar batch rendering
```

### **Lógica de Prioridad Inteligente**
```cpp
// Cálculo de prioridad combinada
PriorityScore = (StateWeight * StatePriority) + 
                (DistanceWeight * DistanceFactor) + 
                (StimulusWeight * StimulusIntensity)

// Estados (StatePriority)
- Attack: 100 (máxima prioridad - atacando activamente)
- TakeDamage: 95 (reacción inmediata - siendo atacado)
- Chase: 80 (persecución activa del jugador)
- Seek: 60 (buscando al jugador)
- WalkAround: 30 (movimiento básico)
- Idle: 10 (mínima prioridad - esperando)
- Dead: 0 (no procesar - entidad inactiva)

// Distancia (DistanceFactor)
- < 200u: 1.0 (máximo)
- 200-500u: 0.7
- 500-800u: 0.4
- > 800u: 0.1 (mínimo)

// Estímulos (StimulusIntensity)
- Daño directo: 100
- Sonidos fuertes: 50
- Estímulos visuales: 30
- Sin estímulos: 0
```

### **Sincronización Estados-Tags**
```cpp
// Método centralizado para cambios de estado
void SetZombieState(FMassEntityHandle Entity, EZombiState NewState)
{
    // 1. Cambiar estado en BehaviorFragment
    BehaviorFragment.CurrentState = NewState;
    
    // 2. Sincronizar tags automáticamente
    switch (NewState)
    {
        case EZombiState::Dead:
            EntityManager.AddTag<FDeadTag>(Entity);
            EntityManager.RemoveTag<FActiveTag>(Entity);
            break;
            
        case EZombiState::Chase:
            EntityManager.AddTag<FChasingTag>(Entity);
            EntityManager.AddTag<FHighPriorityTag>(Entity);
            break;
            
        case EZombiState::Attack:
            EntityManager.AddTag<FAttackingTag>(Entity);
            EntityManager.AddTag<FHighPriorityTag>(Entity);
            break;
            
        // ... otros estados
    }
}
```

### **Optimización de Queries**
```cpp
// Queries optimizadas por prioridad combinada
- QueryCritical: Zombis Attack, TakeDamage, o muy cercanos (60 FPS)
- QueryHigh: Zombis Chase, Seek, o con estímulos intensos (30 FPS)  
- QueryNormal: Zombis WalkAround, o con estímulos moderados (15 FPS)
- QueryLow: Zombis Idle lejanos sin estímulos (5 FPS)
```

### **Queries Específicas por Tags**
```cpp
// Query base para todos los procesadores
ActiveQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
ActiveQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

// Query para zombis persiguiendo
ChasingQuery.AddTagRequirement<FChasingTag>(EMassFragmentPresence::All);

// Query para zombis atacando
AttackingQuery.AddTagRequirement<FAttackingTag>(EMassFragmentPresence::All);

// Query para zombis visibles
VisibleQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All);

// Query para zombis de alta prioridad
HighPriorityQuery.AddTagRequirement<FHighPriorityTag>(EMassFragmentPresence::All);
```

## 📈 Proyección de Rendimiento

### **Escenario Actual (1000 zombis):**
- **CPU**: ~15-20ms por frame
- **GPU**: ~10-15ms por frame
- **Memoria**: ~50-100MB

### **Con Optimizaciones (1000 zombis):**
- **CPU**: ~3-5ms por frame (75% reducción)
- **GPU**: ~2-4ms por frame (80% reducción)  
- **Memoria**: ~25-50MB (50% reducción)

### **Escalabilidad Extendida:**
- **5000 zombis**: 60 FPS estable
- **10000 zombis**: 30 FPS estable
- **20000 zombis**: 15 FPS estable

## 🎮 Funcionalidades de Juego

### **Sistema de Daño Básico**
```cpp
// Implementación simple y eficiente
- Detección de colisiones con jugador
- Estados de vida/muerte
- Sistema de respawn con pooling
- Animaciones de ataque y muerte
```

### **Comportamiento de Horda**
```cpp
// Lógica grupal básica
- Agrupación por proximidad
- Comportamiento coordinado
- Difusión de información entre zombis
- Objetivos compartidos
```

### **Pathfinding Simple**
```cpp
// Navegación básica para isométrico
- Flow field navigation
- Evitación de obstáculos básica
- Rutas optimizadas para grupos
```

## 🔄 Plan de Implementación - Enfoque Híbrido

### **Sprint 1: Arquitectura Híbrida Base (2 semanas)**
1. **Implementar estados en BehaviorFragment** (Idle, WalkAround, Seek, Chase, TakeDamage, Attack, Dead)
2. **Crear sistema de tags** (FActiveTag, FDeadTag, FChasingTag, FAttackingTag, FInFrustumTag, FHighPriorityTag)
3. **Implementar sincronización estados-tags** (método centralizado SetZombieState)
4. **Crear FZombiLODFragment** para optimización

### **Sprint 2: LOD Inteligente y Culling (2 semanas)**
1. **Crear UZombiLODProcessor** con lógica de prioridad combinada
2. **Implementar frustum culling** con FInFrustumTag
3. **Optimizar queries** por tags específicos
4. **Integrar LOD por frecuencia** de update

### **Sprint 3: Procesadores Optimizados (2 semanas)**
1. **Modificar BehaviorProcessor** para usar queries con tags
2. **Optimizar MovementProcessor** con LOD y culling
3. **Mejorar TurboSequenceProcessor** para batch rendering
4. **Implementar entity pooling** para memoria

### **Sprint 4: Funcionalidad y Testing (2 semanas)**
1. **Sistema de daño básico** con estados TakeDamage/Attack
2. **Estados de muerte/respawn** con FDeadTag
3. **Comportamiento de horda simple**
4. **Testing de sincronización** estados-tags

## 📊 Métricas de Éxito

### **Rendimiento**
- [ ] **1000 zombis**: 60 FPS estable
- [ ] **5000 zombis**: 30 FPS estable
- [ ] **Memoria**: < 200MB para 1000 entidades
- [ ] **CPU**: < 8ms por frame para lógica

### **Funcionalidad**
- [ ] **Sistema de daño**: Colisiones y muerte funcionales
- [ ] **Comportamiento de horda**: Lógica grupal observable
- [ ] **Pathfinding**: Navegación básica funcional
- [ ] **Estados fluidos**: Transiciones suaves entre animaciones

## 🎯 Consideraciones Específicas para Isométrico

### **Ventajas del Isométrico:**
1. **Frustum Predictible**: Área de vista constante
2. **Distancia 2D**: Cálculos más simples
3. **Occlusion Limitada**: Menos objetos que se ocultan
4. **Grid Natural**: Particionado espacial natural

### **Optimizaciones Específicas:**
1. **Culling 2D**: Solo calcular X,Y para frustum
2. **Grid Cuadrada**: Particionado natural para isométrico
3. **LOD por Distancia 2D**: Más preciso que 3D
4. **Batch Rendering**: Agrupar por distancia

## 🚨 Reglas de Desarrollo

### **Principios Técnicos**
- **Mantener simplicidad**: No agregar complejidad innecesaria
- **Optimizar incrementalmente**: Mejorar lo existente antes de agregar
- **Enfocarse en rendimiento**: LOD y culling primero
- **Preservar State Sync**: No acoplar lógica con visual

### **Patrones de Diseño**
- **State Sync**: Separación lógica/visual
- **ECS**: Entidades, componentes, sistemas
- **LOD**: Diferentes niveles de detalle
- **Pooling**: Reutilización de objetos
- **Spatial Partitioning**: Optimización de queries

---

**🎯 Objetivo: Sistema de zombis masivos optimizado para juego isométrico, con enfoque en rendimiento y simplicidad.**
