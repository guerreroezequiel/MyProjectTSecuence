# Grid Debug – Guía de visualización por capas

## Objetivo
- Visualizar en tiempo real las capas del sistema de grid para depurar y tunear: Flow (dist/dir), Heat/Density, Occupancy, Capacity.
- Controlarlas desde Unreal (consola, CVars, y/o `UDeveloperSettings`) para una experiencia amigable.

---

## Arquitectura recomendada
- **[Componente de dibujo]** `UGridDebugDrawComponent`
  - Ubicación sugerida: `Source/MyProjectTSecuence/Private/GridSystem/Debug/`.
  - Se adjunta a un `AActor` (por ejemplo, un “Debug Manager”) en el nivel.
  - En `TickComponent()` obtiene parámetros (CVars/Settings) y dibuja con `DrawDebug*`.
- **[Consola/CVars]** Comandos para toggles y parámetros (ver sección de comandos).
- **[Developer Settings]** Clase `UGridDevSettings` (opcional) en `Core/GridConfig.h/.cpp` para exponer parámetros en Project Settings.

---

## Paso 1: Crear e integrar el componente de Debug (mínimo viable)

- **[Crear clase C++]** `UGridDebugDrawComponent` (deriva de `UActorComponent`)
  - Archivos:
    - `Source/MyProjectTSecuence/Public/GridSystem/Debug/GridDebugDrawComponent.h`
    - `Source/MyProjectTSecuence/Private/GridSystem/Debug/GridDebugDrawComponent.cpp`
  - Métodos mínimos a implementar:
    - `InitializeComponent()` → cachear referencias si hace falta.
    - `TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)` → dibujado.
  - Flags:
    - `PrimaryComponentTick.bCanEverTick = true;`
    - `bAutoActivate = true;`

- **[Toggles leídos por CVar]** dentro de `TickComponent()` (crear CVars luego):
  - `grid.debug.flow` (bool), `grid.debug.flow_mode` (string/enum), `grid.debug.flow_scale` (float)
  - `grid.debug.heat` (bool), `grid.debug.heat_range` (min,max)
  - `grid.debug.occ` (bool), `grid.debug.cap` (bool)
  - `grid.debug.grid_step` (int), `grid.debug.tile` (tx,ty)

- **[Dibujo mínimo]** dentro de `TickComponent()`:
  - Obtener bounds a dibujar (todo el grid o un tile) usando `GridWorld`.
  - Recorrer celdas con `grid_step`.
  - Si `flow` activo: leer `ReadDir(cell)`/`ReadDist(cell)` de `FlowFieldStorage` y dibujar flechas/heatmap.
  - Si `heat` activo: `Density::GetHeat(cell)` → color y `DrawDebugPoint`/`DrawDebugSolidBox`.
  - Si `occ` activo: `IsBlocked(cell)`/`IsPortal(cell)` → boxes/íconos.
  - Si `cap` activo: `Capacity::GetBaseCapacity/GetCurrentCount` → color por overratio o texto.

- **[Añadir al nivel]**
  - En el Editor, crea un `Empty Actor` llamado `BP_GridDebugManager` y agrega el componente `GridDebugDrawComponent`.
  - Alternativa solo código: en `GameMode` o `BeginPlay`, spawnear un actor y `CreateDefaultSubobject<UGridDebugDrawComponent>(...)`.

- **[Probar con consola]** (antes de CVars, puedes cablear toggles a constantes):
  - `grid.flow.set_goal 20 20`
  - `grid.flow.rebuild_now`
  - `grid.debug.flow on` (una vez registremos CVars/comandos de debug)

En el Paso 2 definiremos los CVars y comandos `grid.debug.*` para controlar los toggles y parámetros sin recompilar.

---

## Capas y qué dibujar
- **[Flow]** (`FlowFieldStorage.h`)
  - `dir`: flechas por celda (8-dir suavizada). Escalar longitud por magnitud (si aplica).
  - `dist`: heatmap del campo escalar (colormap simple min→max).
  - Opcional: resaltar metas y fronteras de tiles, mostrar `tileVersion`.
- **[Heat/Density]** (`DensityHeatGrid.h`)
  - Color por celda según `GetHeat(cell)` con rampa configurable.
  - Opción para mostrar sólo celdas con heat > umbral.
- **[Occupancy]** (`OccupancyGrid.h`)
  - Cuadrados/boxes para `Obstacle`, bordes/íconos para `Portal`.
  - Modo wireframe para walkables, sólido para bloqueados.
- **[Capacity]** (`CapacityGrid.h`)
  - Número/overlay con `Base` y `Count` por celda (texto pequeño), o color por `OverRatio`.
  - Opción para ocultar texto por performance.

---

## Comandos/Consola (propuestos)
Estos comandos se agregan en `FlowFieldConsole.cpp` (como los existentes `grid.flow.*`, `grid.heat.*`, `grid.occ.*`, `grid.cap.*`).

- **Toggles generales**
  - `grid.debug.flow on|off`
  - `grid.debug.heat on|off`
  - `grid.debug.occ on|off`
  - `grid.debug.cap on|off`
