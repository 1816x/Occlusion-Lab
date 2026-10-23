# Synthetic mesh inspection JSON v1

`occlusion-cli --inspect-mesh <directory> <mesh.obj>` is available when
`OCCLUSION_ENABLE_GEOMETRY` is enabled. It reads one direct synthetic OBJ file,
applies the bounded parser and closed-topology checks, then calls the optional
CGAL inspection API. It does not require FCL or perform an evaluation.

The command writes exactly one compact JSON document and a trailing newline to
stdout. A valid mesh exits 0:

```json
{"schema":"occlusion-mesh-inspection","schemaVersion":1,"status":"valid"}
```

An invalid file or mesh exits 2 with an error object:

```json
{"schema":"occlusion-mesh-inspection","schemaVersion":1,"status":"invalid","error":{"code":"self_intersection","field":"mesh","line":0,"message":"surface has intersecting triangles"}}
```

`code` is stable and intended for decisions; `message` is for people. `field`
identifies a file property, OBJ record, edge, vertex, triangle, or entire mesh.
`line` is the one-based OBJ line for parser errors and 0 when no source line
applies. The directory and filename are never echoed into the JSON. Usage
errors (including a wrong argument count) exit 2 with usage on stderr and no
JSON. The exact document fields and allowed codes are in
[`native-mesh-inspection-v1.schema.json`](../schemas/native-mesh-inspection-v1.schema.json).

Changing field names, order, status meanings, code meanings, line convention,
or JSON encoding requires a new schema version. The command never repairs the
input. It only supports synthetic, meter-valued files from a trusted directory;
the portable file checks do not protect against a concurrent hostile writer.
Successful inspection does not confer clinical validity. The independent
`--evaluate-meshes` path does not automatically run this optional CGAL check.
