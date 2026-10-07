# Native evaluation export v1

## Status and scope

`occlusion-native-evaluation` version 1 is the stable interchange contract for complete native
pose and evaluated-sweep results. Lengths are meters and rotation matrices are row-major 3×3
arrays. Documents are UTF-8 JSON, contain no timestamps or run identifiers, use a stable property
order, and end with one LF byte.

The contract begins with these fields, in order:

1. `schema`: the literal `occlusion-native-evaluation`;
2. `schemaVersion`: the integer `1`;
3. `fixtureId`: the caller-supplied synthetic fixture identifier;
4. `kind`: `pose` or `sweep`;
5. `units`: `{ "length": "meters", "rotation": "row-major-3x3" }`;
6. `manifoldParityGuaranteed`: always `false` in v1.

A pose then contains the requested pose, applied transform, classification, measurement status,
nullable clearance, penetration depth, nullable intersection state, published contact count, and
normalized contacts. A sweep adds its preset, requested frame count, final pose, portable summary,
and the same complete evaluation payload for every contiguous frame.

The machine-readable [JSON Schema](../schemas/native-evaluation-v1.schema.json) defines the accepted document shapes. The reviewed byte-level examples are:

- [`native-evaluation-pose-v1.json`](../fixtures/export/native-evaluation-pose-v1.json);
- [`native-evaluation-sweep-v1.json`](../fixtures/export/native-evaluation-sweep-v1.json).

They use hand-authored, engine-neutral results so a legitimate FCL manifold difference cannot
silently rebaseline the format. Native tests serialize the same values and compare every byte.

## Compatibility policy

- Readers must select behavior using both `schema` and `schemaVersion`; unsupported versions must
  fail explicitly rather than being interpreted as v1.
- A change to field names, field order, units, nullability, enum spelling, number representation,
  required fields, or field semantics requires a new schema version and new golden examples.
- Adding or removing a field is a schema-version change, even when a permissive JSON reader could
  ignore it.
- Implementation changes that leave the golden bytes and documented semantics unchanged do not
  require a version increment.
- Golden files are never regenerated automatically. Any intentional rebaseline must include a
  reviewed contract explanation, serializer tests, and documentation changes in the same pull
  request.
- Writers emit the shortest round-trippable finite JSON number using locale-independent conversion.
  Evaluator validation remains responsible for rejecting non-finite domain values before export.

## Explicit exclusions

Normalized contact samples are exported for reproducibility within a selected native engine and
build, but their coordinates, normals, count, and ordering are not a parity guarantee between FCL,
Rapier, library versions, or platforms. The portable sweep summary is the engine-neutral reduction.
The format carries synthetic geometry results only and does not represent forces, pressure,
clinical severity, diagnosis, or treatment guidance.
