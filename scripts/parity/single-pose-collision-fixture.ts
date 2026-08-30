import { CONTACT_TOLERANCE_METERS, syntheticGapMeasurements } from "../../src/physics/synthetic-contact-reference";

export const COLLISION_FIXTURE_PATH = "fixtures/parity/single-pose-collision-v1.json";
const baseline = "5607b26f856481f4aacb36139ec9837f125e80c5";
const gaps = [
  ["clear-separation", 0.01],
  ["separation-above-tolerance", 0.0000011],
  ["positive-near-touch", 0.0000005],
  ["exact-touching", 0],
  ["negative-near-touch", -0.0000005],
  ["penetration-beyond-tolerance", -0.0000011],
  ["clear-penetration", -0.01],
] as const;

const box = (minimum: [number, number, number], maximum: [number, number, number]) => ({
  verticesMeters: [
    [minimum[0], minimum[1], minimum[2]], [maximum[0], minimum[1], minimum[2]],
    [maximum[0], maximum[1], minimum[2]], [minimum[0], maximum[1], minimum[2]],
    [minimum[0], minimum[1], maximum[2]], [maximum[0], minimum[1], maximum[2]],
    [maximum[0], maximum[1], maximum[2]], [minimum[0], maximum[1], maximum[2]],
  ],
  triangles: [[0,2,1],[0,3,2],[4,5,6],[4,6,7],[0,1,5],[0,5,4],[3,7,6],[3,6,2],[0,4,7],[0,7,3],[1,2,6],[1,6,5]],
});

export function collisionFixture() {
  return {
    schemaVersion: 1,
    behavioralVersion: "phase-4.3",
    workerProtocolVersion: 4,
    sourceImplementation: "src/physics/synthetic-contact-reference.ts",
    sourceBaselineCommit: baseline,
    geometry: {
      id: "closed-opposing-boxes-v1",
      provenance: "Hand-authored synthetic closed boxes; no patient or downloaded geometry",
      units: "meters",
      fixed: box([-0.02, -0.01, -0.02], [0.02, 0, 0.02]),
      moving: box([-0.015, 0, -0.015], [0.015, 0.01, 0.015]),
    },
    classificationToleranceMeters: CONTACT_TOLERANCE_METERS,
    comparisonToleranceMeters: 1e-9,
    parityExclusions: ["contact manifold points", "contact normals", "contact ordering", "contact count", "evaluated motion sweeps"],
    cases: gaps.map(([id, gap]) => {
      const result = syntheticGapMeasurements(gap);
      return {
        id,
        movingTransform: { translationMeters: { x: 0, y: gap, z: 0 }, rotation: [1,0,0,0,1,0,0,0,1] },
        expectedSemanticGapMeters: gap,
        classification: result.classification,
        clearanceMeters: result.clearanceMeters,
        penetrationDepthMeters: result.penetrationDepthMeters,
      };
    }),
  };
}
export const serializeSinglePoseCollisionFixture = () => `${JSON.stringify(collisionFixture(), null, 2)}\n`;
export function verifySinglePoseCollisionFixture(actual: string) {
  const expected = serializeSinglePoseCollisionFixture();
  if (actual !== expected) throw new Error(`${COLLISION_FIXTURE_PATH} differs from its canonical generation`);
}
