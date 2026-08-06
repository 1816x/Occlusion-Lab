# Occlusion Lab

Occlusion Lab is a **work in progress** educational browser sandbox for synthetic dental-occlusion visualization experiments. It is not a medical device, not clinical decision-support software, and not suitable for diagnosis or treatment planning.

## Phase 1 scope

- Loads an original project-owned compressed GLB fixture of low-poly opposing occlusal surfaces with Three.js `GLTFLoader` and `DRACOLoader`.
- Extracts only named collision mesh position/index buffers and transfers those `ArrayBuffer`s to the Rapier Web Worker.
- Keeps Rapier initialized exclusively in `src/workers/rapier.worker.ts`.
- Runs deterministic separated and controlled-contact scenarios off the main thread and displays structured results in the browser.
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
