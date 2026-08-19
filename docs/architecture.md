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

## Phase 1.1 measurement path

The result contract uses `classification` (`separated`, `touching`, or `penetrating`), `clearanceMeters`, and `penetrationDepthMeters`. Every scalar and vector component is runtime-validated with `Number.isFinite`; measurements must be non-negative and mutually consistent with classification. Invalid requests, including non-finite transforms, remain typed worker error responses.

Rapier's worker-side trimesh contact manifold provides contact count, first local point, and normal when available. Because that manifold query does not reliably provide witness-point distance for separated trimeshes, the same worker independently computes the signed vertical gap from the transferred vertices after applying the requested upper Y translation. It subtracts the highest lower vertex from the lowest transformed upper vertex. Positive gap is clearance, a gap within `1e-6 m` is touching, and negative gap is converted to positive penetration depth. Results are rounded to `1e-6 m`. No scientific calculation moves onto the UI thread.

## Phase 2 persistent session and UI boundary

Protocol v3 splits initialization from evaluation. `initialize-occlusion-fixture` transfers both position/index buffer pairs once and replaces any prior worker-owned world. The worker creates a fixed maxillary body and zero-gravity, directly positioned dynamic mandibular body. `evaluate-mandibular-pose` carries only the fixture ID, request ID, integer sequence, and three numeric pose fields; the retained world is stepped rather than rebuilt.

The authoritative worker transform is `(lateral, 0.16 − opening, protrusion)` meters with an identity unit quaternion. Axes follow the fixture's right-handed coordinates: +Y up, +Z anterior for this synthetic lesson, and −X/+X left/right. Three.js never recalculates this transform. It applies the returned translation/quaternion to the lower mesh.

Each incoming UI event is `MessageEvent<unknown>` and must pass `isPhysicsWorkerResponse`. A testable main-thread coordinator assigns a desired revision immediately on every control change. One `requestAnimationFrame` callback may coalesce several changes and dispatch only the latest immutable `{pose, revision, generation}` snapshot; revision gaps are therefore expected. A result older than the latest desired revision is stale even when the newer revision has not yet been posted.

Pending request metadata correlates the response ID, sequence, fixture, generation, and returned `requestedPose` with that immutable snapshot. The lesson interprets an accepted result exclusively through `requestedPose`; Three.js applies only the returned worker-authoritative transform. While the latest desired revision is pending, previous result text and contact markers are cleared and lesson advancement is paused.

Reset/retry increments the generation, invalidates all prior in-flight IDs, clears result/contact state, restores the separated controls and the last worker-authoritative separated transform, and schedules a fresh evaluation. Responses for invalidated generations and older desired revisions are silently ignored. A payload that fails runtime validation, an unknown current-generation ID, a correlation mismatch, a Worker `error`, or a Worker `messageerror` is surfaced visibly and cannot partially update UI state.

For contacts, Rapier supplies local manifold points, normals, signed contact distance, and collision state. The worker transforms both local witness points to world space and reports their midpoint, validates finite components, deduplicates, sorts, and bounds samples. One reusable `THREE.Points` geometry is updated or assigned an empty draw range when contacts disappear; no per-contact scene-object growth occurs.

## Phase 3 sweep lifecycle (protocol 4)

The UI sends `evaluate-mandibular-sweep` with a validated preset, fixture correlation, sequence, and 2–61 frames (default 31). The persistent Worker owns endpoint selection, inclusive interpolation, Rapier evaluation, classifications, penetration measurements, bounded contact sampling (32 per frame), and summary calculation. Colliders are initialized once rather than rebuilt per frame. The response contains authoritative poses/transforms and documents the final world pose as `finalPose` (the last evaluated endpoint).

Runtime validation recursively checks ordering, exact endpoint progress, finite poses/transforms/contact data, collection bounds, and summary consistency before React or Three.js can receive a result. Live and sweep work use generation invalidation; stale known IDs are silent, unknown current IDs are visible errors. Timeline scrubbing consumes only the latest validated cached response and reuses one Three.js marker geometry.

## Phase 3.1 presentation and export boundary

`three-scene.tsx` continues to own Worker creation, response validation/correlation, Three.js transforms, and reusable contact markers. `motion-sweep-lab.tsx` receives typed state and callbacks and owns only the accessible sweep presentation. Pure presentation helpers clamp an index, select an existing cached frame, format progress/meters for display, and map an already returned classification to its label/CSS state. They never interpolate, replace transforms, classify contact, or calculate summaries.

The export flow is `validated protocol-v4 SweepResult → pure serializer → explicit button click → UTF-8 Blob → object URL → browser download → scheduled URL revocation`. No export action posts a Worker request or changes the inspected frame or scene. Starting a newer request clears the retained sweep in the parent, and export controls are disabled while pending, so partial or stale state cannot cross the download boundary. Only the latest validated completed sweep is retained and eligible.

The JSON serializer is the complete-data representation. Its version-1 top-level fields are `schemaVersion`, `workerProtocolVersion`, `fixtureId`, `preset`, `frameCount`, `summary`, `finalPose`, and `frames`. Each frame contains `frameIndex`, `progress`, `requestedPose`, `appliedTransform`, `classification`, `measurementStatus`, `clearanceMeters`, `contactCount`, `penetrationDepthMeters`, and bounded `contactSamples`. Nested poses, transforms, vectors, quaternions, summaries, and samples retain the protocol-v4 Worker values.

The CSV serializer is intentionally a compact frame index with exactly these columns: `frame_index`, `progress`, `opening_meters`, `protrusion_meters`, `lateral_meters`, `translation_x_meters`, `translation_y_meters`, `translation_z_meters`, `classification`, `contact_count`, and `penetration_depth_meters`. It does not include full contact samples; consumers that require those use JSON.

Serialization performs no scientific recomputation. Fixed field/column order, locale-independent JavaScript numeric strings, two-space JSON indentation, RFC-compatible CSV escaping, LF endings, a final newline, and omission of timestamps/random IDs/request sequences make identical validated input byte-identical. Rapier, sweep interpolation, classification, contact measurements, and summaries remain Worker-only.
