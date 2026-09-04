# Plan de migración de Occlusion Lab

## 1. Objetivo del proyecto

Occlusion Lab pretende convertirse en una aplicación científica de escritorio para estudiar, de forma reproducible, escenarios sintéticos de oclusión. La migración a C++ busca contratos de dominio estrictos, precisión y una base adecuada para bibliotecas científicas nativas. El producto final será **desktop-first**. La aplicación TypeScript actual se conserva temporalmente como referencia de comportamiento hasta disponer de pruebas de paridad. El proyecto sigue siendo educativo y sintético; no posee validación clínica salvo que una fase futura la documente explícitamente.

## 2. Estado actual

- **Fase actual:** Phase 4.5: Native Evaluated Motion Sweeps and Deterministic Summaries
- **Estado:** In progress
- **Commit base esperado:** `46ffc9524e781fe2e8d8c269027434f422c2abf7`
- **Rama activa:** `codex/implement-phase-4.5-evaluated-motion-sweeps`
- **PR en curso:** https://github.com/1816x/Occlusion-Lab/pull/15
- **Head publicado al iniciar la reparación:** `e6c46f5368641d12d6676447b5338ecc0b30ea0d`
- **Estado CI previo a la reparación:** Web PASS; Core PASS en Ubuntu, Windows y macOS; Collision + evaluation FAIL en Ubuntu, Windows y macOS; Sanitizers Ubuntu FAIL.
- **Causas diagnosticadas:** incompatibilidad de versión solicitada Eigen 3.4 frente al paquete Eigen 5.0.1; feature `collision` de vcpkg no activada mediante CMake; selección MinGW incompatible con el triplet MSVC `x64-windows`.
- **Phase 4.3:** Completed mediante [PR #13](https://github.com/1816x/Occlusion-Lab/pull/13), merge `da055bc2067b6477eea9efe7abbad0422ed8e9f3`.
- **Phase 4.4:** Completed mediante [PR #14](https://github.com/1816x/Occlusion-Lab/pull/14), head `3d3b3de6cbded3ecaf6b2d61c53ebdb4b84fedca`, merge `46ffc9524e781fe2e8d8c269027434f422c2abf7`.
- **Última actualización significativa:** 2026-09-02 — Phase 4.4 reconciliada como completada y Phase 4.5 iniciada desde el último `main`.

La aplicación web heredada continúa disponible y funcional como referencia. La fundación nativa configura y compila en Debug/Release, `occlusion-core` ofrece unidades fuertes y validación, la CLI ejecuta su autocomprobación determinista y las 17 pruebas CTest pasan. Phase 4 se completó mediante el PR [#10](https://github.com/1816x/Occlusion-Lab/pull/10), mergeado como `11ca272b18467ef0a44140b14e075a311dbb6139`. La verificación local registró 17/17 CTest y 62/62 Vitest. El CI web remoto fue exitoso; el workflow nativo remoto falló y no se presenta como verificado.

## 3. Estado de arquitectura

| Componente | Implementación actual | Implementación objetivo | Estado de migración | Notas |
|---|---|---|---|---|
| Contratos de dominio | TypeScript y nuevos contratos C++ | Biblioteca C++20 `occlusion-core` | Fundación implementada | Sin lógica clínica. |
| Unidades y transformaciones | Tipos fuertes C++ | Tipos fuertes C++ con metros internos | Fundación implementada | Conversión de mm explícita. |
| Procesamiento geométrico | Flujo web existente | CGAL | Futuro | CGAL y validación clínica permanecen diferidos. |
| Detección de colisiones | Worker web/Rapier y FCL nativo | FCL | Implementado | Fundación FCL y evaluación single-pose implementadas. |
| Evaluación de pose individual | Worker web/Rapier y `occlusion-evaluation` | Motor C++ | Implementado | Contrato nativo engine-neutral sobre FCL. |
| Análisis de barrido de movimiento | Web existente | Motor C++ | Fase actual | Phase 4.5 implementa barridos evaluados. |
| Renderer | Three.js | VTK | Futuro | No forma parte del core. |
| UI de escritorio | No existe | Qt 6 | Futuro | Arquitectura desktop-first. |
| Validación clínica | No existe | Investigación futura | Futuro | No se realizan afirmaciones clínicas. |
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

### Verificación local de Phase 4.4 (2026-09-01)

- Fixture de normalización: 10 casos; baseline fijo `da055bc2067b6477eea9efe7abbad0422ed8e9f3`.
- Native core Debug: configure/build y 27/27 CTest PASS.
- Native collision/evaluation Debug: configure/build y 63/63 CTest PASS (19 pruebas nuevas).
- Native core y collision/evaluation Release: configure/build PASS.
- ASan+UBSan collision/evaluation: configure/build y 63/63 CTest PASS.
- Web: `npm ci`, parity, lint, typecheck, 91/91 Vitest, assets, Rapier-thread, build y audit PASS (0 vulnerabilidades).
- Web GitHub Actions: PASS. Native workflow [run 33525752697](https://github.com/1816x/Occlusion-Lab/actions/runs/33525752697): **FAIL** antes de crear jobs (cero jobs), no `Unverified`.

## 7. Bloqueos y riesgos

- Causa exacta del fallo nativo 33525752697: el workflow evaluaba `${{ runner.temp }}` en `jobs.<job_id>.env`, un ámbito donde el contexto `runner` todavía no está disponible; GitHub rechazó el workflow antes de crear jobs. Phase 4.5 traslada esas rutas a pasos ejecutables que publican `$RUNNER_TEMP` mediante `$GITHUB_ENV`.
- La primera ejecución del formato nativo detectó archivos sin formatear; se aplicó `clang-format` y la repetición pasó antes del commit final.
- Riesgo: la paridad cubre únicamente poses, transformaciones y generación determinista de barridos; no permite afirmar paridad de colisión, contacto, barrido ni clínica.
- Riesgo: la estabilidad semántica FCL se prueba localmente, pero los manifolds exactos no son portables ni se consideran contrato.
- Riesgo: CI remota multiplataforma permanece Unverified hasta inspeccionar los jobs del draft PR.
- Diferidos: barridos evaluados completos, UI desktop y validación clínica; no se añadieron fuerzas ni presiones.

## 8. Próxima tarea recomendada

Phase 4.6: expose deterministic native pose and sweep evaluation through the CLI with versioned JSON output.

## 9. Hitos completados

| Fase | Resultado | Commit o PR | Estado de verificación |
|---|---|---|---|
| Phase 4 | Fundación C++20, core, CLI, pruebas y CI | [PR #10](https://github.com/1816x/Occlusion-Lab/pull/10), merge `11ca272b18467ef0a44140b14e075a311dbb6139` | Completada; 17/17 CTest y 62/62 Vitest locales; CI web remoto exitoso, workflow nativo remoto fallido. |
| Phase 4.1 | Fixtures dorados de pose y paridad del contrato C++ | [PR #11](https://github.com/1816x/Occlusion-Lab/pull/11), merge `39e206e48599333d2fa76947352e09be04c39dee` | Completada; 22/22 CTest y 66/66 Vitest locales; GitHub CI del head completado correctamente. |
| Phase 4.2 | Paridad determinista de generación de barridos: 20 casos y 432 frames | [PR #12](https://github.com/1816x/Occlusion-Lab/pull/12), merge `5607b26f856481f4aacb36139ec9837f125e80c5` | Completada; 27/27 CTest y 85/85 Vitest locales; CI web verificada correctamente; CI nativa remota no verificada. |
| Phase 4.3 | Fundación de colisión FCL de producción | [PR #13](https://github.com/1816x/Occlusion-Lab/pull/13), merge `da055bc2067b6477eea9efe7abbad0422ed8e9f3` | Completed; 91/91 Vitest, 44/44 CTest (27 core + 17 collision), ASan y UBSan locales PASS; CI nativa remota Unverified. |
| Phase 4.4 | Evaluación nativa single-pose determinista y normalización acotada | [PR #14](https://github.com/1816x/Occlusion-Lab/pull/14), head `3d3b3de6cbded3ecaf6b2d61c53ebdb4b84fedca`, merge `46ffc9524e781fe2e8d8c269027434f422c2abf7` | Completed; 63/63 CTest (19 nuevas), 91/91 Vitest; ASan+UBSan locales y Web Actions PASS; native run 33525752697 FAIL antes de crear jobs. |

### Verificación local de Phase 4.2 (2026-08-27)

- Fixture de barrido: 20 casos y 432 frames; baseline fijo `39e206e48599333d2fa76947352e09be04c39dee`.
- Vitest: PASS, 15 archivos y 85/85 pruebas.
- CTest Debug: PASS, 27/27 pruebas; configure/build Debug y configure/build Release: PASS.
- `npm ci`, `parity:verify`, lint (0 errores, una advertencia heredada), typecheck, assets, Rapier-thread, build y audit: PASS; 0 vulnerabilidades.
- CI web: verificada correctamente. CI nativa remota (Ubuntu/Windows/macOS): no verificada y no se presenta como aprobada.

### Verificación local de Phase 4.3 (2026-08-30)

- Fixture de colisión single-pose: 7 casos sobre dos cajas cerradas; baseline `5607b26f856481f4aacb36139ec9837f125e80c5`.
- Vitest: PASS, 16 archivos y 91/91 pruebas.
- Core CTest Debug sin FCL: PASS, 27/27. Collision CTest Debug: PASS, 17/17 adicionales (44/44 total).
- Collision Release: configure/build PASS. ASan+UBSan: 44/44 PASS, sin fugas ni comportamiento indefinido reportado.
- `npm ci`, las tres fixtures de paridad, lint, typecheck, assets, Rapier-thread, build y audit: PASS; 0 vulnerabilidades.
- CI remota Ubuntu, Windows y macOS: no verificada hasta que el draft PR publique y ejecute todos los jobs; no se presenta como aprobada.
- Riesgo restante: los resultados multiplataforma y el target FCL exportado por el vcpkg fijado deben confirmarse en el PR. La paridad excluye manifolds y barridos evaluados completos.

### Verificación local de Phase 4.5 (2026-09-02)

- Fixture de resumen: 12 casos, incluidos 2, 31 y 61 frames; baseline `46ffc9524e781fe2e8d8c269027434f422c2abf7`.
- Native core Debug: 27/27 CTest PASS. Collision/evaluation Debug: 75/75 PASS (12 pruebas nuevas).
- Native core y collision/evaluation Release: configure/build PASS. ASan+UBSan: 75/75 PASS.
- Web: `npm ci`, parity, lint, typecheck, 91/91 Vitest, assets, Rapier-thread, build y audit PASS (0 vulnerabilidades).
- Barridos secuenciales: exactamente dos BVH, hasta 32 contactos/frame y 1.952 en 61 frames.
- Paridad limitada al resumen, no manifolds Rapier/FCL. CLI, UI desktop, importación de mallas y validación clínica están diferidos.

### Reparación CI de Phase 4.5 (2026-09-04)

- Se mantuvo el PR draft [#15](https://github.com/1816x/Occlusion-Lab/pull/15) y su historia publicada; no se inició Phase 4.6.
- Eigen: FetchContent resolvió 3.4.0; el manifiesto vcpkg fijado instaló y configuró Eigen 5.0.1 correctamente tras separar la comprobación mínima `>= 3.4.0` de las reglas de compatibilidad major del paquete.
- vcpkg: los presets de colisión activan `VCPKG_MANIFEST_FEATURES=collision`. La instalación local confirmó `fcl` 0.7.0#5 y sus dependencias `ccd` y `octomap`, con GCC 13.3.0 y triplet `x64-linux`.
- Native core Debug: configure/build y 27/27 CTest PASS. Native core Release: configure/build PASS; 0 pruebas registradas porque el preset desactiva tests.
- Collision/evaluation Debug: configure/build y 75/75 CTest PASS. Collision/evaluation Release: configure/build PASS; 0 pruebas registradas porque el preset desactiva tests.
- Sanitizers: configure/build y 75/75 CTest PASS con ASan+UBSan.
- Web: `npm ci`, parity, lint, typecheck, 91/91 Vitest, assets, Rapier-thread, build y audit PASS; 0 vulnerabilidades.
- Formato nativo, fixture dorado sin cambios durante la reparación, ausencia de outputs rastreados y aislamiento de dependencias de `occlusion-core`: PASS.
- Verificación remota: **BLOCKED en este entorno antes del push**. `git push origin HEAD:codex/implement-phase-4.5-evaluated-motion-sweeps` falló porque no hay credenciales GitHub (`could not read Username`). Por tanto, los resultados remotos reparados permanecen pendientes y no se afirma que los checks estén verdes.
