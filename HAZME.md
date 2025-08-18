# MyProjectTSequence - Arquitectura DOP con Microprocessors

## 🎯 **OBJETIVO**
Implementar un sistema ECS escalable para 10,000+ zombies usando **Microprocessors especializados** y **Data Oriented Programming (DOP)**.

## 🧠 **FILOSOFÍA: UNA RESPONSABILIDAD POR PROCESSOR**

### **✅ IMPLEMENTADOS (Funcionales)**
```
📋 BehaviorProcessor     → Orquestador: decide CUÁNDO cambiar estados
🏃 ChaseProcessor        → Especializado: maneja CÓMO perseguir  
🚶 WalkAroundProcessor   → Especializado: maneja CÓMO caminar random
😴 IdleProcessor         → Especializado: maneja CÓMO estar inactivo
🧠 StimulusProcessor     → Especializado: maneja detección de estímulos
🚀 MovementProcessor     → Especializado: aplica física de movimiento
🎨 TurboSequenceProcessor → Especializado: maneja animaciones
⚡ TransformProcessor    → Especializado: valida/corrige transformaciones
```

### **🔄 PRÓXIMOS MICROPROCESSORS (Conceptuales)**
```
💔 TakeDamageProcessor   → Especializado: recibir y procesar daño
⚔️ DealDamageProcessor   → Especializado: causar y aplicar daño
💀 DeathProcessor        → Especializado: manejo de muerte
🩹 HealProcessor         → Especializado: curación y regeneración  
🎵 SoundProcessor        → Especializado: efectos sonoros
✨ ParticleProcessor     → Especializado: efectos visuales
```

## 📦 **FRAGMENTS DOP-OPTIMIZADOS**

### **✅ IMPLEMENTADOS**
- `FZombiStateFragment` - Estados y flags (16 bytes)
- `FZombiTransformFragment` - Posición/rotación (28 bytes) 
- `FZombiMovementFragment` - Dirección/velocidad (16 bytes)
- `FZombiStimuliFragment` - Estímulos externos (32 bytes)
- `FZombiTurboSequenceFragment` - Datos de animación
- `FZombiConfigFragment` - Configuración global
- `FZombiUpdateFrequencyFragment` - Optimización LOD

### **🔄 PRÓXIMOS (Para sistema de daño)**
```cpp
// CONCEPTO: Fragment de daño (DOP-optimized)
struct FZombiDamageFragment {
    uint16 PendingDamage = 0;     // Daño pendiente (0-65535)
    uint8 DamageType = 0;         // Tipo: físico, fuego, etc
    uint8 DamageSource = 0;       // Fuente: jugador, trampa, etc
    uint32 DamageTimestamp = 0;   // Cuándo ocurrió
    // Total: 12 bytes
};

// CONCEPTO: Fragment de ataque (DOP-optimized)
struct FZombiAttackFragment {
    uint16 AttackCooldown = 0;    // Cooldown restante
    uint8 AttackDamage = 50;      // Daño que causa
    uint8 AttackRange = 80;       // Rango de ataque
    uint32 LastAttackTime = 0;    // Último ataque
    // Total: 12 bytes
};
```

## 🔄 **FLUJO DE ORQUESTACIÓN**

### **COMPORTAMIENTO ACTUAL**
```
Frame N:
1. StimulusProcessor → Detecta estímulos
2. BehaviorProcessor → Decide transiciones (Idle ↔ WalkAround ↔ Chase)
3. ChaseProcessor → Calcula persecución (si aplicable)
4. WalkAroundProcessor → Calcula movimiento random (si aplicable)  
5. IdleProcessor → Maneja inactividad (si aplicable)
6. MovementProcessor → Aplica física final
7. TurboSequenceProcessor → Actualiza animaciones
```

### **FUTURO SISTEMA DE DAÑO**
```
Frame N+:
6. DealDamageProcessor → Detecta ataques, marca objetivos
7. TakeDamageProcessor → Procesa daño pendiente
8. DeathProcessor → Maneja muerte si HP <= 0
```

## 🚀 **VENTAJAS DOP CONSEGUIDAS**

