# ADR-0001: Arquitectura C++ desktop-first

- **Estado:** Aceptada
- **Fecha:** 2026-08-24

## Contexto

La implementación browser-first en TypeScript ha permitido validar flujos educativos, interacción 3D y comportamiento visible con rapidez. El navegador, el modelo de Worker y el ecosistema JavaScript también introducen límites técnicos para un motor científico: contratos numéricos menos estrictos, fronteras entre hilos y representación, y una integración menos directa con bibliotecas nativas especializadas. Estas limitaciones no invalidan el trabajo existente; lo convierten en una referencia de comportamiento valiosa durante una migración incremental.

Occlusion Lab procesa actualmente escenarios educativos y sintéticos. No existe validación clínica, por lo que la arquitectura y sus resultados no deben presentarse como herramienta clínica.

## Decisión

Se adopta C++20 como lenguaje del motor y una arquitectura **desktop-first**. El motor científico se separará estrictamente de las capas de presentación:

- **C++20**: contratos, validación, transformaciones y futura coordinación del análisis.
- **Eigen**: álgebra lineal en el núcleo.
- **CGAL**: futuro procesamiento y reparación de mallas.
- **FCL**: futuras consultas de colisión y distancia.
- **VTK**: futuro pipeline de visualización científica.
- **Qt 6**: futura aplicación de escritorio y composición de UI.
- **CMake/vcpkg**: construcción portable y dependencias reproducibles.
- **GoogleTest/CTest**: pruebas nativas deterministas y ejecución uniforme.

## Política de precisión y unidades

Todos los valores científicos usarán `double`. La unidad interna de longitud será el metro. Metros y milímetros serán tipos diferentes y sus conversiones requerirán llamadas explícitas. Todo valor de entrada deberá ser finito: NaN e infinitos producirán errores estructurados, nunca clamps silenciosos.

## Fronteras de dependencias

`occlusion-core` no dependerá de Qt, VTK, CGAL, FCL, filesystem, red ni rendering. Eigen será su única dependencia externa de ejecución en la fundación. La CLI y futuras presentaciones consumirán exclusivamente headers públicos. Las dependencias externas no heredarán las políticas de warnings del código propio.

CGAL y FCL se incorporarán mediante módulos con responsabilidades concretas; VTK y Qt permanecerán fuera del dominio. Esta separación permite probar contratos sin instalar el stack de escritorio completo.

## Estrategia de migración

1. Establecer workspace, contratos de dominio, unidades, CLI y CI.
2. Extraer fixtures dorados deterministas desde casos de la aplicación web.
3. Migrar por capacidades y exigir paridad observable antes de sustituir cada ruta.
4. Incorporar geometría, colisión y análisis detrás de interfaces comprobables.
5. Construir renderer y UI desktop cuando el motor tenga cobertura suficiente.

La aplicación TypeScript permanece sin reescritura como referencia temporal. No se afirma paridad durante esta fase.

## Consecuencias y compromisos

La decisión mejora el tipado, la separación de responsabilidades y el acceso al ecosistema científico nativo, a cambio de mayor complejidad de compilación, distribución multiplataforma y mantenimiento de dos implementaciones durante la transición. Las dependencias pesadas serán opcionales para que el ciclo básico continúe siendo rápido.

## Condiciones para retirar la aplicación web

La implementación heredada solo podrá retirarse cuando existan fixtures dorados representativos, pruebas automatizadas que demuestren paridad del motor C++, una interfaz desktop que sustituya sus flujos necesarios, documentación de cualquier desviación aceptada y una decisión explícita posterior. La validación clínica, si alguna vez se busca, requerirá un proceso separado y no se infiere de la paridad técnica.
