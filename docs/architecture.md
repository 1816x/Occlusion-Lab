# Architecture

## Runtime structure

Occlusion Lab Phase 0 is a client-rendered Next.js application with three deliberately separated layers:

1. **Application shell:** Next.js App Router, React, and strict TypeScript provide the page structure and status UI.
2. **Visual layer:** Three.js `WebGLRenderer` renders only synthetic primitives in Phase 0. Three.js official documentation states current `WebGLRenderer` uses WebGL 2 and no longer supports WebGL 1, so Phase 0 treats WebGL2 as mandatory.
3. **Scientific/physics layer:** Rapier 3D WASM is imported and initialized only from `src/workers/rapier.worker.ts`. The main thread talks to this worker through the typed message contract in `src/physics/worker-contract.ts`.

## Threading rule

Scientific calculations must never run on the browser main thread. The main thread may render, display health status, and send/receive structured-cloneable messages. Physics, collision validation, and future numeric analysis belong in dedicated Web Workers.

## LLM boundary

The LLM is not part of the scientific calculation path. Future LLM features, if any, may only explain UI state or documentation and must not create, modify, approve, or replace numerical collision results.

## Asset pipeline

Phase 0 uses synthetic geometry only. Future dental assets should use glTF/GLB as the interchange format. A future pipeline may add Draco mesh compression and KTX2 texture compression after documenting encoder settings, validation steps, licenses, and reproducibility.

## Deterministic fixture

`src/test-fixtures/synthetic-collision.ts` defines a stable synthetic collision fixture for future validation. It uses SI-style units, fixed constants, and no patient-derived geometry.
