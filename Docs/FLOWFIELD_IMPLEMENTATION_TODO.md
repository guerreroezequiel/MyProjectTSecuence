# TODO: Implementación del Sistema FlowField para Múltiples Entidades

## 🚀 MVP Inicial (Movimiento Básico)

### 1. Configuración Mínima
- [ ] Inicialización básica del FlowField
  - [ ] Un solo tile (64x64 celdas)
  - [ ] Meta fija en el centro
  - [ ] Cálculo de distancias básico

### 2. Movimiento Básico
- [ ] `UFlowFieldMovementComponent`
  - [ ] Obtener dirección del FlowField
  - [ ] Mover entidad en la dirección indicada
  - [ ] Detenerse al llegar a la meta

### 3. Visualización Mínima
- [ ] Debug básico
  - [ ] Flechas de dirección en cada celda
  - [ ] Mostrar posición de la meta
  - [ ] Mostrar posición de las entidades

### 4. Comandos Básicos
- [ ] `grid.flow.set_goal X Y` - Establecer meta
- [ ] `grid.flow.spawn N` - Generar N entidades
- [ ] `grid.flow.reset` - Reiniciar simulación

### 5. Prueba Inicial
- [ ] Escena de prueba con:
  - [ ] 5-10 entidades
  - [ ] Meta fija en el centro
  - [ ] Movimiento básico sin colisiones

---

## Implementación Completa

## 1. Configuración Inicial
- [x] Verificar registro del `GridEpochSubsystem`
  - [x] Se registra automáticamente con el World
- [ ] Configurar capas de datos
  - [ ] `DensityHeatGrid`: Para gestión de multitudes
  - [ ] `CapacityGrid`: Para límites de capacidad por celda
  - [ ] `OccupancyGrid`: Para obstáculos estáticos/dinámicos

## 2. Sistema de Metas y Navegación
- [ ] Configuración de Metas Múltiples
  - [ ] Sistema de etiquetas para diferentes tipos de metas
  - [ ] Priorización de metas basada en densidad
  - [ ] Visualización de áreas objetivo
- [ ] Integración con FlowField
  - [ ] Recalcular rutas basado en cambios de densidad
  - [ ] Actualización selectiva de áreas con alta actividad

## 3. Movimiento de Entidades
- [ ] Componente de Movimiento
  - [ ] `UFlowFieldMovementComponent`
    - [ ] Seguimiento de dirección basado en FlowField
    - [ ] Evitación de colisiones con otras entidades
    - [ ] Ajuste de velocidad según densidad
  - [ ] Sistema de formación para grupos
  - [ ] Comportamientos de espera/espera en cola

## 4. Optimización para Múltiples Entidades
- [ ] Sistema de Niveles de Detalle (LOD)
  - [ ] Agrupación de entidades lejanas
  - [ ] Simplificación de cálculos para áreas de baja prioridad
- [ ] Particionado Espacial
  - [ ] División del mundo en sectores
  - [ ] Carga/descarga dinámica de sectores

## 5. Depuración y Visualización
- [ ] Herramientas de Depuración
  - [ ] Visualización de campos de flujo
  - [ ] Mapa de calor de densidad
  - [ ] Visualización de capacidad por celda
- [ ] Comandos de Consola
  - `grid.flow.debug_density`: Mostrar/ocultar densidad
  - `grid.flow.debug_occupancy`: Mostrar ocupación
  - `grid.flow.debug_paths`: Mostrar rutas activas

## 6. Pruebas de Rendimiento
- [ ] Pruebas de Carga
  - [ ] 100+ entidades simultáneas
  - [ ] Actualización bajo carga de CPU
- [ ] Pruebas de Comportamiento
  - [ ] Colapso de embotellamientos
  - [ ] Reacción a cambios dinámicos
  - [ ] Comportamiento en intersecciones

## 7. Integración con el Editor
- [ ] Herramientas de Depuración en Editor
  - [ ] Visualización en tiempo de edición
  - [ ] Herramientas de pintado de áreas
- [ ] Configuración por Nivel
  - [ ] Zonas de spawn/objetivo
  - [ ] Áreas restringidas

## 8. Documentación
- [ ] Guía de Implementación
  - [ ] Configuración de entidades
  - [ ] Definición de comportamientos
  - [ ] Optimización de rendimiento
- [ ] Ejemplos Prácticos
  - [ ] Multitudes en espacios abiertos
  - [ ] Control de flujo en pasillos
  - [ ] Comportamientos de emergencia
