# Plan de migración de Occlusion Lab

## 1. Objetivo del proyecto

Occlusion Lab pretende convertirse en una aplicación científica de escritorio para estudiar, de forma reproducible, escenarios sintéticos de oclusión. La migración a C++ busca contratos de dominio estrictos, precisión y una base adecuada para bibliotecas científicas nativas. El producto final será **desktop-first**. La aplicación TypeScript actual se conserva temporalmente como referencia de comportamiento hasta disponer de pruebas de paridad. El proyecto sigue siendo educativo y sintético; no posee validación clínica salvo que una fase futura la documente explícitamente.

## 2. Estado actual

- **Fase actual:** Phase 4.14: Shared Mesh File Adapter and Optional Desktop Viewer
- **Estado:** Phase 4.13 verificada en CI; Phase 4.14 tiene un primer visor Qt 6/VTK funcional en Linux, con CI y paquete macOS pendientes
- **Rama y commit iniciales:** `main` en `46ffc9524e781fe2e8d8c269027434f422c2abf7`
- **Rama activa:** `work`
- **Phase 4.3:** Completed mediante [PR #13](https://github.com/1816x/Occlusion-Lab/pull/13), merge `da055bc2067b6477eea9efe7abbad0422ed8e9f3`.
- **Última actualización significativa:** 2026-10-28 — Phase 4.14 añade una sesión compartida y un primer visor Qt 6/VTK con importación OBJ, pose, barrido, cámara y export JSON v1.

La aplicación web heredada continúa disponible y funcional como referencia. La base nativa ofrece unidades fuertes y validación; las capas opcionales de colisión/evaluación ejecutan poses y barridos deterministas. Phase 4.7 expone una fixture sintética por CLI; Phase 4.8 exporta resultados versionados; Phase 4.9 añade un parser OBJ estricto. Phase 4.10 une dos ficheros OBJ sintéticos con el evaluador existente. Phase 4.11 verifica su topología y orientación antes de FCL. Phase 4.12 añade inspección geométrica CGAL optativa y Phase 4.13 la expone por un comando CLI explícito, sin alterar la evaluación FCL existente.

## 3. Estado de arquitectura

| Componente | Implementación actual | Implementación objetivo | Estado de migración | Notas |
|---|---|---|---|---|
| Contratos de dominio | TypeScript y nuevos contratos C++ | Biblioteca C++20 `occlusion-core` | Fundación implementada | Sin lógica clínica. |
| Unidades y transformaciones | Tipos fuertes C++ | Tipos fuertes C++ con metros internos | Fundación implementada | Conversión de mm explícita. |
| Ingestión de mallas | Parser OBJ acotado, validador topológico y adaptador público de fichero | Entrada validada antes de CGAL | Fundación implementada | Cierre, orientación y fans de vértices; sin reparación ni detección de autointersecciones. |
| Procesamiento geométrico | Inspección sintética CGAL opcional | CGAL | Primera frontera y CLI implementadas | `--inspect-mesh` detecta autointersecciones y caras degeneradas; no repara. |
| Detección de colisiones | Worker web/Rapier y biblioteca FCL opcional | FCL | Fundación implementada | Sin afirmar igualdad de manifolds entre motores. |
| Análisis de barrido de movimiento | Web existente y evaluación C++ | Motor C++ | Barrido y resumen implementados | Paridad semántica del resumen cubierta por fixture versionada. |
| Renderer | Three.js y primer visor VTK | VTK | Visor sintético implementado localmente | Renderiza mallas, ejes y contactos normalizados; no forma parte del core. |
| UI de escritorio | Primer visor Qt 6 | Qt 6 | Implementada localmente; CI macOS pendiente | Importación sintética, errores, pose y barrido asíncronos, selección de frame y export JSON v1. |
| CLI | `occlusion-cli` | `occlusion-cli` extensible | Evaluación OBJ e inspección optativa implementadas | Evaluación requiere FCL; inspección CGAL puede compilarse sin FCL. |
| Pruebas | Vitest/web y GoogleTest nativo | GoogleTest + pruebas web | Fundación implementada | Se mantiene la suite heredada. |
| CI | Web y workflow nativo | CI web y nativa multiplataforma | Implementado y verificado | Web `40009148955` y matriz nativa `40008247380` pasaron; los cambios posteriores requieren su propia CI. |
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
- CI remota Ubuntu, Windows y macOS: Unverified; no se presenta como aprobada.

## 7. Bloqueos y riesgos

- La primera ejecución del formato nativo detectó archivos sin formatear; se aplicó `clang-format` y la repetición pasó antes del commit final.
- Riesgo: la paridad de resúmenes cubre solo la reducción de frames ya evaluados; no permite afirmar igualdad de manifolds Rapier/FCL ni paridad clínica.
- Riesgo: la estabilidad semántica FCL se prueba localmente, pero los manifolds exactos no son portables ni se consideran contrato.
- CI de Phase 4.13: web `40005882371` y matriz nativa `40005882067` completadas correctamente en `43c2213`, incluidos Windows collision, CGAL Ubuntu y ASan+UBSan. Los fallos anteriores de toolchain, FCL Debug y CRLF quedaron resueltos. El adaptador de Phase 4.14 también pasó web `40007105819` y matriz nativa `40007106060` completos en `0fe8d4e`; el último ajuste de espera de red CGAL pasó web `40008247367` y matriz nativa `40008247380` en `18e04d0`.
- Diferidos: implementación de UI desktop y validación clínica; no se añadieron fuerzas ni presiones. El directorio OBJ debe ser confiable frente a modificaciones concurrentes: los controles portables de filesystem no son una sandbox para procesos hostiles. La inspección CGAL en la evaluación FCL sigue siendo explícita y optativa.
- Los PR #15, #16 y #21 fueron reconciliados y figuran como `MERGED` en GitHub. Sus contenidos funcionales ya estaban integrados en `main` mediante PR #17 y trabajo posterior; se trasladaron los ajustes de CI/Eigen aún útiles de #16 y se fusionaron las tres ramas con conservación explícita del árbol actual para evitar retirar export v1 y Phase 4.9.

## 8. Próxima tarea recomendada

Phase 4.14: verificar el nuevo visor Qt 6/VTK en CI macOS, empaquetar un `.app` autocontenido y probarlo como usuario final. El visor local ya importa OBJ, renderiza mallas y contactos, evalúa pose y barrido sin bloquear la UI y exporta JSON v1. Faltan pruebas de interacción, de resultados asíncronos obsoletos y de paridad visual/export antes de distribuir la beta. Conservar la referencia web hasta demostrar paridad de los flujos requeridos.

Para la beta macOS sintética, los gates pendientes son: (1) compilación y smoke de render en CI macOS; (2) pruebas de interacción, errores, resultados asíncronos obsoletos y paridad de export; (3) `.app` autocontenido probado en otro Mac. Objetivo provisional: 20 de noviembre de 2026, sujeto al resultado del primer build macOS y del empaquetado. No se considera una beta distribuible hasta pasar estos gates.

## 9. Hitos completados

| Fase | Resultado | Commit o PR | Estado de verificación |
|---|---|---|---|
| Phase 4 | Fundación C++20, core, CLI, pruebas y CI | [PR #10](https://github.com/1816x/Occlusion-Lab/pull/10), merge `11ca272b18467ef0a44140b14e075a311dbb6139` | Completada; 17/17 CTest y 62/62 Vitest locales; CI web remoto exitoso, workflow nativo remoto fallido. |
| Phase 4.1 | Fixtures dorados de pose y paridad del contrato C++ | [PR #11](https://github.com/1816x/Occlusion-Lab/pull/11), merge `39e206e48599333d2fa76947352e09be04c39dee` | Completada; 22/22 CTest y 66/66 Vitest locales; GitHub CI del head completado correctamente. |
| Phase 4.2 | Paridad determinista de generación de barridos: 20 casos y 432 frames | [PR #12](https://github.com/1816x/Occlusion-Lab/pull/12), merge `5607b26f856481f4aacb36139ec9837f125e80c5` | Completada; 27/27 CTest y 85/85 Vitest locales; CI web verificada correctamente; CI nativa remota no verificada. |
| Phase 4.3 | Fundación de colisión FCL de producción | [PR #13](https://github.com/1816x/Occlusion-Lab/pull/13), merge `da055bc2067b6477eea9efe7abbad0422ed8e9f3` | Completed; 91/91 Vitest, 44/44 CTest (27 core + 17 collision), ASan y UBSan locales PASS; CI nativa remota Unverified. |
| Phase 4.4 | Evaluación nativa single-pose determinista y normalización acotada | [PR #14](https://github.com/1816x/Occlusion-Lab/pull/14), merge `46ffc95` | Completed localmente; 63/63 CTest (19 nuevas), 91/91 Vitest PASS; ASan+UBSan PASS; CI remota Unverified. |
| Phase 4.5 | Barridos nativos evaluados, reutilización de BVH y resúmenes deterministas | [PR #17](https://github.com/1816x/Occlusion-Lab/pull/17), merge `2ec46d13a16ad197a397a90bae6d6e29c7016de1` | Completada; 67/67 CTest y 91/91 Vitest locales PASS; CI remota Unverified. |
| Phase 4.6 | Fixture versionada de paridad semántica para resúmenes evaluados | [PR #18](https://github.com/1816x/Occlusion-Lab/pull/18), merge `d3f5f9e` | Completada localmente; 8 casos engine-neutral, 70/70 CTest; CI remota Unverified. |
| Phase 4.7 | Evaluación de pose y barrido sobre fixture sintética mediante CLI JSON | [PR #19](https://github.com/1816x/Occlusion-Lab/pull/19), merge `e226060` | Completada localmente; 74/74 CTest y 94/94 Vitest; CI remota Unverified. |
| Phase 4.8 | Export JSON v1 completo, determinista y engine-neutral | Commit `24a4528` | Completada localmente; ejemplos dorados y política de compatibilidad incluidos; CI remota Unverified. |
| Phase 4.9 | Ingestión OBJ sintética acotada y errores estructurados | Merge #22, `2fb68c9` | Completada localmente; 7 pruebas específicas, 34/34 CTest core y 85/85 CTest con colisión; CI remota nativa no verificada. |
| Phase 4.10 | Adaptador CLI para dos ficheros OBJ sintéticos y evaluación existente | `7e22412` | Implementada y publicada en `main`; CI web remota PASS. CI nativa parcialmente completada; tres jobs sin runner. |
| Phase 4.11 | Validación topológica y orientación OBJ antes de FCL | `5c98b0c` | Publicada en `main`; 39/39 CTest core, 100/100 CTest colisión y 100/100 ASan+UBSan PASS locales; CI remota pendiente. |
| Phase 4.12 | Inspección CGAL optativa, conversión comprobada y predicados de mallas sintéticas | `b2483d4`, ajuste CI `aec6975` | Publicada en `main`; geometry Ubuntu y los tres core PASS; Ubuntu/macOS collision y Ubuntu sanitizer PASS; Windows collision build FAIL. |
| Phase 4.13 | CLI de inspección JSON v1 y contrato de visor Qt 6/VTK | `a7aaba4`–`43c2213` | Completada; 39/39 core, 49/49 geometry, 100/100 collision, 110/110 combinado y 100/100 sanitizer CTest locales PASS; web y matriz nativa CI remota PASS. |
| Phase 4.14 | Adaptador OBJ compartido y primer visor opcional | `0fe8d4e`, `df87f18` (en curso) | Adaptador y sesión verificados en CI multiplataforma. Primer visor Qt 6/VTK: 107/107 CTest locales PASS con render bajo Xvfb; CI macOS y paquete pendientes. |

### Verificación local de Phase 4.13 (2026-10-23)

- `--inspect-mesh <directory> <mesh.obj>` está disponible en builds con geometría sin requerir FCL. Devuelve JSON v1 de éxito o error estructurado con código, campo, línea y mensaje; usa los mismos límites, rechazo de symlink/traversal, parser y validación topológica que la evaluación existente. Su contrato está en [native-mesh-inspection-v1.md](docs/native-mesh-inspection-v1.md).
- Fixture nueva de dos cajas cerradas que se intersecan; pruebas del comando y su adaptador cubren caja válida, autointersección, OBJ malformado, malla abierta, traversal y fichero ausente. Una prueba integral adicional confirma stdout JSON, códigos de salida y stderr de la CLI para éxito, errores de malla y argumentos incorrectos. El adaptador no modifica el OBJ ni conecta CGAL implícitamente a `--evaluate-meshes`.
- El visor Qt 6/VTK todavía no se compila; su frontera de datos, errores, threading, pose, barrido, export y criterios de paridad se especifican en [desktop-viewer-phase-4.13.md](docs/desktop-viewer-phase-4.13.md).
- Core Debug 39/39, Geometry Debug 49/49, Collision Debug 100/100, Geometry+Collision Debug 110/110 y Collision ASan+UBSan 100/100 CTest PASS locales; builds core/collision Release PASS. Web: `npm ci`, lint, typecheck, paridad, 94/94 Vitest, assets, build y smoke HTTP de producción PASS. Las actualizaciones compatibles de Next.js 16.4.3, Vitest 4.1.12 y DOMPurify 3.4.17 dejaron `npm audit` completo en 0 vulnerabilidades. El fallo remoto Windows collision de Phase 4.12 se confirmó; los demás jobs nativos y web de ese head pasaron.
- La primera CI de Phase 4.13 (`39993524643`) encontró fallo 128 en `Bootstrap pinned vcpkg (Unix)` antes de compilar el proyecto. Un fetch shallow del mismo SHA con Git HTTP/1.1 y `vcpkg install --dry-run` resolvieron el manifiesto localmente; el workflow ahora usa ese mecanismo con tres intentos también en Windows. Las anotaciones Windows identificaron un enlace MinGW/MSVC incompatible; Visual Studio no está instalado en el runner. El primer triplet MinGW alcanzó la compilación de FCL, pero su Debug produjo objetos demasiado grandes. El overlay `x64-mingw-dynamic-release-deps` mantiene el proyecto Debug con dependencias Release. El manifiesto confirma que GTest y nlohmann-json se instalan para el target MinGW, no para el host MSVC. `40004857801` localizó fallos CTest en la comparación binaria de fixtures JSON: el serializador emite LF y Git les aplicó CRLF en Windows. `.gitattributes` fija LF. La siguiente CI nativa `40005882067` y web `40005882371` pasaron completas.

### Verificación local de Phase 4.14 (2026-10-23)

- `load_synthetic_obj` y su error estructurado son públicos en `occlusion-mesh-io`, sin enlazar FCL ni CGAL. Los consumidores CLI de evaluación e inspección comparten la misma política de fichero.
- Tres pruebas directas cubren caja cerrada, nombre inválido/fichero ausente y topología abierta en un build core-only. Core Debug 42/42, Geometry+Collision Debug 113/113 y Collision ASan+UBSan 103/103 CTest PASS; builds core y collision Release PASS. Web `40007105819` y los ocho jobs de la matriz nativa `40007106060` PASS en `0fe8d4e`; la ejecución final en `18e04d0` también pasó web `40008247367` y los ocho jobs nativos `40008247380`. El visor opcional queda pendiente.
- `MeshEvaluationSession` conserva las mismas mallas validadas que compilan el evaluador FCL; la CLI reutiliza esta sesión. La prueba elimina los ficheros fuente tras importar y confirma que pose y barrido siguen funcionando sobre los datos propios. La nueva CI de este cambio sigue pendiente.
- La sesión y CLI compilan en Release; Collision Debug 105/105, Geometry+Collision Debug 115/115 y Collision ASan+UBSan 105/105 CTest PASS el 2026-10-28.
- Web `40711050926` y matriz nativa `40711050847` PASS en `df87f18`. Un primer visor opcional Qt 6/VTK compila en Linux con dependencias locales extraídas a `/workspace/occlusion-tools/sysroot`: importación de dos OBJ, escena VTK, pose/barrido asíncronos, selección de frame, cámara y export con el serializador existente. Smoke de render y rechazo de directorio ausente PASS; preset `desktop-debug` 107/107 CTest PASS bajo Xvfb. Se inspeccionó la captura local: las dos mallas y ejes son visibles. El job macOS todavía no se ha verificado.

### Verificación local de Phase 4.12 (2026-10-22)

- `occlusion-geometry` es una biblioteca independiente habilitada solo por `OCCLUSION_ENABLE_GEOMETRY` y la feature vcpkg `geometry`. CGAL queda privado; `occlusion-core`, `occlusion-mesh-io` y builds por defecto siguen libres de CGAL.
- La API inspecciona finitud, topología cerrada y orientada, conversión a `CGAL::Surface_mesh`, caras de área nula y autointersecciones, sin modificar los datos. Devuelve código, campo y mensaje propios del proyecto.
- Pruebas sintéticas: caja cerrada válida, caja abierta, cara degenerada, dos shells cerrados que se intersecan y coordenada no finita. `geometry-debug`: 44/44 CTest PASS. El primer job remoto de geometría falló durante la configuración mediante vcpkg; con `libcgal-dev` en Ubuntu, instalación, configuración, compilación y pruebas remotas pasaron en la ejecución `39817385244`. La feature vcpkg `geometry` sigue disponible para builds que la utilicen.
- Builds locales sin CGAL: core Debug 39/39 CTest y collision Debug 100/100 CTest PASS. CI web del commit `aec6975`: PASS. En la matriz nativa, todos los jobs salvo Windows collision pasaron; Windows falló en build.
- La CLI existente no llama aún a CGAL. El módulo detecta defectos geométricos cuando un consumidor invoca explícitamente la nueva API; no constituye validación clínica ni reparación.

### Verificación local de Phase 4.11 (2026-10-20)

- Se eliminaron las ocho ramas remotas `codex/*` después de comprobar que todos sus commits ya eran ancestros de `main`; la única rama remota restante es `main`.
- El validador puro de `occlusion-mesh-io` exige dos caras con aristas de sentidos opuestos, un único fan por vértice y volumen positivo por cada shell. Los errores estructurados de topología indican arista, vértice o malla con línea 0. No detecta autointersecciones.
- La CLI ejecuta esa validación antes de convertir índices y compilar los BVH FCL. Las pruebas cubren caja cerrada, malla abierta, arista no manifold, vertex fan desconectado, orientación inconsistente, shell invertido, vértice huérfano y rechazo CLI de malla abierta. La validación geométrica FCL sigue rechazando triángulos degenerados.
- Native core Debug: 39/39 CTest PASS. Native collision Debug y ASan+UBSan: 100/100 CTest PASS cada uno. Native core/collision Release: build PASS. Formato C++ PASS.
- Web: lint, typecheck, Vitest, assets, aislamiento Rapier, build y audit de producción PASS; 0 vulnerabilidades. La propuesta de arquitectura CGAL/Qt 6/VTK queda en `docs/native-desktop-roadmap.md`.

### Verificación local de Phase 4.10 (2026-10-20)

- El adaptador CLI recibe un directorio explícito y dos nombres `.obj` directos; rechaza traversal, enlaces simbólicos, archivos ausentes/no regulares y entradas superiores a 16 MiB antes del parser.
- El parser mantiene sus límites de vértices/triángulos y mensajes de error por línea. La conversión a índices `uint32_t` es comprobada. `PoseEvaluator::create` valida geometría y compila dos BVH para poses y barridos.
- `occlusion-cli --version` se alinea con la versión CMake 0.5.0. El workflow nativo incluye ahora los cambios en fixtures de pose, generación de barridos y OBJ.
- Web: lint, typecheck, 94/94 Vitest, paridad, assets, build de producción, smoke HTTP de página/fixture y audit de producción (0 vulnerabilidades) PASS.
- Native core Debug: configure/build y 34/34 CTest PASS. Native collision Debug: configure/build y 94/94 CTest PASS, con pruebas nuevas de rutas, symlinks, límites, parser, geometría y CLI. Native core y collision Release: configure/build PASS. ASan+UBSan: configure/build y 94/94 CTest PASS.
- Se incorporó la inicialización de vcpkg por plataforma, triplets explícitos, cache y bootstraps separados de PR #16; la versión Eigen se comprueba después de encontrar el paquete para admitir 3.4 o superior. En CI nativa remota pasaron Ubuntu core/collision/sanitizer y macOS collision; Windows core/collision y macOS core no fueron adquiridos por runners alojados.
- GitHub Actions web (`.github/workflows/ci.yml`) para los pushes de Phase 4.10 y la actualización de PLAN: PASS.
- Los antiguos PR #15, #16 y #21 se compararon con `main`: sus ramas precedían funcionalidades posteriores. Se añadieron sus heads como padres de merge conservando el árbol verificado actual; GitHub muestra los tres PR como `MERGED`. El push a `main` y la lectura del HEAD remoto se comprobaron. Su historial quedó incluido sin aplicar sus versiones antiguas de los archivos.

### Verificación local de Phase 4.9 (2026-10-07)

- `occlusion-mesh-io` acepta únicamente comentarios, líneas vacías, vértices XYZ en metros y caras triangulares con índices positivos a vértices precedentes.
- Límites por defecto: 16 MiB de entrada, 1.000.000 de vértices y 2.000.000 de triángulos.
- Errores estructurados publican código, línea, campo y mensaje; nunca se devuelve una malla parcial.
- Native core Debug: configure/build y 34/34 CTest PASS (7 pruebas nuevas).
- Native collision/evaluation Debug: configure/build y 85/85 CTest PASS.
- Web: Next.js 16.4.0, Vitest 4.1.11, lint, typecheck, 94/94 Vitest, build y audit de producción PASS (0 vulnerabilidades).
- No se añadieron filesystem, datos clínicos, inferencia de unidades, triangulación, reparación, CGAL ni FCL al parser.
- CI remota Ubuntu, Windows y macOS: Unverified.

### Alineación de PR de Phase 4.5 (2026-10-05)

- PR #15 fue la propuesta inicial de barridos nativos evaluados y PR #16 combinó correcciones de toolchain con ese trabajo.
- Ambos quedaron superados funcionalmente por la integración final de Phase 4.5 en PR #17; posteriormente se reconciliaron en el historial de `main` sin sustituir el árbol moderno.
- Phase 4.7 quedó integrada posteriormente mediante PR #19. La rama `work` ya contiene ambas líneas de trabajo.
- El filtro de paths de CI nativa ahora incluye la fixture de resumen evaluado, corrigiendo la omisión que podía evitar la ejecución del workflow.

### Inicio local de Phase 4.8 (2026-10-05)

- Export JSON v1 con identificador de schema, unidades explícitas y orden estable.
- Pose: incluye pose solicitada, transformación aplicada, mediciones, intersección y contactos normalizados.
- Barrido: incluye preset, pose final, resumen y todos los frames completos.
- El contrato declara explícitamente que no garantiza paridad de manifolds entre motores.
- Ejemplos dorados engine-neutral de pose y barrido completo congelan el contrato byte a byte.
- La política de compatibilidad exige nueva versión para cambios de campos, orden, unidades, nulabilidad, representación o semántica.
- Native collision/evaluation Debug: configure/build y 78/78 CTest PASS.
- CI remota Ubuntu, Windows y macOS: Unverified; es el único gate pendiente de Phase 4.8.

### Verificación local de Phase 4.7 (2026-09-29)

- CLI con colisión: pose y barrido producen JSON versionado válido sobre `synthetic-boxes-v1`.
- Native core Debug: configure/build y 27/27 CTest PASS.
- Native collision/evaluation Debug: configure/build y 74/74 CTest PASS (cuatro pruebas CLI nuevas).
- Native collision/evaluation Release: configure/build PASS.
- Web: fixtures de paridad, lint, typecheck, 94/94 Vitest y aislamiento de Rapier PASS.
- Formato nativo: PASS. CI remota: Unverified y no se presenta como aprobada.

### Verificación local de Phase 4.6 (2026-09-29)

- Fixture de resumen evaluado: 8 casos semánticos; baseline fijo `2ec46d13a16ad197a397a90bae6d6e29c7016de1`.
- Native collision/evaluation Debug: configure/build y 70/70 CTest PASS.
- Native collision/evaluation Release: configure/build PASS.
- Web: paridad canónica, lint, typecheck y 94/94 Vitest PASS; la regla de aislamiento de Rapier también pasó.
- Formato nativo: PASS. CI remota: Unverified y no se presenta como aprobada.

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