### **✅ PERFORMANCE**
- **Queries ultra-específicos** por comportamiento
- **Cache locality perfecta** (datos contiguos)
- **Escalabilidad lineal** hasta 10,000+ entidades
- **Optimización por frecuencia** (Idle = 15 FPS, Chase = 60 FPS)

### **✅ DESARROLLO**
- **Una responsabilidad por processor** = fácil testing
- **Modular** = fácil agregar features
- **Aislado** = fácil debugging  
- **Predecible** = fácil optimización

## 🎯 **IMPLEMENTACIÓN ACTUAL**

### **ESTADOS SOPORTADOS**
- **Idle**: Se queda parado, rota ±30° cada 3-8s
- **WalkAround**: Camina random, cambia dirección cada 2-4s, velocidad 30-70
- **Chase**: Persigue jugador, velocidad adaptativa 150→60, para a 80 unidades

### **OPTIMIZACIONES DOP**
- **IdleProcessor**: 15 FPS (cada 4 frames) para entidades inactivas
- **Fragments compactos**: 16-32 bytes cada uno
- **Flags en lugar de enums**: operaciones bitwise ultra-rápidas
- **Cache-friendly**: datos relacionados juntos en memoria

## 📋 **PRÓXIMOS PASOS**

### **Fase 1: Consolidar Base** ✅ 
- [x] Arquitectura de microprocessors funcional
- [x] Estados básicos (Idle, WalkAround, Chase)
- [x] Performance DOP optimizada

### **Fase 2: Sistema de Daño (Conceptual)**
- [ ] Diseñar `FZombiDamageFragment` 
- [ ] Diseñar `FZombiAttackFragment`
- [ ] Implementar `TakeDamageProcessor` (concepto)
- [ ] Implementar `DealDamageProcessor` (concepto)

### **Fase 3: Sistemas Avanzados**
- [ ] DeathProcessor + respawn
- [ ] SoundProcessor + efectos
- [ ] ParticleProcessor + VFX
- [ ] HealProcessor + regeneración

### **Fase 4: LOD + Rate limiting + TurboSequence GT (Escalar a 10k)**

#### 4.1 LOD/Rate limiting real con `FZombiUpdateFrequencyFragment`
- [ ] `ZombiStimulusProcessor` (`Source/.../ZombiStimulusProcessor.*`):
  - [ ] Agregar requisito del fragment `FZombiUpdateFrequencyFragment` (RW) en el query.
  - [ ] Antes de procesar el chunk: obtener `CurrentTime = GetWorld()->GetTimeSeconds()` y `PlayerLocation` (vía `UStimulusSubsystem::TryGetLatestPlayerStimulus` o `UGameplayStatics`).
  - [ ] Por entidad: calcular `DistanceToPlayer`, llamar `UpdateFrequencyInfo(DistanceToPlayer, CurrentTime)` y saltar si `!ShouldUpdate(CurrentTime)`.
  - [ ] Bucketizar por frame: procesar solo si `(EntityBucket == CurrentBucket)`. Usar `BatchGroup` o `EntityIndex` para derivar bucket.
- [ ] `ZombiBehaviorProcessor` (`Source/.../ZombiBehaviorProcessor.*`): mismo gating por `FZombiUpdateFrequencyFragment` que arriba.
- [ ] `ZombiMovementProcessorOptimized` (`Source/.../ZombiMovementProcessorOptimized.*`):
  - [ ] Usar `FZombiUpdateFrequencyFragment` para decidir si aplicar movimiento según LOD.
  - [ ] Definir buckets por LOD (ej.: LOD0=1, LOD1=2, LOD2=4, LOD3=12 frames) y procesar 1 bucket por frame.
- [ ] `ZombiTurboSequenceProcessor` (`Source/.../ZombiTurboSequenceProcessor.*`):
  - [ ] Gating por `FZombiUpdateFrequencyFragment` (menos frecuencia para LOD altos) para anim/transform.

