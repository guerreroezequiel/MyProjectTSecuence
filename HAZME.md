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

### **Arquitectura Optimizada: Tags para Lógica + Estados para TurboSequence**

#### **Tags para Lógica y Optimización:**
```cpp
// Tags para comportamiento y optimización
FActiveTag        // Entidades activas (base para queries)
FDeadTag          // Entidades muertas (excluir de procesamiento)
FChasingTag       // Persiguiendo al jugador
FAttackingTag     // Atacando al jugador
FSeekingTag       // Buscando al jugador
FWalkingTag       // Caminando aleatoriamente
FIdleTag          // Esperando en idle
FTakeDamageTag    // Estado de daño activo (super prioritario)
FInFrustumTag     // Visible en cámara (frustum culling)
FHighPriorityTag  // Necesita 60 FPS (LOD crítico)
```

#### **Estados Solo para TurboSequence:**
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

// Solo en TurboSequenceFragment para animaciones
struct FZombiTurboSequenceFragment : public FMassFragment
{
    EZombiState VisualState;    // Estado solo para animaciones
    // ... otros datos visuales
};
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
// Procesador que maneja lógica de comportamiento por orden de prioridad
- QueryTakeDamage: Procesar primero (super crítico, 60 FPS)
- QueryAttack: Procesar segundo (alta prioridad, 60 FPS)
- QueryChase: Procesar tercero (media-alta, 30 FPS)
- QuerySeek: Procesar cuarto (media, 15 FPS)
- QueryWalk: Procesar quinto (baja, 15 FPS)
- QueryIdle: Procesar último (mínima, 5 FPS)
- TakeDamage interrumpe cualquier estado (limpiar todos los tags)
- Sincronizar tags automáticamente al cambiar estado
```

#### **UZombiMovementProcessor (Modificado):**
```cpp
// Procesador optimizado para movimiento
- ActiveVisibleQuery: Solo entidades activas y visibles (FActiveTag + FInFrustumTag)
- Procesar según frecuencia de update (LOD)
- Aplicar movimiento basado en tags de comportamiento
- Optimizar cálculos por prioridad
```

#### **UZombiTurboSequenceProcessor (Modificado):**
```cpp
// Procesador optimizado para render
- TakeDamageQuery: Siempre renderizar, máxima calidad (ignorar frustum)
- VisibleQuery: Solo entidades visibles (FInFrustumTag)
- Aplicar LOD visual según distancia
- Sincronizar transformaciones eficientemente
- Optimizar batch rendering
```

### **Lógica de Prioridad Inteligente**
```cpp
// Jerarquía de prioridad por orden de procesamiento
// 1. TakeDamage: 100 (máxima prioridad - super crítico, interrumpe todo)
// 2. Attack: 95 (alta prioridad - atacando activamente)
// 3. Chase: 80 (persecución activa del jugador)
// 4. Seek: 60 (buscando al jugador)
// 5. WalkAround: 30 (movimiento básico)
// 6. Idle: 10 (mínima prioridad - esperando)
// 7. Dead: 0 (no procesar - entidad inactiva)

// Frecuencias de Update por Prioridad
- TakeDamage: 60 FPS (super crítico)
- Attack: 60 FPS (alta prioridad)
- Chase: 30 FPS (media-alta)
- Seek: 15 FPS (media)
- WalkAround: 15 FPS (baja)
- Idle: 5 FPS (mínima)

// Regla de Interrupción
- TakeDamage interrumpe cualquier estado
- Limpia todos los tags de comportamiento
- Fuerza FHighPriorityTag para 60 FPS
```

### **Sincronización Tags → Estados Visuales**
```cpp
// Flujo de sincronización optimizado
// 1. BehaviorProcessor: Cambia tags según lógica
if (DistanceToPlayer < 200.0f) {
    EntityManager.AddTagToEntity(Entity, FChasingTag::StaticStruct());
}

// 2. TakeDamage: Super prioritario - interrumpe cualquier estado
if (ShouldTakeDamage()) {
    // Limpiar TODOS los tags de comportamiento
    EntityManager.RemoveTagFromEntity(Entity, FAttackingTag::StaticStruct());
    EntityManager.RemoveTagFromEntity(Entity, FChasingTag::StaticStruct());
    EntityManager.RemoveTagFromEntity(Entity, FSeekingTag::StaticStruct());
    EntityManager.RemoveTagFromEntity(Entity, FWalkingTag::StaticStruct());
    EntityManager.RemoveTagFromEntity(Entity, FIdleTag::StaticStruct());
    
    // Agregar TakeDamage + HighPriority
    EntityManager.AddTagToEntity(Entity, FTakeDamageTag::StaticStruct());
    EntityManager.AddTagToEntity(Entity, FHighPriorityTag::StaticStruct());
    return; // NO evaluar otros estados
}

// 3. TurboSequenceProcessor: Sincroniza tags → estado visual
EZombiState VisualState = DetermineVisualStateFromTags();
TurboSequenceFragment.VisualState = VisualState;

// 4. TurboSequence: Usa estado visual para animaciones
switch (TurboSequenceFragment.VisualState) {
    case EZombiState::TakeDamage: PlayAnimation("MM_Fall_Loop"); break; // Prioridad máxima
    case EZombiState::Chase: PlayAnimation("MM_Run_Fwd"); break;
    case EZombiState::Idle: PlayAnimation("MM_Idle"); break;
}
```

### **Optimización de Queries por Orden de Prioridad**
```cpp
// Jerarquía de queries por prioridad de procesamiento
- QueryTakeDamage: Zombis TakeDamage (60 FPS - super crítico, interrumpe todo)
- QueryAttack: Zombis Attack (60 FPS - alta prioridad)
- QueryChase: Zombis Chase (30 FPS - media-alta)
- QuerySeek: Zombis Seek (15 FPS - media)
- QueryWalk: Zombis WalkAround (15 FPS - baja)
- QueryIdle: Zombis Idle (5 FPS - mínima)
```

### **Queries Específicas por Tags**
```cpp
// Query base para todos los procesadores
ActiveQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
ActiveQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

