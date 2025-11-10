# TODO: Implementación del Sistema FlowField

## 1. Configuración Inicial
- [ ] Verificar registro del `GridEpochSubsystem`
  - Agregar verificación en tiempo de ejecución
  - Forzar registro en `StartupModule` si es necesario
  SE REGISTRA POR DEFECTO CON EL WORLD

## 2. Implementación de Metas (Goals)
- [ ] Crear sistema de configuración de metas
  - Interfaz para agregar/remover metas
  - Visualización de metas en el editor
- [ ] Implementar `RequestDirtyRebuildAll()` en `GridEpochSubsystem`
  - Escanear área del mundo
  - Marcar tiles sucios

## 3. Sistema de Movimiento
- [ ] Crear `UFlowFieldMovementComponent`
  - Seguir direcciones del FlowField
  - Manejar rotación y velocidad
  - Detección de colisiones

## 4. Depuración
- [ ] Comandos de consola
  - `grid.flow.status`: Estado actual
  - `grid.flow.debug_tile`: Depurar tile específico
  - `grid.flow.set_goal`: Establecer meta
- [ ] Visualización en tiempo real
  - Flechas de dirección
  - Distancia al objetivo
  - Estado de los tiles

## 5. Optimización
- [ ] Sistema de pooling para tiles
- [ ] Actualización por sectores
- [ ] Priorización de actualizaciones

## 6. Documentación
- [ ] Guía de uso
- [ ] Ejemplos de implementación
- [ ] Solución de problemas comunes

## 7. Pruebas
- [ ] Pruebas con múltiples agentes
- [ ] Pruebas de rendimiento
- [ ] Pruebas con obstáculos dinámicos