Sugerencia de bucketización (simple y cache-friendly):
```
BucketsPorLOD: LOD0=1, LOD1=2, LOD2=4, LOD3=12
BucketEntidad = (BatchGroup /*0..3*/ + EntityIndex) % BucketsPorLOD
BucketFrame = (FrameCounter) % BucketsPorLOD
Procesar si BucketEntidad == BucketFrame
```

#### 4.2 Reducir trabajo de TurboSequence en Game Thread
- [ ] `AZombiTestController::Tick` (`Source/.../ZombiTestController.*`):
  - [ ] Reemplazar el loop de 4 llamadas a `SolveMeshes_GameThread` por una sola llamada por frame.
  - [ ] Eliminar `try/catch(...)` del tick (innecesario en UE y costoso).
- [ ] `ZombiTurboSequenceProcessor`:
  - [ ] Añadir dirty flags en `FZombiTurboSequenceFragment` (p. ej., `bTransformDirty`, `bAnimDirty`).
  - [ ] Solo llamar `SetMeshWorldSpaceTransform_Concurrent` si `bTransformDirty` (o si la delta de posición/yaw supera umbral).
  - [ ] Solo llamar `PlayAnimation_Concurrent` cuando cambia `TargetAnimation` o al ritmo del LOD (p. ej., LOD2 cada 2-3 frames, LOD3 cada 10-12 frames).
  - [ ] Resetear flags tras aplicar los cambios.

Heurísticas recomendadas:
- Umbral transform: `|Δpos| > 0.5f` o `|Δyaw| > 1°` para marcar dirty.
- Ritmo anim por LOD: LOD0=60 Hz, LOD1=30 Hz, LOD2≈15-20 Hz, LOD3≈5-10 Hz.

#### 4.3 Micro-afinados de hot paths
- [ ] `ZombiMovementProcessorOptimized`:
  - [ ] Evitar trigonometría: usar siempre `MovementFragment.Direction` para trasladar y orientar, en lugar de `TransformFragment.GetForwardVector()` y conversiones rot→vector cada frame.
  - [ ] Al rotar hacia la dirección, usar `FInterpTo` sobre yaw con la yaw derivada de `MovementFragment.Direction` (sin reconstruir rot completa).
- [ ] `ZombiWalkAroundProcessor` y `ZombiBehaviorProcessor`:
  - [ ] Congelar generación de random a ventanas discretas (timers) y ampliar intervalos para LOD altos.
  - [ ] Evitar múltiples llamadas a RNG por entidad y frame.
- [ ] `AZombiTestController::Tick`:
  - [ ] Quitar `try/catch(...)`.
  - [ ] Reducir logs en hot paths o ponerlos tras macros/temporalizadores.

#### 4.4 Pseudocódigo de gating por LOD (por entidad)
```cpp
// Dentro de Execute(), por entidad
const float currentTime = GetWorld()->GetTimeSeconds();
const float distance = FVector::Dist(Transform.Position, CachedPlayerPos);
UpdateFrequencyFragment.UpdateFrequencyInfo(distance, currentTime);

// Bucket por LOD
const uint8 buckets = GetBucketsForLOD(UpdateFrequencyFragment.Priority); // 1,2,4,12
const uint32 entityBucket = (Movement.BatchGroup + EntityIndex) % buckets;
const uint32 frameBucket = (GFrameCounter /*o similar*/) % buckets;
if (!UpdateFrequencyFragment.ShouldUpdate(currentTime) || entityBucket != frameBucket) {
    return; // saltar entidad
}
```

Checklist de verificación (performance):
- [ ] Estímulos: sin crecimiento de listas, coste ≈ O(n + m), cache player OK.
- [ ] LOD aplicado en Stimulus/Behavior/Movement/TS, con buckets.
- [ ] TS: una llamada `SolveMeshes_GameThread` por frame; diffs/dirty flags activos.
- [ ] Hot paths sin trig ni RNG per-frame; logs minimizados.

## 🏗️ **ARQUITECTURA FINAL**

