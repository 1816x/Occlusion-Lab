# Plan de migración de Occlusion Lab

## 1. Objetivo del proyecto

Occlusion Lab pretende convertirse en una aplicación científica de escritorio para estudiar, de forma reproducible, escenarios sintéticos de oclusión. La migración a C++ busca contratos de dominio estrictos, precisión y una base adecuada para bibliotecas científicas nativas. El producto final será **desktop-first**. La aplicación TypeScript actual se conserva temporalmente como referencia de comportamiento hasta disponer de pruebas de paridad. El proyecto sigue siendo educativo y sintético; no posee validación clínica salvo que una fase futura la documente explícitamente.

## 2. Estado actual

- **Fase actual:** Phase 4: C++ Engine Migration Foundation
- **Estado:** Blocked
- **Rama y commit iniciales:** `main` en `b35bf53cdc5df735d09e65e3cf75a71be69b209c`
- **Rama activa:** `codex/phase-4-cpp-migration-foundation`
- **Pull request:** Pending
- **Última actualización significativa:** 2026-08-24 — verificación local completada; publicación bloqueada por ausencia de credenciales GitHub.

La aplicación web heredada continúa disponible y funcional como referencia. La fundación nativa configura y compila en Debug/Release, `occlusion-core` ofrece unidades fuertes y validación, la CLI ejecuta su autocomprobación determinista y las 17 pruebas CTest pasan. Existe CI multiplataforma, pero no se pudo publicar la rama ni abrir el draft PR porque el entorno no tiene credenciales GitHub.

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
- [ ] Abrir el draft PR requerido con resultados reales y siete commits.

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
| `git push -u origin codex/phase-4-cpp-migration-foundation` | BLOCKED | 2026-08-24 | Error exacto: `fatal: could not read Username for 'https://github.com': No such device or address`; `gh auth status` confirma que no existe sesión. |

## 6. Decisiones

- C++20 ofrece tipado fuerte, interoperabilidad con el stack científico y una base portable moderna; véase [ADR-0001](docs/adr/0001-cpp-desktop-architecture.md).
- `double` es obligatorio para evitar pérdidas innecesarias de precisión en datos geométricos y transformaciones.
- FCL se dedicará a consultas de colisión y distancia; CGAL, al procesamiento robusto de mallas.
- VTK y Qt pertenecerán a presentación; no entran en `occlusion-core`, que debe permanecer reutilizable y comprobable sin UI.
- La implementación web no puede eliminarse antes de contar con fixtures dorados, paridad demostrada y una interfaz de escritorio sustituta.

## 7. Bloqueos y riesgos

- Bloqueo activo: el entorno no tiene credenciales GitHub; `git push -u origin codex/phase-4-cpp-migration-foundation` falla con `fatal: could not read Username for 'https://github.com': No such device or address`, por lo que el draft PR no puede abrirse desde esta sesión.
- Riesgo: todavía no existen fixtures dorados que permitan afirmar paridad entre TypeScript y C++.
- Riesgo: Qt, VTK, CGAL y FCL se han seleccionado pero no se validan ni integran funcionalmente en esta fase.

## 8. Próxima tarea recomendada

Crear el primer fixture dorado versionado de conversión/validación de una pose mandibular desde la implementación TypeScript y ejecutarlo contra `occlusion-core` como inicio de las pruebas de paridad.

## 9. Hitos completados

| Fase | Resultado | Commit o PR | Estado de verificación |
|---|---|---|---|
| Phase 4 | Fundación C++20, core, CLI, pruebas y CI | Draft PR blocked | Verificación local completa; publicación y CI remota bloqueadas. |
