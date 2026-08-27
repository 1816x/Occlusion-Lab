import { mandibularTransform } from "../../src/physics/mandibular-pose-reference";
import { sweepEndpoints, sweepPoses } from "../../src/physics/mandibular-sweep-reference";
import {
  SWEEP_FRAME_LIMITS,
  SWEEP_PRESETS,
  WORKER_PROTOCOL_VERSION,
  isMandibularPose,
} from "../../src/physics/worker-contract";

export const SWEEP_FIXTURE_PATH = "fixtures/parity/mandibular-sweep-generation-v1.json";
export const SWEEP_NUMERIC_COMPARISON_TOLERANCE = 1e-12;
export const SWEEP_SOURCE_BASELINE_COMMIT = "39e206e48599333d2fa76947352e09be04c39dee";
export const SWEEP_FIXTURE_FRAME_COUNTS = [2, 3, 11, 31, 61] as const;

export function createMandibularSweepFixture() {
  const cases = SWEEP_PRESETS.flatMap((preset) =>
    SWEEP_FIXTURE_FRAME_COUNTS.map((frameCount) => {
      const [startPose, endPose] = sweepEndpoints(preset);
      const frames = sweepPoses(preset, frameCount).map(({ frameIndex, progress, pose }) => {
        if (!isMandibularPose(pose)) throw new Error(`${preset}-${frameCount} generated an invalid pose`);
        const transform = mandibularTransform(pose);
        return {
          frameIndex,
          progress,
          expectedPose: pose,
          expectedTranslationMeters: transform.translationMeters,
          expectedRotationQuaternion: transform.rotationQuaternion,
        };
      });
      return { id: `${preset}-${frameCount}-frames`, preset, frameCount, startPose, endPose, frames };
    }),
  );
  return {
    schemaVersion: 1,
    legacyBehaviorVersion: "mandibular-sweep-generation-v1",
    workerProtocolVersion: WORKER_PROTOCOL_VERSION,
    sourceImplementation: "src/physics/mandibular-sweep-reference.ts",
    sourceBaselineCommit: SWEEP_SOURCE_BASELINE_COMMIT,
    internalUnit: "meters",
    supportedPresets: SWEEP_PRESETS,
    frameCountLimits: { minimum: SWEEP_FRAME_LIMITS.min, maximum: SWEEP_FRAME_LIMITS.max },
    defaultFrameCount: SWEEP_FRAME_LIMITS.default,
    poseRoundingPrecisionDecimalPlaces: 6,
    progressFormula: "frameIndex / (frameCount - 1)",
    numericComparisonTolerance: SWEEP_NUMERIC_COMPARISON_TOLERANCE,
    rejectedFrameCounts: { belowMinimum: 1, aboveMaximum: 62 },
    cases,
  };
}

export const serializeMandibularSweepFixture = () => `${JSON.stringify(createMandibularSweepFixture(), null, 2)}\n`;

export function verifyMandibularSweepFixture(actual: string) {
  if (actual !== serializeMandibularSweepFixture()) {
    throw new Error(`Golden fixture drift detected: run npm run parity:generate:sweep and review ${SWEEP_FIXTURE_PATH}`);
  }
}
