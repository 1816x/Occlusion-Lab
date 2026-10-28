# Phase 4.13 desktop viewer contract

Phase 4.14 now has a first Qt 6/VTK development viewer. It uses a VTK offscreen
render window captured into a Qt image, with Qt controls for import, pose, sweep,
frame selection, camera rotation, and export. The Linux display smoke covers
import, failed import and recovery, stale worker result rejection, pose, sweep
selection, rendering, and exact JSON export bytes. The Apple Silicon macOS
bundle passes CI relocation, signature, and clean-environment launch checks;
a visual launch on another Mac remains a beta gate.

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
| `occlusion-evaluation` | Two validated project meshes and a pose or sweep request | `MeshEvaluationSession` owns the imported mesh pair and compiled evaluator; queries return `PoseEvaluationResult` or `EvaluatedSweepResult` with unchanged v1 JSON export. |
| Shared mesh file adapter | Two direct `.obj` names in an operator-selected directory | Bounded read and structured file error, using the same rules as the CLI. Exported by `occlusion-mesh-io` for desktop clients. |
| VTK viewer layer | Copies of project meshes plus a published evaluation result | Fixed/moving actors and contact markers. It performs no collision or classification. |
| Qt application | File selection, pose and sweep requests, and engine results | Controls, error display, frame selection, and export action. |

Only the optional desktop target will link Qt 6 and VTK. `occlusion-core` must
remain independent of both. The VTK adapter accepts copied mesh arrays and
`RigidTransform`/normalized contact samples from public project headers; it
must not call FCL or CGAL or reinterpret their private data.

## First viewer flow

1. The user selects a trusted directory and two direct synthetic `.obj` files.
   The desktop file adapter enforces the CLI's filename, symlink, 16 MiB, OBJ,
   and closed-topology checks. If the optional geometry feature is present,
   the user can request the same CGAL inspection before evaluation.
2. File, parser, and topology errors appear with file, code, field, line
   (when known), and message. An invalid mesh blocks evaluation and export;
   the scene keeps its last valid image clearly labeled. Optional CGAL
   inspection remains a separate CLI action.
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
