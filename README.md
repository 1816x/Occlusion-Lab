# Occlusion Lab

Occlusion Lab is a **work in progress** educational browser sandbox for synthetic dental-occlusion visualization experiments. It is not a medical device, not clinical decision-support software, and not suitable for diagnosis or treatment planning.

## Phase 1 scope

- Loads an original project-owned compressed GLB fixture of low-poly opposing occlusal surfaces with Three.js `GLTFLoader` and `DRACOLoader`.
- Extracts only named collision mesh position/index buffers and transfers those `ArrayBuffer`s to the Rapier Web Worker.
- Keeps Rapier initialized exclusively in `src/workers/rapier.worker.ts`.
- Runs deterministic separated and touching scenarios off the main thread and displays structured results in the browser.
- Preserves the Phase 0 worker health check and synthetic collision fixture.

## Development

```bash
npm install
npm run dev
npm run lint
npm run typecheck
npm run test
npm run build
npm run assets:generate
npm run assets:verify
```

## Fixture regeneration and verification

The fixture source is `scripts/generate-phase1-fixture.mjs`. It hand-authors two small planar triangle meshes and runs the documented `gltf-pipeline` Draco compression path. `npm run assets:generate` writes the ignored build artifact `public/generated/opposing-occlusal-surfaces.glb`; `predev` and `prebuild` run that command automatically so local development and production builds are self-contained. The generator source and text manifest are authoritative; generated binaries are deliberately not committed.

Run `npm run assets:verify` to regenerate the fixture, calculate its SHA-256, and compare it with the expected deterministic hash in `public/assets/fixtures/opposing-occlusal-surfaces.manifest.json`.

## Licensing and assets

The code is MIT licensed. The generated Phase 1 GLB is original synthetic geometry and is safe to redistribute under the repository license, but the binary itself is a build artifact rather than tracked source. No dental model assets, patient data, clinical scans, or fake textures are bundled. KTX2 texture compression is documented as a future pipeline step only when a genuine textured fixture exists.

## Phase 1.1 metric contract

All fixture coordinates and reported measurements are in **meters**. The worker classifies a result as `separated` when clearance is positive, `touching` when the signed vertical gap is within the `1e-6 m` tolerance, and `penetrating` when overlap depth is positive. Accordingly, `clearanceMeters` and `penetrationDepthMeters` are always finite, non-negative numbers; the inapplicable measurement is zero rather than an ambiguous `null`.

For this deliberately planar educational fixture, clearance is calculated in the worker from the transformed collision vertices: the lowest transformed upper Y coordinate minus the highest lower Y coordinate. Thus the separated scenario is `(0.08 m + 0.25 m) - (-0.08 m) = 0.41 m`, while the `-0.16 m` translation is coplanar touching with zero penetration. A hard-coded result would silently become false whenever fixture coordinates or the transform changed. Rapier supplies contact count and, when present, the first contact point and normal; it does not supply the separated trimesh witness distance used here.

These metrics describe only synthetic planar geometry. They have no clinical or diagnostic validity.

## Phase 2 interaction

Phase 2 keeps a Rapier world alive in one Web Worker. The GLB collision buffers are transferred once during fixture initialization; subsequent version-3 messages contain only a finite, range-validated pose and sequence number. The maxilla is fixed and the directly controlled dynamic mandible receives the worker-authoritative transform. Three.js renders that returned transform and a bounded `THREE.Points` contact map.

The simplified pose uses meters internally: opening `0–0.25 m` (neutral `0.25 m`), protrusion `0–0.05 m` on +Z, and lateral `−0.05–0.05 m` on X. The UI displays millimeters. Rotation is the identity quaternion in Phase 2. These deliberately exaggerated controls teach geometric cause and effect; they do not reproduce anatomy or temporomandibular-joint biomechanics.

The guided “move from separation to first contact” checkpoint advances only after validated worker results show separation, then touching with contacts, then contacts after at least 5 mm of lateral or protrusive movement. Progress lasts only in React state for the current browser session.

### Phase 2.1 synchronization

Every control change immediately receives a monotonically increasing **desired revision**. A single animation-frame callback coalesces a burst and dispatches only its newest immutable pose snapshot, so dispatched sequences may contain gaps. As soon as revision N+1 is desired, a result for N is stale even if N+1 has not been posted yet. While waiting, the UI clears the prior result and contacts, reports that evaluation is pending, and pauses lesson advancement.

