import { createHash } from "node:crypto";
import { mkdir, writeFile } from "node:fs/promises";
import pkg from "gltf-pipeline";

const { gltfToGlb, processGltf } = pkg;
const outputDirectory = "public/generated";
const outputPath = `${outputDirectory}/opposing-occlusal-surfaces.glb`;

const upperPositions = new Float32Array([
  -1, 0.08, -1, 1, 0.08, -1, 1, 0.08, 1, -1, 0.08, 1, 0, 0.08, 0,
]);
const lowerPositions = new Float32Array([
  -1, -0.08, -1, 1, -0.08, -1, 1, -0.08, 1, -1, -0.08, 1, 0, -0.08, 0,
]);
const indices = new Uint16Array([0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4]);
const chunks = [];
const bufferViews = [];
let byteOffset = 0;

function appendView(typedArray) {
  const padding = (4 - (byteOffset % 4)) % 4;
  if (padding) {
    chunks.push(Buffer.alloc(padding));
    byteOffset += padding;
  }

  const bytes = Buffer.from(typedArray.buffer, typedArray.byteOffset, typedArray.byteLength);
  chunks.push(bytes);
  bufferViews.push({ buffer: 0, byteOffset, byteLength: bytes.length });
  byteOffset += bytes.length;
  return bufferViews.length - 1;
}

const upperPositionView = appendView(upperPositions);
const upperIndexView = appendView(indices);
const lowerPositionView = appendView(lowerPositions);
const lowerIndexView = appendView(indices);
const sourceBuffer = Buffer.concat(chunks);
const gltf = {
  asset: { version: "2.0", generator: "Occlusion Lab fixture generator 1.0.0" },
  scene: 0,
  scenes: [{ nodes: [0, 1] }],
  nodes: [
    { name: "OL_COLLISION_UPPER", mesh: 0 },
    { name: "OL_COLLISION_LOWER", mesh: 1 },
  ],
  meshes: [
    { name: "OL_COLLISION_UPPER", primitives: [{ attributes: { POSITION: 0 }, indices: 1, mode: 4 }] },
    { name: "OL_COLLISION_LOWER", primitives: [{ attributes: { POSITION: 2 }, indices: 3, mode: 4 }] },
  ],
  buffers: [{
    byteLength: sourceBuffer.length,
    uri: `data:application/octet-stream;base64,${sourceBuffer.toString("base64")}`,
  }],
  bufferViews,
  accessors: [
    { bufferView: upperPositionView, componentType: 5126, count: 5, type: "VEC3", min: [-1, 0.08, -1], max: [1, 0.08, 1] },
    { bufferView: upperIndexView, componentType: 5123, count: 12, type: "SCALAR" },
    { bufferView: lowerPositionView, componentType: 5126, count: 5, type: "VEC3", min: [-1, -0.08, -1], max: [1, -0.08, 1] },
    { bufferView: lowerIndexView, componentType: 5123, count: 12, type: "SCALAR" },
  ],
  materials: [],
};

const processed = await processGltf(gltf, {
  dracoOptions: { compressionLevel: 7 },
  resourceDirectory: outputDirectory,
});
const { glb } = await gltfToGlb(processed.gltf, { resourceDirectory: outputDirectory });

await mkdir(outputDirectory, { recursive: true });
await writeFile(outputPath, glb);
const hash = createHash("sha256").update(glb).digest("hex");
console.log(`Generated ${outputPath}`);
console.log(`SHA-256 ${hash}`);
