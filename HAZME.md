# 🎯 Roadmap de Desarrollo - Sistema ECS Zombis

## 📊 Estado Actual del Sistema

### **✅ Lo que ya funciona:**
- **4 procesadores estables**: Behavior, Movement, TurboSequence, LOD
- **State Sync**: Separación lógica/visual con TurboSequence
- **Batch processing**: Procesamiento en chunks optimizado
- **Fragmentos especializados**: Solo datos necesarios por procesador
- **Sistema de comportamiento básico**: Seek y Chase funcionales
- **Arquitectura limpia**: Tags por frecuencia + Estados para TurboSequence

### **❌ Limitaciones actuales:**
- **Sin LOD dinámico**: No implementado tags por frecuencia
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
- **Sistema de LOD**: Tags por frecuencia implementados
- **Culling básico**: Solo renderizar zombis visibles

### **Fase 3: Funcionalidad (2-3 semanas)**
- **Sistema de daño**: Colisiones y muerte
- **Comportamiento de horda**: Lógica grupal básica
- **Pathfinding simple**: Navegación básica

## 🚀 Prioridad Crítica - Optimizaciones de Rendimiento

### **1. Tags por Frecuencia (ALTA)**
```cpp
// LOD dinámico por estado
- CRÍTICO (60 FPS): Zombis TakeDamage, Attack (FUpdate60FPS)
- ALTO (30 FPS): Zombis Chase (FUpdate30FPS)
- NORMAL (15 FPS): Zombis Seek, WalkAround (FUpdate15FPS)
- BAJO (5 FPS): Zombis Idle, Dead (FUpdate5FPS)
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

## 🔧 Implementación Técnica - Tags por Frecuencia

### **Arquitectura Optimizada: Tags por Frecuencia + Estados para TurboSequence**



#### **Tags por Frecuencia (LOD Dinámico):**
```cpp
// Tags para LOD y optimización
FActiveTag          // Entidades activas (base para queries)
FDeadTag            // Entidades muertas (excluir de procesamiento)
FInFrustumTag       // Visible en cámara (frustum culling)
FUpdate60FPS        // Frecuencia crítica (TakeDamage, Attack)
FUpdate30FPS        // Frecuencia alta (Chase)
FUpdate15FPS        // Frecuencia normal (Seek, WalkAround)
FUpdate5FPS         // Frecuencia mínima (Idle, Dead)
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

// Solo en BehaviorFragment para animaciones
struct FZombiBehaviorFragment : public FMassFragment
{
    EZombiState CurrentState;    // Estado solo para animaciones
    // ... otros datos de comportamiento
};
```

#### **Fragmento LOD Simplificado (Datos Estables):**
```cpp
struct FZombiLODFragment : public FMassFragment
{
    uint8 VisualLOD : 2;        // 0=Full, 1=Reduced, 2=Simple, 3=Sprite
    uint8 PriorityLevel : 3;    // 0=Idle, 1=WalkAround, 2=Seek, 3=Chase, 4=TakeDamage, 5=Attack, 6=Dead
    uint8 Padding : 3;          // Alineación
    
    float DistanceToPlayer;     // Distancia al jugador (necesario para cálculos)
};
```

### **Procesadores Optimizados:**

#### **UZombiLODProcessor (Modificado):**
```cpp
// Procesador que maneja sincronización Estado → Tags de Frecuencia
- Calcular distancia al jugador
- Evaluar estado actual del BehaviorFragment
- Sincronizar estado → tags de frecuencia
- Aplicar frustum culling
- Marcar entidades para procesamiento según frecuencia
```

#### **UZombiBehaviorProcessor (Modificado):**
```cpp
// Procesador que maneja lógica de comportamiento por orden de prioridad
- QueryUpdate60FPS: Procesar primero (super crítico, TakeDamage, Attack)
- QueryUpdate30FPS: Procesar segundo (alta prioridad, Chase)
- QueryUpdate15FPS: Procesar tercero (normal, Seek, WalkAround)
- QueryUpdate5FPS: Procesar último (mínima, Idle, Dead)
- TakeDamage interrumpe cualquier estado (limpiar todos los tags)
- Sincronizar estado automáticamente
```

#### **UZombiMovementProcessor (Modificado):**
```cpp
// Procesador optimizado para movimiento
- ActiveVisibleQuery: Solo entidades activas y visibles (FActiveTag + FInFrustumTag)
- Procesar según frecuencia de update (tags de frecuencia)
- Aplicar movimiento basado en estado del BehaviorFragment
- Optimizar cálculos por prioridad
```

#### **UZombiTurboSequenceProcessor (Modificado):**
```cpp
// Procesador optimizado para render
- Update60FPSQuery: Siempre renderizar, máxima calidad (ignorar frustum)
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