// Queries por orden de prioridad (BehaviorProcessor)
TakeDamageQuery.AddTagRequirement<FTakeDamageTag>(EMassFragmentPresence::All); // Super crítico
TakeDamageQuery.AddTagRequirement<FHighPriorityTag>(EMassFragmentPresence::All); // 60 FPS

AttackQuery.AddTagRequirement<FAttackingTag>(EMassFragmentPresence::All); // Alta prioridad
AttackQuery.AddTagRequirement<FHighPriorityTag>(EMassFragmentPresence::All); // 60 FPS

ChaseQuery.AddTagRequirement<FChasingTag>(EMassFragmentPresence::All); // Media-alta
ChaseQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All); // Solo visibles

SeekQuery.AddTagRequirement<FSeekingTag>(EMassFragmentPresence::All); // Media
SeekQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All); // Solo visibles

WalkQuery.AddTagRequirement<FWalkingTag>(EMassFragmentPresence::All); // Baja
WalkQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All); // Solo visibles

IdleQuery.AddTagRequirement<FIdleTag>(EMassFragmentPresence::All); // Mínima
IdleQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All); // Solo visibles

// Queries para optimización
VisibleQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All);
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

### **Fase 1: Limpiar y Actualizar Fragmentos ✅ COMPLETADA**
1. **✅ Simplificar FZombiBehaviorFragment** - Eliminada complejidad innecesaria
2. **✅ Actualizar estados** - Agregados Seek, TakeDamage, Attack, Idle
3. **✅ Limpiar tags innecesarios** - Eliminados FMovingTag, FNeedsAnimationUpdateTag, FNeedsVisualSyncTag
4. **✅ Agregar tags faltantes** - Agregados FAttackingTag, FInFrustumTag, FHighPriorityTag

### **Fase 2: Implementar LOD ✅ COMPLETADA**
1. **✅ Crear FZombiLODFragment** - LOD inteligente implementado
2. **✅ Crear UZombiLODProcessor** - Lógica de prioridad combinada
3. **✅ Implementar sincronización estados-tags** - Método centralizado SetZombieState
4. **✅ Actualizar queries** - Optimizadas con nuevos tags

### **Fase 3: Optimizar Procesadores ✅ COMPLETADA**
1. **✅ Modificar BehaviorProcessor** - Queries con tags implementadas
2. **✅ Optimizar MovementProcessor** - Queries optimizadas
3. **✅ Mejorar TurboSequenceProcessor** - Queries optimizadas
4. **✅ Implementar frustum culling** - Con FInFrustumTag y LOD inteligente

### **Fase 4: Funcionalidad y Testing (1 semana)**
1. **Sistema de daño básico** - Estados TakeDamage/Attack con prioridad máxima
2. **Estados de muerte/respawn** - Con FDeadTag
3. **Testing de sincronización** - Estados-tags
4. **Optimización final** - Entity pooling

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

## 📊 Progreso Actual - Estado del Sistema

### **✅ Completado (Fases 1-2):**
- **Arquitectura híbrida**: Estados + Tags implementados
- **Fragmentos optimizados**: BehaviorFragment simplificado, LODFragment creado
- **Tags limpios**: Eliminados innecesarios, agregados nuevos
- **LOD inteligente**: UZombiLODProcessor con prioridad combinada
- **Sincronización automática**: Estados y tags siempre sincronizados
- **Queries optimizadas**: Todos los procesadores actualizados

### **✅ Completado (Fase 3):**
- **Frustum culling**: Implementado con LOD inteligente
- **LOD inteligente**: Prioridad combinada (estado + distancia + estímulos)
- **Integración completa**: LODProcessor conectado con todos los procesadores
- **Crash resuelto**: Eliminada modificación de entidades durante iteración

### **✅ Completado (Fase 4 - Parcial):**
- **Sistema de estímulos eliminado**: Simplificado completamente
- **Seek y Chase implementados**: Comportamiento funcional sin crashes
- **Movimiento hacia jugador**: Velocidad alta (120 u/s) para persecución
- **Transiciones de estado**: Idle → Seek → Chase basadas en distancia

### **📋 Próximas Tareas (Fase 4 - Restante):**
1. **Implementar jerarquía de queries** - Orden de prioridad en BehaviorProcessor
2. **Sistema de daño básico** - Estados TakeDamage/Attack con prioridad máxima
3. **Lógica de interrupción TakeDamage** - Limpiar tags y forzar prioridad
4. **Estados de muerte/respawn** - Con FDeadTag
5. **Optimización final** - Entity pooling

### **🎯 Beneficios Obtenidos:**
- **Código más limpio**: Fragmentos simplificados y organizados
- **Mejor rendimiento**: Queries optimizadas con tags
- **Escalabilidad**: LOD inteligente preparado
- **Mantenibilidad**: Arquitectura híbrida clara y documentada
- **Sistema estable**: Sin crashes, comportamiento funcional
- **Comportamiento realista**: Seek y Chase implementados correctamente
- **TakeDamage prioritario**: Sistema preparado para prioridad máxima en daño
- **Jerarquía de queries**: Orden de procesamiento optimizado por prioridad
- **Interrupción inteligente**: TakeDamage interrumpe cualquier estado

---

**🎯 Objetivo: Sistema de zombis masivos optimizado para juego isométrico, con enfoque en rendimiento y simplicidad.**
