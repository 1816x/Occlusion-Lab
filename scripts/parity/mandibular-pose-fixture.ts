import { MANDIBLE_CLOSED_TRANSLATION_Y_METERS, mandibularTransform } from "../../src/physics/mandibular-pose-reference";
import { POSE_LIMITS, WORKER_PROTOCOL_VERSION, isMandibularPose, type MandibularPose } from "../../src/physics/worker-contract";

export const FIXTURE_PATH = "fixtures/parity/mandibular-pose-v1.json";
export const NUMERIC_COMPARISON_TOLERANCE = 1e-12;

type CaseSeed = { id: string; pose: MandibularPose };
const validSeeds: CaseSeed[] = [
  { id: "neutral-open-pose", pose: { openingMeters: 0.25, protrusionMeters: 0, lateralMeters: 0 } },
  { id: "fully-closed-pose", pose: { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 } },
  { id: "maximum-opening-boundary", pose: { openingMeters: 0.25, protrusionMeters: 0.025, lateralMeters: 0 } },
  { id: "maximum-protrusion-boundary", pose: { openingMeters: 0.1, protrusionMeters: 0.05, lateralMeters: 0 } },
  { id: "minimum-left-lateral-boundary", pose: { openingMeters: 0.1, protrusionMeters: 0.01, lateralMeters: -0.05 } },
  { id: "maximum-right-lateral-boundary", pose: { openingMeters: 0.1, protrusionMeters: 0.01, lateralMeters: 0.05 } },
  { id: "combined-valid-interior", pose: { openingMeters: 0.123, protrusionMeters: 0.034, lateralMeters: -0.012 } },
  { id: "opening-lower-boundary", pose: { openingMeters: 0, protrusionMeters: 0.025, lateralMeters: 0.01 } },
  { id: "protrusion-lower-boundary", pose: { openingMeters: 0.1, protrusionMeters: 0, lateralMeters: 0.01 } },
  { id: "lateral-lower-with-axis-upper-boundaries", pose: { openingMeters: 0.25, protrusionMeters: 0.05, lateralMeters: -0.05 } },
];
const invalidSeeds: (CaseSeed & { field: keyof MandibularPose })[] = [
  { id: "opening-below-minimum", pose: { openingMeters: -0.001, protrusionMeters: 0, lateralMeters: 0 }, field: "openingMeters" },
  { id: "opening-above-maximum", pose: { openingMeters: 0.251, protrusionMeters: 0, lateralMeters: 0 }, field: "openingMeters" },
  { id: "protrusion-below-minimum", pose: { openingMeters: 0.1, protrusionMeters: -0.001, lateralMeters: 0 }, field: "protrusionMeters" },
  { id: "protrusion-above-maximum", pose: { openingMeters: 0.1, protrusionMeters: 0.051, lateralMeters: 0 }, field: "protrusionMeters" },
  { id: "lateral-below-minimum", pose: { openingMeters: 0.1, protrusionMeters: 0, lateralMeters: -0.051 }, field: "lateralMeters" },
  { id: "lateral-above-maximum", pose: { openingMeters: 0.1, protrusionMeters: 0, lateralMeters: 0.051 }, field: "lateralMeters" },
];

export function createMandibularPoseFixture() {
  const validCases = validSeeds.map(({ id, pose }) => {
    if (!isMandibularPose(pose)) throw new Error(`Generator seed ${id} must be accepted`);
    return { id, inputPose: pose, expectedAcceptance: true, expectedTranslationMeters: mandibularTransform(pose).translationMeters, expectedRotationQuaternion: { x: 0, y: 0, z: 0, w: 1 } };
  });
  const invalidCases = invalidSeeds.map(({ id, pose, field }) => {
    if (isMandibularPose(pose)) throw new Error(`Generator seed ${id} must be rejected`);
    return { id, inputPose: pose, expectedAcceptance: false, expectedValidationField: field, expectedErrorCategory: "invalid_range" };
  });
  return {
    schemaVersion: 1,
    legacyBehaviorVersion: "mandibular-pose-v1",
    workerProtocolVersion: WORKER_PROTOCOL_VERSION,
    sourceImplementation: "src/physics/mandibular-pose-reference.ts",
    sourceBaselineCommit: "11ca272b18467ef0a44140b14e075a311dbb6139",
    internalUnit: "meters",
    poseLimits: POSE_LIMITS,
    closedMandibleTranslationYMeters: MANDIBLE_CLOSED_TRANSLATION_Y_METERS,
    numericComparisonTolerance: NUMERIC_COMPARISON_TOLERANCE,
    validCases,
    invalidCases,
  };
}

export const serializeMandibularPoseFixture = () => `${JSON.stringify(createMandibularPoseFixture(), null, 2)}\n`;
export function verifyMandibularPoseFixture(actual: string) {
  const expected = serializeMandibularPoseFixture();
  if (actual !== expected) throw new Error(`Golden fixture drift detected: run npm run parity:generate and review ${FIXTURE_PATH}`);
}
