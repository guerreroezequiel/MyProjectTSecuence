
# Guía de Implementación: Widget de Depuración de Grilla

## 1. Configuración Inicial

### 1.1 Crear el Widget
1. Crear nuevo Widget Blueprint basado en `GridDebugWidgetBase`
2. Nombrar: `WBP_GridDebug`

### 1.2 Diseño de la Interfaz
1. Añadir `Canvas Panel` como raíz
2. Añadir `Image` llamado `GridImage` que ocupe todo el espacio
3. Crear un panel lateral (`Vertical Box`) para controles

## 2. Asignación Directa del Render Target (sin Material)

1. En C++ (`CreateGridRenderTarget()`), el `UCanvasRenderTarget2D` se crea y se asigna al `UImage` mediante un `FSlateBrush`:
   - Llama a `GridRenderTarget->InitAutoFormat(1024, 1024);`
   - Crea un `FSlateBrush`, usa `Brush.SetResourceObject(GridRenderTarget);`, define `Brush.ImageSize` y asigna con `GridImage->SetBrush(Brush);`
   - Vincula el delegado `OnCanvasRenderTargetUpdate` para dibujar la grilla.
   - Nota: incluye `#include "Styling/SlateBrush.h"` en el `.cpp`.

2. En el Widget Blueprint (`WBP_GridDebug`):
   - Asegúrate de que el control se llame exactamente `GridImage` (BindWidget).
   - No necesitas crear ni asignar un material o textura en el Designer (C++ lo hace).

## 3. Implementación en Blueprint

### 3.1 Event Graph
```blueprint
Event Construct:
    [Call Parent]           // Parent: Construct
    [Optional] [Call UpdateGridVisualization]

// No llames CreateGridRenderTarget desde Blueprint (lo hace C++ en NativeConstruct)

Event Tick:    // Opcional, solo si necesitas interacción de hover
    [If] bIsHovering
        [Get Mouse Position]
        [Convert to Grid Coordinates]
        [Update Cell Info Panel]
```
## 4. Integración con el Juego
1. En el GameMode o PlayerController:
cpp
// Crear instancia del widget
UGridDebugWidget* DebugWidget = CreateWidget<UGridDebugWidget>(GetWorld(), DebugWidgetClass);
DebugWidget->AddToViewport();

2. Conectar con el subsistema:
cpp
UGridDebugSubsystem* Subsystem = GetWorld()->GetSubsystem<UGridDebugSubsystem>();
if (Subsystem)
{
    Subsystem->OnGridUpdated.AddDynamic(this, &UYourWidget::HandleGridUpdate);
}



----------------------------------------------------------------------------------------------------------
# Implementación del GridDebugWidget v2

## Arquitectura Basada en Capas con Render Targets

### Visión General
Sistema de visualización de grilla que utiliza una arquitectura en capas con los siguientes componentes principales:

1. **GridDebugWidgetBase** (Widget de UI)
   - Contiene la lógica para mostrar la interfaz de usuario
   - Gestiona la interacción del usuario (botones, sliders, etc.)
   - Muestra el `UCanvasRenderTarget2D` directamente en un `UImage`

2. **Render Targets** (Capas)
   - `RT_Grid_Base`: Capa base con la grilla y celdas
     - Dibuja la estructura de la grilla
     - Muestra el estado de las celdas
     - Incluye coordenadas y etiquetas
   - `RT_FlowField`: Capa de vectores de flujo
     - Muestra direcciones y fuerzas
     - Se actualiza dinámicamente
   - `RT_Density`: Capa de densidad/calor
     - Mapa de calor de ocupación
     - Niveles de influencia
   - Características:
     - Cada capa es independiente
     - Se pueden habilitar/deshabilitar individualmente
     - Control de opacidad por capa

3. **Materiales**
   - `M_GridDebug`: Material base para la visualización
     - Combina múltiples Render Targets
     - Control de opacidad por capa
     - Ajustes de visualización (tinte, contraste, etc.)
   - `M_FlowArrow`: Visualización de vectores
   - `M_HeatMap`: Visualización de mapa de calor

### Flujo de Trabajo por Capas

1. **Inicialización**:
   - `GridDebugWidgetBase` crea los Render Targets para cada capa
   - Se configuran los materiales con sus respectivos Render Targets
   - Se crea un material final que combina todas las capas
   - Se asigna el material final a un `Image` en la interfaz