Accepted results are correlated by request ID, revision, fixture, generation, and the dispatched pose snapshot. The returned `requestedPose`—not mutable control state—is used for lesson progression, while the returned transform remains the sole authority for Three.js. Reset/retry increments a lesson generation and silently ignores its invalidated in-flight responses. Older revisions are also ignored; malformed payloads, unsolicited current-generation responses, Worker errors, and message-deserialization errors remain visible boundary failures. These rules do not change the synthetic, educational, non-clinical scope.

Interactive contact points and normals come from Rapier manifolds and are converted from collider-local to world coordinates, sorted, deduplicated at `1e-5 m`, and capped at 32. Marker color means touching or geometric penetration only. It is not force, pressure, stress, severity, diagnosis, or treatment advice. Rapier's selected trimesh API does not provide a reliable arbitrary separated-mesh distance, so interactive clearance is explicitly unavailable rather than displayed as zero. The aligned planar Phase 1.1 `0.41 m` regression remains separate and valid only for that fixture/query.

## Phase 3 — deterministic motion sweeps

The Motion Sweep Lab requests one of four synthetic presets (`closing`, `protrusive`, `left-lateral`, or `right-lateral`) from protocol version 4. The Worker deterministically interpolates 2–61 frames (31 by default), includes both endpoints, evaluates every frame in the persistent Rapier world, and leaves the mandible at the returned `finalPose`. Each frame is capped at 32 world-space contact samples. The Worker also calculates the first/last contact frame, contact-frame count, maximum geometric penetration and frame, and final-frame persistence.

Live pose and sweep generations are isolated: beginning either mode invalidates older in-flight work, late invalidated responses are ignored, and uncorrelated current responses are errors. Scrubbing applies cached, validated Worker transforms and samples without issuing physics work. Only the latest sweep is retained.

All geometry and motion ranges are synthetic educational fixtures. They are not anatomically accurate and do not represent patient data, diagnosis, treatment planning, force, pressure, stress, or bite quality.

## Phase 3.1 — deterministic sweep exports

After the latest sweep has completed validation, the Motion Sweep Lab can download either a complete JSON document or a compact CSV table. Export controls remain disabled before completion and while a newer sweep is pending. JSON preserves the Worker summary, final pose, every frame, applied transforms, measurements, and bounded contact samples. CSV deliberately provides only one summary row per frame and excludes contact samples.

JSON schema version 1 has this fixed top-level order:

```text
schemaVersion, workerProtocolVersion, fixtureId, preset, frameCount,
summary, finalPose, frames
```

Each `frames[]` entry contains, in order, `frameIndex`, `progress`, `requestedPose`, `appliedTransform`, `classification`, `measurementStatus`, `clearanceMeters`, `contactCount`, `penetrationDepthMeters`, and `contactSamples`. `requestedPose` contains `openingMeters`, `protrusionMeters`, and `lateralMeters`; `appliedTransform` contains `translationMeters` (`x`, `y`, `z`) and `rotationQuaternion` (`x`, `y`, `z`, `w`). Each bounded sample contains the protocol-v4 fields `id`, `pointWorldMeters`, `normalWorld`, `signedDistanceMeters`, `penetrationDepthMeters`, `surfaces`, and `units`. `summary` and `finalPose` are the validated Worker-returned values documented by protocol v4.

CSV uses this exact fixed header:

```csv
frame_index,progress,opening_meters,protrusion_meters,lateral_meters,translation_x_meters,translation_y_meters,translation_z_meters,classification,contact_count,penetration_depth_meters
```

Both serializers omit timestamps, random IDs, and request sequence IDs; serialize numbers without locale formatting; use LF line endings and a final newline; and produce byte-identical output for identical validated results. JSON uses two-space indentation and stable property order. CSV uses RFC-compatible field escaping and intentionally does not flatten contact samples. Filenames are `occlusion-lab-{preset}-{frameCount}-frames.json` and `.csv`.

Exports contain synthetic geometric Worker results only. Penetration is geometric overlap—not force, pressure, stress, clinical severity, or bite quality. These files contain no patient data and are not clinical reports, diagnoses, or treatment guidance.