```
ORQUESTADOR:
📋 BehaviorProcessor → Decide cuándo cambiar estados

MICROPROCESSORS ESPECIALIZADOS:
🏃 ChaseProcessor → Cómo perseguir
🚶 WalkAroundProcessor → Cómo caminar  
😴 IdleProcessor → Cómo estar inactivo
💔 TakeDamageProcessor → Cómo recibir daño
⚔️ DealDamageProcessor → Cómo causar daño
🚀 MovementProcessor → Cómo aplicar física

INFRAESTRUCTURA:
🧠 StimulusProcessor → Detección externa
🎨 TurboSequenceProcessor → Animaciones  
⚡ TransformProcessor → Validación
```

**PRINCIPIO CLAVE: Una feature = Un processor = Una responsabilidad**

---

*Última actualización: Sistema de microprocessors DOP implementado y funcional*

Hallazgos clave (concretos en tu código)
Estímulos
UStimulusSubsystem no “tickea”: declaras Tick(float) pero la clase no implementa FTickableGameObject ni registra un ticker. Resultado: no hay decaimiento ni limpieza periódica y la lista ActiveStimuli puede crecer, ampliando el coste O(n·m) en UZombiStimulusProcessor::ProcessStimuliForZombie(...).
UZombiStimulusProcessor itera todos los CachedActiveStimuli para cada zombie: O(numZombis × numStimuli). Con 10k zombis es prohibitivo si la lista no se “poda” y no hay partición espacial.
TurboSequence
AZombiTestController::Tick llama ATurboSequence_Manager_Lf::SolveMeshes_GameThread(...) 4 veces por frame (grupos 0..3), siempre en Game Thread. Esto escala mal con 10k instancias.
UZombiTurboSequenceProcessor hace PlayAnimation_Concurrent/SetMeshWorldSpaceTransform_Concurrent por entidad. Aunque es concurrente, sigues enviando órdenes cada frame incluso cuando no hay cambios de anim/transform. No hay gating por LOD/frecuencia actual.
LOD/Rate limiting
Tienes FZombiUpdateFrequencyFragment con lógica de LOD/Hz, pero ningún processor lo usa para saltarse entidades por frame. Los processors corren a frecuencia completa para todos los activos.
Costes innecesarios por frame
Trigonometría redundante: en caminar usas TransformFragment.GetForwardVector() (convierte yaw → rot → vector) en lugar de usar MovementFragment.Direction ya normalizado.
Aleatoriedad por entidad en bucles calientes (p. ej., ZombiWalkAroundProcessor/BehaviorProcessor) — aun con algunos timers, el random se evalúa con frecuencia.
try/catch(...) en AZombiTestController::Tick (UE normalmente compila sin excepciones); además añade overhead superfluo.
Orden de ejecución
Mezcla de ExecuteInGroup con ExecuteBefore/After usando nombres de grupo personalizados (“MassBehavior”, “MassMovement”, “MassTransform”). Si no existen como grupos reales, el orden puede no respetarse. Asegura grupos soportados o alinea todo en un único esquema.
Datos
Buen uso de fragments compactos y flags (uint8, uint16). Ojo: FZombiTransformFragment::Yaw guarda “grados” directamente en uint16 y GetYaw() devuelve 0..65535. Eso funciona por wrap, pero convendría cuantizar 0..360 a 0..65535 (y des-cuántizar al leer) para evitar rotaciones “gigantes” y trabajo extra al normalizar.
Qué cambiar primero (priorizado para escalar a ~10k)
1) Arreglar el tick y complejidad de estímulos
Haz que UStimulusSubsystem realmente tickee (hereda FTickableGameObject o registra FTSTicker) y mueve ahí decaimiento/limpieza; o trasládalo a UZombiStimulusProcessor para que limpie en batch cada StimulusUpdateInterval.
Cambia O(n·m) a O(n + m) aproximado:
Para “señal de jugador” usa una única “fuente” por frame: calcula dirección/distancia por chunk (posición media del chunk) y propaga a las entidades del chunk, evitando comparar con todos los estímulos.
Para estímulos múltiples, usa partición espacial (grid uniforme simple) de estímulos; consulta solo celdas vecinas por zombie.
2) Aplicar LOD y rate limiting reales en todos los processors y en TurboSequence
Usa FZombiUpdateFrequencyFragment para decidir si una entidad se procesa este frame. Regla simple: LOD0=60Hz, LOD1=30Hz, LOD2=15Hz, LOD3=5Hz — ya está codificado en el fragment.
Stagger: reparte entidades por buckets para cada LOD y procesa solo un bucket por frame. Esto elimina 60–90% de trabajo en lejanos 1.
3) Reducir el trabajo de TurboSequence en Game Thread
Evita llamar SolveMeshes_GameThread cuatro veces por frame. Llama una sola vez o usa la API recomendada para programar la actualización por grupo sin bloquear el GT.
No llames PlayAnimation_Concurrent/SetMeshWorldSpaceTransform_Concurrent si no hay cambios. Mantén “dirty flags” por entidad (posición/rot/anim) y empuja solo diffs. Baja la frecuencia de anim en LOD altos (p. ej., actualiza anim cada N frames).
Si TS expone API de batch para transformar/animar muchas instancias a la vez, úsala.
4) Ahorrar CPU en los bucles calientes
En caminar, usa siempre MovementFragment.Direction (evita GetForwardVector()/trig).
Congela aleatoriedad y cambios de dirección a timers discretos por entidad (ya vas por buen camino, pero endurece los intervalos para LOD altos).
Elimina try/catch(...) del tick y cualquier log residual en hot-path (deja Verbose desactivado o detrás de macros).
5) Spawning y memoria
Continúa con lotes, pero amortiza creación visual: distribuye la creación de instancias visuales en varios frames y/o usa llamadas bulk si existen.
Preconstruye arquetipos Mass (mismo set de fragments) y usa creación batch si es posible en tu versión de UE5.5.
Estimación de impacto
LOD + buckets + gating de processors: 3–5×.
TS: una sola pasada en GT + diffs + gating anim: 2–3×.
Estímulos con partición espacial y tick real: 2–10× (depende de m).
Ahorros en trig/aleatoriedad/logs: 1.3–1.8×.
Combinado, es realista pasar de ~500 a varios miles y acercarte a 10k en PC con TS, si repartes trabajo fuera del GT y eliminas el O(n·m) en estímulos.
Opinión sobre el enfoque
Lo bueno: arquitectura modular y “state sync” clara, fragments compactos, flags, queries por chunk, separación lógica/visual, grupos TS previstos. Esto se alinea perfecto con tu objetivo de escalado y con las micro-optimizaciones planificadas (rates LOD, agrupado, culling, batch/pooling) 1.
Lo a mejorar: faltan activar los “cortes de coste” ya diseñados (LOD/rate limit, batching TS, spatial hashing estímulos) y hay trabajo excesivo en GT. Una vez actives esas palancas, tu diseño actual debería escalar.
Si quieres, en el siguiente paso te preparo diffs puntuales para:
Hacer UStimulusSubsystem tickeable y limpiar estímulos.
Introducir gating por FZombiUpdateFrequencyFragment en Stimulus/Behavior/Movement/TurboSequence.
Cambiar caminar a MovementFragment.Direction y evitar trigonometría.
Evitar PlayAnimation/Transform por frame sin cambios.
Agregaría un grid simple para estímulos y un “chunk-level direction” del jugador para recortar trabajo al mínimo.
En AZombiTestController, dejar una sola llamada adecuada al solver TS y quitar try/catch(...).
Opcional: corrección de cuantización de yaw en FZombiTransformFragment para mantener la rotación robusta.
¿Quieres que lo implemente ahora sobre tus archivos existentes? Puedo hacerlo en pequeños bloques para que puedas probar el impacto incremental.
Se detectaron estos puntos críticos:
UStimulusSubsystem no está tickeando; la limpieza y decaimiento no ocurren.
UZombiStimulusProcessor tiene coste O(n·m); falta partición espacial.
FZombiUpdateFrequencyFragment no se usa para saltar entidades por frame.
AZombiTestController invoca SolveMeshes_GameThread 4 veces por frame; mucho trabajo en Game Thread.
UZombiTurboSequenceProcessor empuja anim/transform cada frame; faltan “dirty flags” y rate limiting.
Trig y random se usan en hot paths; sustituir por datos ya cacheados y timers discretos.