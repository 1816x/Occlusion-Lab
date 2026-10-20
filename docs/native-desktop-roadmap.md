# Native geometry and desktop presentation roadmap

The current native path accepts only synthetic, meter-valued triangle OBJ files. It parses
them without external dependencies, verifies closed outward-oriented topology, and passes
them to the existing FCL evaluator. A closed surface can still self-intersect or contain
degenerate triangles; topology validation is not geometry repair or clinical validation.

## Phase 4.12: optional CGAL processing boundary

- Add a separate `occlusion-geometry` library behind a CMake option and the existing
  vcpkg `geometry` feature. Keep CGAL private to this target; `occlusion-core`, OBJ
  parsing, and default builds must remain independent of it.
- Convert the validated project `TriangleMesh` into a CGAL surface mesh with explicit
  `double` meter coordinates and checked indices. Return project-owned structured
  errors when conversion or geometric validation fails.
- First expose read-only checks for self-intersection and degenerate faces. A repair
  operation, if later needed, must be opt-in and return a new mesh plus a report of
  every topology or position change. Do not silently alter the input used in a result.
- Cover a closed box, an open box, a self-intersecting synthetic example, and a
  degenerate face. Run both geometry-enabled and geometry-disabled builds in CI.

## Desktop integration after the geometry boundary

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
