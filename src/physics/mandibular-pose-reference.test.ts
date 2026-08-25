import { describe, expect, it } from "vitest";
import { mandibularTransform } from "./mandibular-pose-reference";

describe("legacy mandibular pose reference", () => {
  it("preserves the worker-visible neutral and combined transforms", () => {
    expect(mandibularTransform({ openingMeters: 0.25, protrusionMeters: 0, lateralMeters: 0 })).toEqual({
      translationMeters: { x: 0, y: -0.09, z: 0 },
      rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
    });
    expect(mandibularTransform({ openingMeters: 0, protrusionMeters: 0.01, lateralMeters: -0.02 })).toEqual({
      translationMeters: { x: -0.02, y: 0.16, z: 0.01 },
      rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
    });
  });
});
