### 2. Componentes Principales

#### 2.1. [FTileContext.cpp]
- Responsabilidad: Gestiona el estado y versionamiento de un tile individual
- Características:
  - Mantiene seguimiento de epochs para costos estáticos, goals y flow fields
  - Maneja banderas de suciedad (dirty flags) para actualizaciones eficientes
  - Proporciona métodos para marcar cambios y validar estado

#### 2.2. [FFlowField.cpp]
- Responsabilidad: Implementa la lógica de un campo de flujo individual
- Características:
  - Validación de estado basada en epochs
  - Sistema de reconstrucción condicional
  - Soporte para múltiples intenciones (Players, Influences, Ambient)

#### 2.3. [UFlowFieldSystem.cpp](Componente de Actor en el mundo)
- Responsabilidad: Orquestar el pipeline por tile e integrar el Player como fuente de goals
- Características:
  - Bucle de actualización principal sobre tiles activos (HOT ∪ WARM)
  - Gestión de estados HOT/WARM/COLD a partir del tile del Player
  - Actualiza GoalSet de Players cuando cambia el tile o la celda del Player
  - Pipeline por contrato: UpdateEpochs() → Bake FinalCost_Static → IsValid/Rebuild
  - Sistema de depuración integrado (bordes/arrows vía consola)

### 3. Flujo de Datos

1. Actualización de Estados:
   - Occupancy/BaseCost marcan bStaticCostDirty en el TileContext correspondiente.
   - Movimiento del Player marca bGoalsDirty_Players en tiles activos cuando:
     - Cambia de tile (HOT cambia y se recomputa WARM), o
     - Cambia de celda dentro del mismo tile.

2. Reconstrucción:
   - UpdateEpochs() avanza epochs y limpia dirty flags.
   - Si cambió StaticCostEpoch, se hornea FinalCost_Static.
   - FFlowField::IsValid() compara epochs y reconstruye si es necesario.

3. Debug:
   - Visualización de bordes por estado y flechas de flow en HOT/WARM.

### 4. Próximos Pasos

1. Completar/optimizar la lógica interna del solver en FFlowField::Rebuild.
2. Añadir soporte para costos dinámicos y su integración con epochs.
3. Habilitar intents Influences y Ambient con fuentes reales y toggles de debug.
4. Optimizar rendimiento (prioridades/budgets) para escenarios con muchos tiles.
5. Extender depuración (telemetría por tile, overlays de costos).

==============================================================================================================

# Estado Actual de la Implementación

## 1. FlowField
✅ IsValid() (FlowField.h) — Implementado:
- Verifica BuiltStaticCostEpoch vs Context.GetStaticCostEpoch()
- Verifica BuiltGoalsEpoch vs Context.GetGoalsEpoch(Intent)

🔄 Rebuild() (FlowField.h) — Parcial:
- Actualiza epochs
- Falta la lógica completa del solver

## 2. TileContext
✅ Epochs:
- StaticCostEpoch
- GoalsEpochs por EFlowIntent
- FlowEpochs por EFlowIntent

✅ Dirty Flags:
- bStaticCostDirty
- GoalsDirtyFlags por intento

✅ UpdateEpochs():
- Incrementa epochs con dirty
- Limpia flags dirty

## 3. Componentes Faltantes/Estado

### TileStaticData
✅ Integrado vía Grid::StaticCost::BakeFinalCostStatic(TileXY, *Tile)
- Se hornea FinalCost_Static cuando avanza StaticCostEpoch.
- Integrado con dirty flags de ocupación/base.

### Bake de Costos Estáticos
✅ Integrado en el pipeline
- Se ejecuta tras UpdateEpochs() si cambia StaticCostEpoch.

### FlowFieldSolver
❓ No visible en archivos revisados
- Debe consumir FinalCost_Static
- Debe generar IntegrationField y DirectionField

### FlowFieldSystem
✅ Orquestación implementada
- Orden: UpdateEpochs() → Bake FinalCost_Static → IsValid/Rebuild (por intención)
- HOT/WARM/COLD derivados del tile del Player
- GoalSet de Players se actualiza al cambiar tile o celda del Player

## 4. Próximos Pasos Recomendados

1. Afinar solver de Rebuild (estructuras/algoritmos).
2. Consolidar telemetría de epochs/flags y budgets por tile.
3. Habilitar Influences/Ambient como intents funcionales.