- **Parámetros visuales**
  - `grid.debug.flow_mode dir|dist|both`
  - `grid.debug.flow_scale s` (escala de flechas)
  - `grid.debug.heat_range min max` (rango colormap)
  - `grid.debug.tile tx ty` (enfocar/filtrar un tile)
  - `grid.debug.grid_step n` (dibujar 1 de cada n celdas para rendimiento)
- **Solver params (ya implementado)**
  - `grid.flow.set_params T AlphaHeat BetaCapacity`
- **Authoring (ya implementado)**
  - `grid.occ.set x y state`, `grid.occ.clear`
  - `grid.cap.set_base x y v`, `grid.cap.set_count x y v`, `grid.cap.clear`
  - `grid.heat.inject x y v`, `grid.heat.decay dt`, `grid.heat.clear`

---

## Parametrización vía CVars y Settings
- **CVars**: usar `static TAutoConsoleVariable<...>` o `IConsoleManager::RegisterConsoleVariable` para toggles y floats (ej: `grid.debug.flow`, `grid.debug.flow_scale`, `grid.debug.heat_min/max`).
- **Developer Settings** (opcional):
  - `UCLASS(config=Game, defaultconfig)` → `UGridDevSettings`
  - Miembros: `bDrawFlow`, `bDrawHeat`, `FlowArrowScale`, `HeatMin`, `HeatMax`, etc.
  - Ventaja: editable en editor y persistente.
  - Sincronizar: el componente lee primero CVars (si están set), sino toma Settings.

---

## Ciclo de dibujo (pseudocódigo)
1) Determinar bounds visibles: cámara o `grid.debug.tile` si se fijó.
2) Iterar por tiles/celdas con `grid_step` para limitar densidad.
3) Por celda:
   - Si `bDrawFlow`: leer `ReadDir(cell)` y/o `ReadDist(cell)` y dibujar.
   - Si `bDrawHeat`: `Density::GetHeat(cell)` → color.
   - Si `bDrawOcc`: `IsBlocked(cell)`/`IsPortal(cell)` → ícono/box.
   - Si `bDrawCap`: `GetBaseCapacity(cell)`, `GetCurrentCount(cell)` → texto/colores.
4) Opcional: leyenda en pantalla (rango heat, T/α/β actuales).

---

## Performance y buenas prácticas
- Dibujar con **step** (ej: 2–4) y a **nivel de tile** cuando sea posible.
- Evitar cálculo caro por celda en `Tick`: cachear colormap y posiciones de centros (`GridWorld::CellToWorldCenterXY`).
- Recalcular sólo cuando cambien toggles/params o se marque dirty.
- Usar `PersistentLines=false` y tiempos cortos para evitar sobreacumulación.

---

## Integración en Unreal (paso a paso)
Esta guía refleja el estado actual del repo con el componente `UGridDebugDrawComponent` ya implementado.

- **[Compilar]** Build del proyecto en el Editor o IDE.
- **[Crear Blueprint]** Crea un Blueprint de tipo `Actor` llamado `BP_GridDebugManager`.
- **[Agregar componente]** En el BP, Add Component → `GridDebugDrawComponent`.
- **[Colocar en el nivel]** Arrastra `BP_GridDebugManager` al mapa.
- **[Play]** Entra en Play (PIE) para ver el debug. Por defecto dibuja la capa `Capacity` en el tile `(0,0)`.

Parámetros editables desde el BP (detalles en `Source/MyProjectTSecuence/Public/GridSystem/Debug/GridDebugDrawComponent.h`):
- `bDrawCapacity` (on/off)
- `GridStep` (densidad de muestreo)
- `BoxExtent`, `TextZOffset` (tamaño/offset visual)

Importante:
- No hace falta colocar un "Grid Actor" principal. El grid corre en `UGridEpochSubsystem` y los datos viven en estructuras estáticas por capa.
- `UGridEpochSubsystem` se instancia solo por ser `UWorldSubsystem`.

Para ver datos en `Capacity` rápidamente (consola):
- `grid.cap.set_base 12 13 4`
- `grid.cap.set_count 12 13 6`
- `grid.cap.clear`

CVars de debug (opcional, si se registran):
- `grid.debug.cap 1|0`
- `grid.debug.grid_step N`

---

## Roadmap sugerido
- [1] Componente de debug + CVars/Consola (mínimo viable).
- [2] Opcional: `UGridDevSettings` para exponer en Project Settings.
- [3] Leyendas/overlay con parámetros activos y escalas.
- [4] Capturas automáticas (screencaps) para testeo visual por escenario.

---

## Referencias
- `FLOWFIELD_CORE.md` (overview y consola).
- `FlowFieldStorage.h`, `FlowFieldSolver.h`, `FlowFieldRebuilder.h`.
- `DensityHeatGrid.h`, `OccupancyGrid.h`, `CapacityGrid.h`.

---

## Troubleshooting
- Si no ves nada:
  - Verifica que estás cerca del tile `(0,0)` o ajusta `BoxExtent`/`TextZOffset` en el componente.
  - Asegúrate de estar en Play; por defecto no usamos líneas persistentes.
  - Inyecta datos con los comandos `grid.cap.*` para tener algo que mostrar.
- Performance:
  - Sube `GridStep` (2–4) para reducir densidad de dibujo.
