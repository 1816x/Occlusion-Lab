import type { MandibularPose } from "./worker-contract";

export const MANDIBLE_CLOSED_TRANSLATION_Y_METERS = 0.16;

export function mandibularTransform(pose: MandibularPose) {
  return {
    translationMeters: {
      x: pose.lateralMeters,
      y: MANDIBLE_CLOSED_TRANSLATION_Y_METERS - pose.openingMeters,
      z: pose.protrusionMeters,
    },
    rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
  };
}
