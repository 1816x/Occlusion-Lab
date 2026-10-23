# Native geometry and desktop presentation roadmap

The current native path accepts only synthetic, meter-valued triangle OBJ files. It parses
them without external dependencies, verifies closed outward-oriented topology, and passes
them to the existing FCL evaluator. A closed surface can still self-intersect or contain
degenerate triangles; topology validation is not geometry repair or clinical validation.

## Phase 4.12: optional CGAL processing boundary (implemented)

- `occlusion-geometry` is a separate library behind a CMake option and the existing
  vcpkg `geometry` feature. CGAL is private to this target; `occlusion-core`, OBJ
  parsing, and default builds remain independent of it.
- The library converts a validated project `TriangleMesh` into a CGAL surface mesh
  with explicit `double` meter coordinates and checked indices. It returns
  project-owned structured errors when conversion or geometric validation fails.
- The first read-only checks now cover self-intersection and degenerate faces. A repair
  operation, if later needed, must be opt-in and return a new mesh plus a report of
  every topology or position change. Do not silently alter the input used in a result.
- Tests cover a closed box, an open box, a self-intersecting synthetic example,
  a degenerate face, and a non-finite coordinate. CI configures both geometry-enabled
  and geometry-disabled builds; the Ubuntu geometry job installs `libcgal-dev` and
  passed its remote configure, build, and test steps. The vcpkg feature remains available for
  toolchain-based builds.

Phase 4.13 adds an explicit `--inspect-mesh` CLI command with versioned JSON errors.
The CLI continues to use its existing topology and FCL path for evaluation; CGAL
screening is requested separately and does not silently change evaluation results.

## Desktop integration after the geometry boundary

The first viewer's data flow, error presentation, threading, and parity gates are
specified in [desktop-viewer-phase-4.13.md](desktop-viewer-phase-4.13.md).

- Put Qt 6 application state, file selection, error presentation, and cancellation in
  `occlusion-desktop`; call public project APIs rather than FCL or CGAL directly.
- Put VTK scene construction in a presentation adapter. Display units and axis labels
  explicitly, and keep rendering state out of the scientific result contracts.
- Reuse the same versioned pose and sweep exports as the CLI. Capture deterministic
  desktop flow tests for importing two synthetic meshes, evaluating a pose and sweep,
  displaying structured errors, and exporting results.
- Keep the browser reference until representative golden fixtures and the desktop
  flows demonstrate the required behavioral parity. Exact contact manifolds are not
  part of that parity contract.
