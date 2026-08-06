# Architecture

## Runtime structure

Occlusion Lab is a client-rendered Next.js application with three separated layers:

1. **Application shell:** Next.js App Router, React, and strict TypeScript provide the page structure, scenario controls, and status UI.
2. **Visual/asset layer:** `predev` and `prebuild` deterministically generate the ignored build artifact `public/generated/opposing-occlusal-surfaces.glb`. Three.js uses `GLTFLoader` to load `/generated/opposing-occlusal-surfaces.glb`. Because the GLB uses `KHR_draco_mesh_compression`, `DRACOLoader` uses the text JavaScript decoder served from `/draco/`; no binary decoder is tracked.
3. **Scientific/physics layer:** Rapier 3D WASM is imported and initialized only from `src/workers/rapier.worker.ts`. The main thread talks to this worker through the runtime-validated message contract in `src/physics/worker-contract.ts`.

## Phase 1 data flow

`GLB fixture → GLTFLoader/DRACOLoader → named Three.js meshes → position/index ArrayBuffers → transferable postMessage → Rapier Web Worker → trimesh colliders → contactPair query → structured result`.

The generator and tracked provenance manifest are the source of truth. The GLB is generated before Next.js collects `public` assets, so it is included in production output without placing a generated binary in Git.

The UI extracts only `POSITION` and index attributes from `OL_COLLISION_UPPER` and `OL_COLLISION_LOWER`. It transfers cloned attribute buffers in the `postMessage` transfer list so geometry ownership moves to the worker without an additional structured-clone copy. Rapier constructs trimesh colliders and executes the contact query in the worker.

## Threading rule

Scientific calculations must never run on the browser main thread. The main thread may render, display loading state, and send/receive structured-cloneable messages. Physics, collision validation, and future numeric analysis belong in dedicated Web Workers.

## LLM boundary

The LLM is not part of the scientific calculation path. Future LLM features, if any, may only explain UI state or documentation and must not create, modify, approve, or replace numerical collision results.
