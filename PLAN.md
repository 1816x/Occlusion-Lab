# Plan de migración de Occlusion Lab

## 1. Objetivo del proyecto

Occlusion Lab pretende convertirse en una aplicación científica de escritorio para estudiar, de forma reproducible, escenarios sintéticos de oclusión. La migración a C++ busca contratos de dominio estrictos, precisión y una base adecuada para bibliotecas científicas nativas. El producto final será **desktop-first**. La aplicación TypeScript actual se conserva temporalmente como referencia de comportamiento hasta disponer de pruebas de paridad. El proyecto sigue siendo educativo y sintético; no posee validación clínica salvo que una fase futura la documente explícitamente.

## 2. Estado actual

- **Fase actual:** Phase 4.1: Golden Pose Fixtures and C++ Contract Parity
- **Estado:** In progress
- **Rama y commit iniciales:** `main` en `11ca272b18467ef0a44140b14e075a311dbb6139`
- **Rama activa:** `codex/phase-4.1-pose-golden-parity`
- **Pull request de Phase 4.1:** no abierto; publicación bloqueada (véase §7)
- **Última actualización significativa:** 2026-08-25 — implementación y verificación local de Phase 4.1 completadas; PR todavía sin abrir, por lo que la fase permanece `In progress`.

La aplicación web heredada continúa disponible y funcional como referencia. La fundación nativa configura y compila en Debug/Release, `occlusion-core` ofrece unidades fuertes y validación, la CLI ejecuta su autocomprobación determinista y las 17 pruebas CTest pasan. Phase 4 se completó mediante el PR [#10](https://github.com/1816x/Occlusion-Lab/pull/10), mergeado como `11ca272b18467ef0a44140b14e075a311dbb6139`. La verificación local registró 17/17 CTest y 62/62 Vitest. El CI web remoto fue exitoso; el workflow nativo remoto falló y no se presenta como verificado.

## 3. Estado de arquitectura

| Componente | Implementación actual | Implementación objetivo | Estado de migración | Notas |
|---|---|---|---|---|
| Contratos de dominio | TypeScript y nuevos contratos C++ | Biblioteca C++20 `occlusion-core` | Fundación implementada | Sin lógica clínica. |
| Unidades y transformaciones | Tipos fuertes C++ | Tipos fuertes C++ con metros internos | Fundación implementada | Conversión de mm explícita. |
| Procesamiento geométrico | Flujo web existente | CGAL | Futuro | No se implementa en Phase 4. |
| Detección de colisiones | Worker web/Rapier | FCL | Futuro | FCL se reservará para colisión y distancia. |
| Análisis de barrido de movimiento | Web existente | Motor C++ | Futuro | Primero se crearán fixtures dorados. |
| Renderer | Three.js | VTK | Futuro | No forma parte del core. |
| UI de escritorio | No existe | Qt 6 | Futuro | Arquitectura desktop-first. |
| CLI | `occlusion-cli` | `occlusion-cli` extensible | Fundación implementada | Solo versión y autocomprobación. |
| Pruebas | Vitest/web y GoogleTest nativo | GoogleTest + pruebas web | Fundación implementada | Se mantiene la suite heredada. |
| CI | Web y workflow nativo | CI web y nativa multiplataforma | Implementado, pendiente ejecución remota | Sin debilitar CI existente. |
| Aplicación web heredada | Next.js/React/Three.js | Referencia temporal | Conservada | No eliminar antes de paridad y UI sustituta. |

## 4. Lista de comprobación de Phase 4

- [x] Crear `PLAN.md` y registrar el SHA inicial real.
- [x] Documentar la decisión arquitectónica en `docs/adr/0001-cpp-desktop-architecture.md`.
- [x] Añadir workspace reproducible C++20, presets Debug/Release, vcpkg y formato.
- [x] Habilitar warnings portables, CTest y compile commands sin rutas absolutas.
- [x] Declarar Eigen y GoogleTest como dependencias básicas y features opcionales `geometry`, `collision` y `desktop`.
- [x] Implementar `occlusion-core` con unidades fuertes, vectores, poses y transformaciones.
- [x] Implementar contratos de contacto, evaluación y barrido, con errores estructurados.
- [x] Rechazar NaN e infinitos sin clamp, usando `double` y metros internos.
- [x] Implementar `occlusion-cli --version` y `--self-check` deterministas mediante la API pública.
- [x] Añadir las 15 coberturas nativas requeridas, incluidas CLI y headers públicos.
- [x] Añadir CI nativa para Ubuntu, Windows y macOS con formato, configure, build y CTest.
- [x] Actualizar `README.md`, `docs/architecture.md` y `docs/design-decisions.md`.
- [x] Ejecutar configure/build/test nativo Debug y configure/build Release.
- [x] Ejecutar `npm ci`, lint, typecheck, test, verificaciones de assets/Rapier, build y audit.
- [x] Verificar ausencia de artefactos, rutas locales, binarios nuevos y dependencias prohibidas en core.
- [x] Preservar sin cambios funcionales la aplicación web, Worker, schemas y tolerancias.
- [x] Abrir y fusionar el PR de Phase 4: https://github.com/1816x/Occlusion-Lab/pull/10.

## 5. Matriz de verificación

| Comando | Último resultado | Fecha | Notas relevantes |
|---|---|---|---|
| `cmake --preset native-debug` | PASS | 2026-08-24 | Workspace base configurado con GCC 13.3. |
| `cmake --build --preset native-debug --parallel` | PASS | 2026-08-24 | Workspace base; todavía sin targets propios. |
| `ctest --preset native-debug --output-on-failure` | PASS | 2026-08-24 | 17/17 pruebas deterministas superadas. |
| `cmake --preset native-release` | PASS | 2026-08-24 | Configuración Release completada. |
| `cmake --build --preset native-release --parallel` | PASS | 2026-08-24 | Core y CLI compilados. |
| `npm ci` | PASS | 2026-08-24 | 632 paquetes instalados; 0 vulnerabilidades. |
| `npm run lint` | PASS | 2026-08-24 | ESLint sin errores. |
| `npm run typecheck` | PASS | 2026-08-24 | TypeScript sin errores. |
| `npm run test` | PASS | 2026-08-24 | 11 archivos, 62 pruebas superadas. |
| `npm run assets:verify` | PASS | 2026-08-24 | SHA-256 esperado y real coinciden. |
| `npm run check:rapier-thread` | PASS | 2026-08-24 | Import de Rapier permanece solo en Worker. |
| `npm run build` | PASS | 2026-08-24 | Build Next.js de producción completado. |
| `npm audit --omit=dev` | PASS | 2026-08-24 | 0 vulnerabilidades. |
| `find native -type f \\( -name '*.cpp' -o -name '*.hpp' \\) -print0 \| xargs -0 clang-format --dry-run --Werror` | PASS | 2026-08-24 | Formato nativo verificado. |
| `git ls-files 'build/**' 'public/generated/**'` | PASS | 2026-08-24 | Sin artefactos de build rastreados. |
| `git grep -nE 'Qt|VTK|CGAL|FCL|React|Three' -- native/libs/occlusion-core` | PASS | 2026-08-24 | Sin dependencias prohibidas en core. |

### Verificación local de Phase 4.1 (2026-08-25)

- Native Debug configure/build y CTest: PASS, 22/22.
- Native Release configure/build: PASS.
- Vitest: PASS, 13 archivos y 66/66 pruebas.
- `npm ci`, `parity:verify`, lint, typecheck, assets, Rapier-thread, build y audit: PASS (0 vulnerabilidades).
- Formato nativo: PASS.
- Fixture dorado: 16 casos (10 válidos, 6 inválidos), tolerancia `1e-12` metros.
- Rama: `codex/phase-4.1-pose-golden-parity`; inicio: `11ca272b18467ef0a44140b14e075a311dbb6139`.

## 6. Decisiones

- C++20 ofrece tipado fuerte, interoperabilidad con el stack científico y una base portable moderna; véase [ADR-0001](docs/adr/0001-cpp-desktop-architecture.md).
- `double` es obligatorio para evitar pérdidas innecesarias de precisión en datos geométricos y transformaciones.
- FCL se dedicará a consultas de colisión y distancia; CGAL, al procesamiento robusto de mallas.
- VTK y Qt pertenecerán a presentación; no entran en `occlusion-core`, que debe permanecer reutilizable y comprobable sin UI.
- La implementación web no puede eliminarse antes de contar con fixtures dorados, paridad demostrada y una interfaz de escritorio sustituta.

## 7. Bloqueos y riesgos

- Bloqueo de publicación actual (2026-08-25): `git push -u origin codex/phase-4.1-pose-golden-parity` falla exactamente con `fatal: could not read Username for 'https://github.com': No such device or address`; `gh auth status` indica que no hay sesión. Por ello no existe todavía URL real de PR ni resultados CI de Phase 4.1. Esto no restaura el blocker obsoleto de Phase 4, que quedó resuelto mediante PR #10; describe una limitación real de esta sesión nueva.
- Riesgo: la paridad cubre únicamente el contrato de pose descrito; no permite afirmar paridad de colisión, contacto, barrido ni clínica.
- Riesgo: Qt, VTK, CGAL y FCL permanecen diferidos.

## 8. Próxima tarea recomendada

Phase 4.2: port deterministic motion-sweep endpoint and interpolation generation to C++ using new versioned golden fixtures.

## 9. Hitos completados

| Fase | Resultado | Commit o PR | Estado de verificación |
|---|---|---|---|
| Phase 4 | Fundación C++20, core, CLI, pruebas y CI | [PR #10](https://github.com/1816x/Occlusion-Lab/pull/10), merge `11ca272b18467ef0a44140b14e075a311dbb6139` | Completada; 17/17 CTest y 62/62 Vitest locales; CI web remoto exitoso, workflow nativo remoto fallido. |
