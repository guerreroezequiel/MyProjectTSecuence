# Pendientes finales — Cierre del pipeline viejo (FlowField)

Este documento enumera los cambios **mínimos y necesarios** para cerrar correctamente el pipeline viejo y dejar el pipeline nuevo (epochs + dirty + storage por intent) completamente definido y estable.

No incluye refactors grandes ni cambios de arquitectura.

---

## 1. FlowField.cpp — Cerrar casos de early-return (validez)

### Problema
En `FFlowField::Rebuild()` existen salidas tempranas cuando:
- No hay goals
- El `GridWorld` / `GridEpochSubsystem` no existe

En estos casos:
- No se actualiza la metadata de build
- El flowfield queda inválido permanentemente
- El sistema intenta rebuild en cada tick/epoch

### Cambio requerido
Definir explícitamente que:
- **Un flowfield sin goals es válido**
- Aun si no se ejecuta el solver, se debe:
  - Actualizar la metadata (`BuiltMeta`) con los epochs actuales
  - Evitar loops infinitos de rebuild

> Opcional MVP: limpiar dist/dir a valores por defecto  
> Obligatorio MVP: marcar el flowfield como construido

---

## 2. FlowFieldSolver.h — Limpiar contrato del solver (pipeline nuevo)

### 2.1 Parámetros fuera del MVP
`FSolverParams` todavía incluye:
- Heat
- Capacity

Pero:
- El pipeline MVP no usa estas capas
- El solver actual no las integra realmente

### Cambio requerido (una de dos):
- Eliminar estos parámetros del contrato
**o**
- Marcar explícitamente como `TODO / post-MVP` y documentar que **no se usan**

Objetivo: evitar ambigüedad conceptual del solver.

---

### 2.2 Duplicación de APIs (con / sin intent)
Existen dos caminos:
- `SolveTile*()` → default Players (pipeline viejo / compat)
- `SolveTile*WithIntent()` → pipeline nuevo

### Cambio requerido
Dejar explícito que:
- El pipeline nuevo **solo** usa las funciones `*WithIntent`
- Las variantes sin intent quedan:
  - deprecated
  - compat
  - o solo para tests

Objetivo: evitar escrituras accidentales al intent `Players`.

---

## 3. FlowFieldConsoleLite.cpp — Dirty global incompleto

### 3.1 `grid.occ.set`
✔ Correcto  
- Marca dirty de costos estáticos
- Calcula TileXY correctamente

---

### 3.2 `grid.occ.clear`
❌ Incompleto para el pipeline nuevo

Actualmente:
- Limpia el grid
- No invalida tiles
- No avanza epochs
- No marca dirty

### Cambio requerido
Al limpiar ocupación global:
- Marcar dirty de costos estáticos para los tiles afectados  
  **o**
- Disparar una invalidación/rebuild global vía sistema de epochs

Objetivo: que los flowfields reflejen el estado real del grid luego del clear.

---

### 3.3 Limpieza de comandos/documentación
En comentarios de ayuda todavía aparecen:
- comandos de capacity / pipeline viejo

### Cambio requerido
- Eliminar referencias a comandos que ya no existen
- Dejar la consola alineada con el pipeline actual

---

## 4. Estado final esperado

Una vez aplicados estos cambios:
- No quedan loops de rebuild
- El estado “sin goals” está bien definido
- El solver tiene un contrato claro (MVP)
- La consola no deja el sistema en estado inconsistente
- No quedan rastros funcionales del pipeline viejo

Esto cierra formalmente la eliminación del pipeline anterior.
