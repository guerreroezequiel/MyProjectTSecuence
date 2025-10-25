# Arquitectura del Sistema de Depuración de la Grilla

## 1. Visión General

Sistema de depuración modular para visualizar y analizar el estado de la grilla en tiempo real, organizado por capas funcionales.

## 2. Subsistema de Depuración (C++)

### 2.1. Gestión Básica
- **GetInstance**: Obtiene la instancia del subsistema para el mundo actual
- **SetDebugMode**: Activa/desactiva el modo de depuración
- **SetPauseState**: Pausa/reanuda la simulación
- **StepSimulation**: Avanza un paso de simulación
- **IsDebugActive**: Verifica si la depuración está activa

## 3. Consulta por Capas

### 3.1. Capa Base (Celdas)
- **GetCellInfo**: Obtiene información básica de una celda (posición, estado, flags)
- **GetCellNeighbors**: Devuelve información de las celdas adyacentes
- **GetCellWorldPosition**: Convierte coordenadas de grilla a posición en mundo

### 3.2. Capa de Ocupación/Capacidad
- **GetCellOccupancy**: Obtiene el nivel de ocupación actual de la celda
- **GetCellCapacity**: Devuelve la capacidad máxima de la celda
- **GetSubSlotStatus**: Estado de los sub-slots (ocupado/libre)
- **GetBlockedCells**: Lista de celdas bloqueadas/obstáculos

### 3.3. Capa de FlowField
- **GetFlowVector**: Obtiene el vector de flujo en una posición
- **GetFlowCost**: Devuelve el costo de movimiento en la celda
- **GetGoalInfluence**: Influencia del objetivo más cercano
- **GetFlowFieldData**: Datos completos del campo de flujo para visualización

### 3.4. Capa de Densidad/Calor
- **GetDensityValue**: Valor de densidad en una posición
- **GetHeatMapData**: Datos de calor para visualización
- **GetDensityThresholds**: Umbrales actuales para transiciones de densidad

### 3.5. Capa de Roles (LOD)
- **GetAgentRoles**: Distribución actual de roles (Líder/Seguidor/Horda)
- **GetLODInfo**: Nivel de detalle actual para un área
- **GetClusterData**: Información de agrupamiento de agentes

## 4. Control de Visualización

### 4.1. Filtros
- **SetLayerVisibility**: Muestra/oculta capas específicas
- **SetViewRange**: Define el rango de visualización
- **SetDebugFlags**: Configura banderas de depuración

### 4.2. Overlays
- **DrawDebugPrimitives**: Renderiza elementos de depuración
- **UpdateTextOverlay**: Actualiza información textual
- **DrawFlowVectors**: Visualización de vectores de flujo

## 5. Herramientas de Análisis

### 5.1. Consultas Espaciales
- **RaycastGrid**: Intersección de rayo con la grilla
- **GetCellsInRadius**: Celdas dentro de un radio
- **FindPath**: Depuración de búsqueda de caminos

### 5.2. Métricas
- **GetPerformanceStats**: Estadísticas de rendimiento
- **GetMemoryUsage**: Uso de memoria por componente
- **DumpDebugInfo**: Vuelca información de depuración a consola/archivo

## 6. Eventos

### 6.1. Callbacks
- **OnCellUpdated**: Se dispara cuando una celda cambia
- **OnFlowFieldRebuilt**: Evento de actualización del campo de flujo
- **OnDebugStateChanged**: Cambios en el estado de depuración

## 7. Integración con Blueprint

### 7.1. Nodos Expuestos
- **Get Grid Debug Subsystem**: Acceso al subsistema
- **Is Cell Walkable**: Consulta de transitabilidad
- **Draw Debug Cell**: Dibuja una celda en el viewport
- **Get Cell Info**: Versión amigable para Blueprint de GetCellInfo

### 7.2. Estructuras de Datos
- **FCellDebugInfo**: Información de depuración de celdas
- **FFlowFieldData**: Datos del campo de flujo
- **FDensityMapData**: Datos de densidad/calor
