# ADR-0002: Isolated FCL collision boundary

- **Status:** Accepted for Phase 4.3
- **Date:** 2026-08-30

## Context

Native collision must support deterministic queries over as many as 61 poses without rebuilding mesh acceleration structures. FCL is an implementation dependency, while application contracts must remain stable and independently testable.

## Decision

`occlusion-collision` depends on `occlusion-core` and links FCL privately. Public headers contain only project-owned meshes, transforms, measurements, classifications, validation results, and an opaque `CollisionModel` implementation. `occlusion-core` does not depend on FCL.

Compilation validates and consumes a triangle mesh once, builds and finalizes an immutable double-precision FCL BVH, and returns a safely shared model. Queries reuse that BVH and create only lightweight per-query collision objects and transforms. There are no global caches, singleton engines, mutable global state, or borrowed mesh lifetimes.

A model may be copied and shared across threads. Concurrent queries are safe because compiled geometry is immutable and all request/result state is local. The engine itself has no state.

Lengths are `double` meters. The semantic tolerance is `1e-6 m`. Distance is queried first; collision contacts are requested only inside that tolerance. Results normalize to `separated`, `touching`, or `penetrating`; penetration selects the maximum finite depth, independent of manifold ordering. Non-finite engine output is an error.

Parity uses two explicitly indexed, consistently triangulated closed boxes. Closed volumes give penetration a robust meaning that the legacy open planar GLB cannot provide. The golden fixture compares classification, clearance, and penetration depth—not manifold point, normal, order, or count.

## Consequences

Core-only builds remain lightweight. Collision builds require the pinned vcpkg `collision` feature and its exported FCL CMake package. BVH construction cost is paid once per mesh and models can be reused for complete future sweeps.

This phase does not replace Rapier, alter protocol version 4, evaluate complete sweeps, normalize contact samples, expose FCL, add UI/rendering, or make clinical claims. Native single-pose evaluation and bounded contact-sample normalization are deferred to Phase 4.4.
