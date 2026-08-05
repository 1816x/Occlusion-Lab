# Design decisions

## Decisions documented in Phase 0

- **Next.js + strict TypeScript:** selected for a production-oriented React application shell with compile-time checks and CI enforcement.
- **Three.js WebGL2:** selected for broad browser 3D support. The app explicitly checks `renderer.capabilities.isWebGL2` and does not provide a WebGL1 fallback.
- **Rapier in a Web Worker:** Rapier WASM is initialized in `src/workers/rapier.worker.ts`, never in UI components. This preserves responsiveness and keeps scientific calculations off the main thread.
- **Compatibility Rapier package:** `@dimforge/rapier3d-compat` is used because Rapier documentation identifies compat packages as bundler-friendly packages that embed WASM.
- **Typed worker contract:** all worker messages are defined in TypeScript and tested before adding full simulation features.
- **Synthetic-only assets:** Phase 0 intentionally avoids dental meshes, clinical scans, patient data, and unlicensed models.

## Licensing requirements

- Repository code: MIT, inherited from `LICENSE`.
- Rapier: Apache-2.0 according to Rapier project documentation; retain notices when required.
- Three.js: MIT; retain notices when required.
- Next.js/React/tooling: open-source dependencies must be reviewed before release.
- Future dental models: must include provenance, author, license, redistribution permission, allowed educational/scientific use, and confirmation that no real patient data or protected health information is present.

## Documentation source check

On 2026-08-05, official documentation reviewed included Next.js TypeScript/configuration docs, Three.js `WebGLRenderer` docs, Rapier JavaScript getting-started/determinism/common-mistakes docs, and MDN Web Workers/structured clone docs. The supplied project specification URL could not be fetched anonymously in this environment; the Phase 0 implementation follows the requirements provided in the task prompt.
