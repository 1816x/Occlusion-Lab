# Occlusion Lab

Occlusion Lab is a **work in progress** educational browser sandbox for future synthetic dental-occlusion visualization experiments. It is not a medical device, not clinical decision-support software, and not suitable for diagnosis or treatment planning.

## Phase 0 scope

- Next.js App Router with strict TypeScript.
- Minimal Three.js visual layer using synthetic primitives only.
- Rapier 3D WASM initialized inside a dedicated Web Worker.
- Typed worker health-check and synthetic collision fixture contracts.
- Documentation for architecture, design decisions, and licensing constraints.

## Non-goals

No authentication, database, backend service, LLM integration, real patient data, clinical data, complete occlusion simulation, deployment, or unlicensed dental assets are included in this phase.

## Development

```bash
npm install
npm run dev
npm run lint
npm run typecheck
npm run test
npm run build
```

## Licensing and assets

The code is MIT licensed. No dental model assets are bundled. Future glTF dental assets must have documented provenance, redistribution rights, and clinical/educational use permissions before being added.
