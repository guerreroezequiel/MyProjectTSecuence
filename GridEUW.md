# GridEUW - Visualización del Grid en Tiempo Real

## 1. Estructura de Datos

### a. Datos de Flujo (Flow Field)
- **Ubicación**: `Grid::Flow::GStorage` en `FlowFieldStorage.h`
- **Estructura por tile**:
  ```cpp
  struct FTileData {
      TArray<float> Dist;       // Distancias (TileDim*TileDim)
      TArray<FVector2D> Dir;    // Direcciones normalizadas
      uint32 TileVersion;       // Control de versión
  };
  ```
- **Funciones clave**:
  ```cpp
  // Obtener distancia
  float dist = Grid::Flow::ReadDist(CellXY);
  
  // Obtener dirección
  FVector2D dir = Grid::Flow::ReadDir(CellXY);
  ```

### b. Datos de Capacidad
- **Ubicación**: `Grid::Capacity::GCapTiles` en `CapacityGrid.h`
- **Estructura por tile**:
  ```cpp
  struct FTileCapacity {
      TArray<int16> Base;      // Capacidad base
      TArray<int16> Count;     // Conteo actual
  };
  ```
- **Funciones clave**:
  ```cpp
  // Obtener capacidad base
  int32 base = Grid::Capacity::GetBaseCapacity(CellXY);
  
  // Obtener conteo actual
  int32 count = Grid::Capacity::GetCurrentCount(CellXY);
  ```

## 2. Sistema de Actualización

### a. Sincronización con Epoch
- El `GridEpochSubsystem` actualiza los datos periódicamente
- Frecuencia definida en `GridConfig::EpochMs`
- Detección de cambios:
  ```cpp
  // En el tick del widget
  double CurrentTime = GetWorld()->GetTimeSeconds();
  if (GridEpoch::HasEpochAdvanced(LastUpdateTime, CurrentTime)) {
      UpdateGridVisualization();
      LastUpdateTime = CurrentTime;
  }
  ```

## 3. Diseño del Widget

### a. Estructura del Widget
```
W_GridDebug (EditorUtilityWidget)
└── CanvasPanel
    ├── GridPanel (64x64 celdas)
    │   └── W_GridCell (instancias reutilizables)
    └── InfoOverlay
        ├── MousePositionText
        ├── CellInfoText
        └── TileInfoText
```

### b. W_GridCell (Celda Individual)
- **Propiedades**:
  - `CellPosition` (FIntPoint): Posición en la grilla
  - `BaseCapacity` (int32): Capacidad base
  - `CurrentCount` (int32): Conteo actual
  - `FlowDirection` (FVector2D): Dirección del flujo
  - `bIsHovered` (bool): Estado de hover

- **Visualización**:
  - **Fondo**:
    - Verde: Count <= Base
    - Naranja: Base < Count <= EffectiveCapacity
    - Rojo: Count > EffectiveCapacity
  - **Texto**: Muestra "Count/Base"
  - **Flecha**: Opcional, muestra dirección del flujo

## 4. Implementación

### a. Clase Base del Widget
```cpp
// W_GridDebug.h
UCLASS()
class MYPROJECT_API UGridDebugWidget : public UEditorUtilityWidget
{
    GENERATED_BODY()
    
public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    
private:
    void UpdateGridVisualization();
    void UpdateCell(int32 X, int32 Y);
    
    UPROPERTY(meta = (BindWidget))
    class UUniformGridPanel* GridPanel;
    
    UPROPERTY(EditDefaultsOnly, Category = "Grid")
    TSubclassOf<class UGridCellWidget> CellWidgetClass;
    
    FIntPoint CurrentTile;
    TArray<UGridCellWidget*> CellWidgets;
    double LastUpdateTime = 0.0;
};
```

### b. Actualización de Celdas
```cpp
void UGridDebugWidget::UpdateGridVisualization()
{
    if (!GridPanel) return;
    
    // Actualizar todas las celdas del tile actual
    for (int32 Y = 0; Y < 64; ++Y) {
        for (int32 X = 0; X < 64; ++X) {
            UpdateCell(X, Y);
        }
    }
}

void UGridDebugWidget::UpdateCell(int32 X, int32 Y)
{
    FIntPoint CellPos(CurrentTile.X * 64 + X, CurrentTile.Y * 64 + Y);
    
    // Obtener datos
    int32 Base = Grid::Capacity::GetBaseCapacity(CellPos);
    int32 Count = Grid::Capacity::GetCurrentCount(CellPos);
    FVector2D FlowDir = Grid::Flow::ReadDir(CellPos);
    
    // Actualizar UI
    if (UGridCellWidget* Cell = GetCellAt(X, Y)) {
        Cell->UpdateVisuals(Base, Count, FlowDir);
    }
}
```

## 5. Optimizaciones

1. **Actualización Selectiva**:
   - Usar `TileVersion` para detectar cambios
   - Actualizar solo celdas modificadas

2. **Pooling**:
   - Reutilizar instancias de `W_GridCell`

3. **Niveles de Detalle**:
   - Reducir actualizaciones cuando el widget no está visible
   - Simplificar la visualización con muchos tiles visibles

## 6. Próximos Pasos

1. Crear `W_GridCell` básico
2. Implementar `W_GridDebug` con GridPanel
3. Conectar con datos del grid
4. Añadir interacción (hover, selección)
