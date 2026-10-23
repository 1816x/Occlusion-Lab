# Phase 4.13 desktop viewer contract

The first desktop application will be a synthetic-mesh viewer and evaluation client,
not a second scientific engine. Qt 6 owns windows, commands, and state. VTK owns
scene objects. Project libraries own parsing, geometry checks, transforms, FCL
queries, sweep summaries, and versioned export. The existing browser application
remains the behavior reference until its required workflows have desktop parity.

## Module and data boundaries

| Module | Inputs | Outputs and responsibility |
|---|---|---|
| `occlusion-mesh-io` | Meter-valued OBJ text | Project `TriangleMesh` or structured parse/topology error. |
| `occlusion-geometry` (optional) | Project `TriangleMesh` | Read-only defect report. No repair or VTK types. |
| `occlusion-evaluation` | Two validated project meshes and a pose or sweep request | `PoseEvaluationResult` or `EvaluatedSweepResult`; unchanged v1 JSON export. |
| Future desktop file adapter | Two direct `.obj` names in an operator-selected directory | Bounded read and structured file error, using the same rules as the CLI. This adapter must move out of the CLI tree before the desktop imports files. |
| Future VTK adapter | Copies of project meshes plus a published evaluation result | Fixed/moving actors and contact markers. It performs no collision or classification. |
| Future Qt application | File selection, pose and sweep requests, and engine results | Controls, error display, frame selection, and export action. |

Only the optional desktop target will link Qt 6 and VTK. `occlusion-core` must
remain independent of both. The VTK adapter accepts copied mesh arrays and
`RigidTransform`/normalized contact samples from public project headers; it
must not call FCL or CGAL or reinterpret their private data.

## First viewer flow

1. The user selects a trusted directory and two direct synthetic `.obj` files.
   The desktop file adapter enforces the CLI's filename, symlink, 16 MiB, OBJ,
   and closed-topology checks. If the optional geometry feature is present,
   the user can request the same CGAL inspection before evaluation.
2. File, parser, topology, and geometry errors appear with file, code, field,
   line (when known), and message. An invalid mesh blocks evaluation and
   export; the scene keeps its last valid result clearly labeled.
3. A valid pair compiles one `PoseEvaluator` session. Pose inputs use explicit
   millimeters and existing inclusive limits. Evaluation runs away from the
   Qt UI thread and publishes only a complete `PoseEvaluationResult`.
4. The VTK adapter displays the fixed mesh and applies the result's
   `applied_transform` to the moving mesh. It draws only normalized contact
   samples and shows clearance or penetration according to the public
   measurement status. Rendering never recomputes these values.
5. A sweep uses the existing preset and 2–61 frame contract. The frame slider
   selects a stored `EvaluatedSweepFrame`; summary labels come from the
   returned `EvaluatedSweepSummary`. Export calls the existing v1 serializer.

Qt and VTK objects stay on the UI thread. Worker results cross that boundary as
owned project values, with a request generation number so late results from an
older selection cannot replace a newer scene. The first release disables a
request while it is running; cooperative cancellation needs an explicit engine
API and is not simulated by discarding a running FCL query. It must never
publish a partial sweep as a complete result.

## Presentation and acceptance rules

- Label coordinate axes and all displayed distances in millimeters, converting
  only for display; engine storage remains `double` meters.
- Distinguish separated, touching, and penetrating states using text as well
  as color. An unavailable clearance is shown as unavailable, never zero.
- Contact markers are bounded by the published normalized sample count. They
  are geometric samples, not force or pressure measurements.
- Test the synthetic closed-box pose and sweep against the current v1 export,
  including exact frame count, endpoints, and selected-frame transform.
- Test malformed OBJ, open mesh, self-intersection (when CGAL is enabled),
  invalid pose, and stale asynchronous result handling. Validate export bytes
  against the existing engine serializer rather than a second UI serializer.
- Keep the web app and its tests until the desktop workflow and engine parity
  gates in ADR-0001 are met. No clinical interpretation is introduced.