// Frecuencias de Update por Estado
- TakeDamage: 60 FPS (FUpdate60FPS)
- Attack: 60 FPS (FUpdate60FPS)
- Chase: 30 FPS (FUpdate30FPS)
- Seek: 15 FPS (FUpdate15FPS)
- WalkAround: 15 FPS (FUpdate15FPS)
- Idle: 5 FPS (FUpdate5FPS)
- Dead: 5 FPS (FUpdate5FPS)

// Regla de Interrupción
- TakeDamage interrumpe cualquier estado
- Limpia todos los tags de frecuencia
- Fuerza FUpdate60FPS para 60 FPS
```

### **Sincronización Estado → Tags de Frecuencia**
```cpp
// Flujo de sincronización optimizado
// 1. BehaviorProcessor: Cambia estado según lógica
if (DistanceToPlayer < 200.0f) {
    BehaviorFragment.CurrentState = EZombiState::Chase;
}

// 2. LODProcessor: Sincroniza estado → tags de frecuencia
switch (BehaviorFragment.CurrentState)
{
case EZombiState::TakeDamage:
case EZombiState::Attack:
    EntityManager.AddTagToEntity(Entity, FUpdate60FPS::StaticStruct());
    break;
case EZombiState::Chase:
    EntityManager.AddTagToEntity(Entity, FUpdate30FPS::StaticStruct());
    break;
case EZombiState::Seek:
case EZombiState::WalkAround:
    EntityManager.AddTagToEntity(Entity, FUpdate15FPS::StaticStruct());
    break;
case EZombiState::Idle:
case EZombiState::Dead:
    EntityManager.AddTagToEntity(Entity, FUpdate5FPS::StaticStruct());
    break;
}

// 3. TurboSequenceProcessor: Lee estado para animaciones
switch (BehaviorFragment.CurrentState) {
    case EZombiState::TakeDamage: PlayAnimation("MM_Fall_Loop"); break;
    case EZombiState::Chase: PlayAnimation("MM_Run_Fwd"); break;
    case EZombiState::Idle: PlayAnimation("MM_Idle"); break;
}
```

### **Optimización de Queries por Frecuencia**
```cpp
// Jerarquía de queries por frecuencia de procesamiento
- QueryUpdate60FPS: Zombis críticos (60 FPS - TakeDamage, Attack)
- QueryUpdate30FPS: Zombis alta prioridad (30 FPS - Chase)
- QueryUpdate15FPS: Zombis normal (15 FPS - Seek, WalkAround)
- QueryUpdate5FPS: Zombis mínima prioridad (5 FPS - Idle, Dead)
```

### **Queries Específicas por Tags**
```cpp
// Query base para todos los procesadores
ActiveQuery.AddTagRequirement<FActiveTag>(EMassFragmentPresence::All);
ActiveQuery.AddTagRequirement<FDeadTag>(EMassFragmentPresence::None);

// Queries por frecuencia (BehaviorProcessor)
Update60FPSQuery.AddTagRequirement<FUpdate60FPS>(EMassFragmentPresence::All); // Super crítico
Update30FPSQuery.AddTagRequirement<FUpdate30FPS>(EMassFragmentPresence::All); // Alta prioridad
Update15FPSQuery.AddTagRequirement<FUpdate15FPS>(EMassFragmentPresence::All); // Normal
Update5FPSQuery.AddTagRequirement<FUpdate5FPS>(EMassFragmentPresence::All);   // Mínima

// Queries para optimización
VisibleQuery.AddTagRequirement<FInFrustumTag>(EMassFragmentPresence::All);
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

## 🔄 Plan de Implementación - Tags por Frecuencia

