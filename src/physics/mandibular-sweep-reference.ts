import {
  NEUTRAL_POSE,
  POSE_LIMITS,
  SWEEP_FRAME_LIMITS,
  type MandibularPose,
  type SweepPreset,
} from "./worker-contract";

export type SweepPose = {
  frameIndex: number;
  progress: number;
  pose: MandibularPose;
};

const CONTACT_POSE: MandibularPose = Object.freeze({
  openingMeters: 0,
  protrusionMeters: 0,
  lateralMeters: 0,
});

const roundPoseComponent = (value: number) => Number(value.toFixed(6));

export function sweepEndpoints(preset: SweepPreset): [MandibularPose, MandibularPose] {
  if (preset === "closing") return [{ ...NEUTRAL_POSE }, { ...CONTACT_POSE }];
  if (preset === "protrusive") {
    return [{ ...CONTACT_POSE }, { ...CONTACT_POSE, protrusionMeters: POSE_LIMITS.protrusionMeters.max }];
  }
  if (preset === "left-lateral") {
    return [{ ...CONTACT_POSE }, { ...CONTACT_POSE, lateralMeters: POSE_LIMITS.lateralMeters.min }];
  }
  return [{ ...CONTACT_POSE }, { ...CONTACT_POSE, lateralMeters: POSE_LIMITS.lateralMeters.max }];
}

export function sweepPoses(preset: SweepPreset, frameCount: number): SweepPose[] {
  if (!Number.isInteger(frameCount) || frameCount < SWEEP_FRAME_LIMITS.min || frameCount > SWEEP_FRAME_LIMITS.max) {
    throw new RangeError(`frameCount must be an integer in [${SWEEP_FRAME_LIMITS.min}, ${SWEEP_FRAME_LIMITS.max}]`);
  }
  const [start, end] = sweepEndpoints(preset);
  return Array.from({ length: frameCount }, (_, frameIndex) => {
    const progress = frameIndex / (frameCount - 1);
    const interpolate = (from: number, to: number) => roundPoseComponent(from + (to - from) * progress);
    return {
      frameIndex,
      progress,
      pose: {
        openingMeters: interpolate(start.openingMeters, end.openingMeters),
        protrusionMeters: interpolate(start.protrusionMeters, end.protrusionMeters),
        lateralMeters: interpolate(start.lateralMeters, end.lateralMeters),
      },
    };
  });
}