2. **Actualización de Capas**:
   - Cada capa se actualiza de forma independiente
   - Proceso por capa:
     1. Se selecciona el Render Target de la capa
     2. Se limpia el contenido anterior
     3. Se dibujan los elementos específicos de la capa
     4. Se actualiza la textura

3. **Composición Final**:
   - El material final (`M_GridDebug`) combina todas las capas activas
   - Se aplican los ajustes de opacidad y mezcla
   - El resultado se muestra en la interfaz de usuario

4. **Optimizaciones**:
   - Solo se actualizan las capas que han cambiado
   - Las capas inactivas no se procesan
   - Se puede ajustar la calidad por capa

### Estructura de Capas

```
UGridDebugWidget (UserWidget)
├── CanvasPanel (Root)
│   ├── VerticalBox (Contenedor principal)
│   │   ├── HorizontalBox (Controles superiores)
│   │   │   ├── Button_PlayPause
│   │   │   ├── Button_Step
│   │   │   ├── Button_Stop
│   │   │   └── TextBlock_Status
│   │   ├── HorizontalBox (Controles de capas)
│   │   │   ├── Toggle_BaseLayer
│   │   │   ├── Toggle_FlowLayer
│   │   │   └── Slider_Opacity
│   │   ├── Image_GridBase (Render Target - Capa Base)
│   │   ├── Image_FlowField (Render Target - Vectores de Flujo)
│   │   ├── Image_Density (Render Target - Mapa de Calor)
│   │   └── TextBlock_Info
```

## Plan de Implementación Detallado

### 1. Configuración de Render Targets

#### 1.1. Creación de Assets
- `RT_Grid_Base` (1024x1024, RGBA8, No Mipmaps)
  - Estado de celdas (libre/ocupado/obstáculo)
  - Bordes de la grilla
  - Coordenadas/etiquetas

- `RT_Grid_Flow` (1024x1024, RGBA16F, No Mipmaps)
  - Vectores de flujo (RG: dirección, B: magnitud)
  - Costos de movimiento
  - Meta más cercana

- `RT_Grid_Density` (1024x1024, R16F, No Mipmaps)
  - Mapa de densidad/calor
  - Niveles de ocupación
  - Zonas de influencia

#### 1.2. Sistema de Capas
- Clase `UGridLayerManager`
  ```cpp
  class UGridLayerManager : public UObject
  {
      // Configuración de capas
      TArray<FGridLayer> Layers;
      
      // Inicialización
      void Initialize(const FIntPoint& GridDimensions);
      
      // Actualización de capas
      void UpdateLayer(EGridLayerType LayerType, const FUpdateRegion& Region);
      
      // Renderizado
      void DrawCell(EGridLayerType Layer, const FIntPoint& CellCoord, const FLinearColor& Color);
  };
  ```

### 2. Implementación del Widget

#### 2.1. Estructura UMG
- `Image_GridBase`: Fondo con la cuadrícula y celdas
- `Image_FlowField`: Flechas y vectores de flujo
- `Image_Density`: Mapa de calor semitransparente
- Controles de capa (toggle visibilidad/opacidad)

#### 2.2. Interacción
- Detección de celdas bajo el cursor
- Selección con clic/arrastre
- Tooltips con información detallada
- Zoom/desplazamiento con gestos táctiles

### 3. Optimizaciones

#### 3.1. Actualización Diferida
- Sistema de "suciedad" para actualizar solo lo necesario
- Límite de FPS para actualizaciones
- Niveles de detalle (LOD) para vista lejana

#### 3.2. Gestión de Memoria
- Pooling de recursos gráficos
- Liberación de memoria cuando no es visible
- Reducción de resolución en dispositivos móviles

## Próximos Pasos

1. Crear los Render Targets básicos
2. Implementar UGridLayerManager
3. Configurar el sistema de capas en UMG
4. Implementar el dibujado de la capa base
5. Añadir interacción básica

## Recursos Necesarios

- Texturas:
  - `T_Grid_Cell_64x64`: Textura base para celdas
  - `T_Arrow_64x64`: Para vectores de flujo
  - `T_Heat_Gradient`: Gradiente para el mapa de calor

- Materiales:
  - `M_GridCell`: Material para celdas base
  - `M_FlowArrow`: Material para visualización de flujo
  - `M_HeatMap`: Material para mapa de calor

## Notas de Rendimiento
- Usar `ENQUEUE_RENDER_COMMAND` para operaciones de renderizado
- Implementar `FDeferredCleanupSlateResource` para recursos pesados
- Usar `SInvalidationPanel` para actualizaciones eficientes
- Considerar `SZoomPan` para la navegación con gestos