### **Fase 1: Limpiar y Actualizar Fragmentos ✅ COMPLETADA**
1. **✅ Simplificar FZombiBehaviorFragment** - Eliminada complejidad innecesaria
2. **✅ Actualizar estados** - Agregados Seek, TakeDamage, Attack, Idle
3. **✅ Limpiar tags innecesarios** - Eliminados tags de comportamiento
4. **✅ Simplificar FZombiLODFragment** - Solo datos estables

### **Fase 2: Implementar Tags por Frecuencia (EN PROGRESO)**
1. **🔄 Crear tags por frecuencia** - FUpdate60FPS, FUpdate30FPS, FUpdate15FPS, FUpdate5FPS
2. **🔄 Modificar UZombiLODProcessor** - Sincronización Estado → Tags de Frecuencia
3. **🔄 Implementar queries por frecuencia** - Orden de prioridad en BehaviorProcessor
4. **🔄 Actualizar todos los procesadores** - Usar queries por frecuencia

### **Fase 3: Optimizar Procesadores (PENDIENTE)**
1. **Sistema de daño básico** - Estados TakeDamage/Attack con prioridad máxima
2. **Lógica de interrupción TakeDamage** - Limpiar tags y forzar FUpdate60FPS
3. **Estados de muerte/respawn** - Con FDeadTag
4. **Optimización final** - Entity pooling

### **Fase 4: Funcionalidad y Testing (PENDIENTE)**
1. **Testing de sincronización** - Estado → Tags de Frecuencia
2. **Optimización de rendimiento** - Medir impacto de LOD dinámico
3. **Comportamiento de horda** - Lógica grupal básica
4. **Pathfinding simple** - Navegación básica

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

### **✅ Completado (Fase 1):**
- **Arquitectura limpia**: Tags por frecuencia + Estados implementados
- **Fragmentos optimizados**: BehaviorFragment simplificado, LODFragment simplificado
- **Tags limpios**: Eliminados tags de comportamiento, preparados para tags por frecuencia
- **Sistema estable**: Sin crashes, comportamiento funcional
- **Comportamiento realista**: Seek y Chase implementados correctamente

### **✅ Completado (Fase 2):**
- **Tags por frecuencia**: FUpdate60FPS, FUpdate30FPS, FUpdate15FPS, FUpdate5FPS ✅
- **Sincronización Estado → Tags**: LODProcessor modificado ✅
- **Queries por frecuencia**: BehaviorProcessor optimizado ✅
- **Orden de ejecución**: LODProcessor configurado para ejecutarse PRIMERO ✅
- **Patrón Command**: Comandos diferidos implementados ✅
- **Integración completa**: Todos los procesadores actualizados ✅

### **🔄 En Progreso (Fase 3):**
1. **Sistema de daño básico** - Estados TakeDamage/Attack con FUpdate60FPS
2. **Lógica de interrupción TakeDamage** - Limpiar tags y forzar frecuencia crítica
3. **Estados de muerte/respawn** - Con FDeadTag
4. **Optimización final** - Entity pooling

### **📋 Próximas Tareas (Fase 4):**
1. **Testing de rendimiento** - Medir impacto de LOD dinámico
2. **Comportamiento de horda** - Lógica grupal básica
3. **Pathfinding simple** - Navegación básica

### **🎯 Beneficios Obtenidos:**
- **Código más limpio**: Fragmentos simplificados y organizados
- **Arquitectura clara**: Tags por frecuencia + Estados para TurboSequence
- **Escalabilidad**: LOD dinámico implementado y funcional
- **Mantenibilidad**: Separación clara entre LOD y comportamiento
- **Sistema estable**: Sin crashes, comportamiento funcional
- **Comportamiento realista**: Seek y Chase implementados correctamente
- **Orden de ejecución optimizado**: LODProcessor ejecuta PRIMERO
- **Patrón Command**: Sin errores de modificación de arrays durante iteración
- **Tags por frecuencia**: Implementados y funcionando
- **Queries optimizadas**: Por orden de prioridad (60FPS → 30FPS → 15FPS → 5FPS)

---

**🎯 Objetivo: Sistema de zombis masivos optimizado para juego isométrico, con enfoque en rendimiento y simplicidad usando Tags por Frecuencia.**
