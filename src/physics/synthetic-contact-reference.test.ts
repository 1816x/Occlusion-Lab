import { describe, expect, it } from "vitest";
import { classifySyntheticGap, syntheticGapMeasurements, verticalSurfaceGap } from "./synthetic-contact-reference";

describe("synthetic contact reference", () => {
  it.each([[2e-6, "separated"], [1e-6, "touching"], [0, "touching"], [-1e-6, "touching"], [-2e-6, "penetrating"]] as const)("classifies %s", (gap, expected) => expect(classifySyntheticGap(gap)).toBe(expected));
  it("derives the aligned vertical gap without runtime dependencies", () => {
    const upper = { positions: [0, 1, 0, 1, 1, 0, 0, 1, 1] };
    const lower = { positions: [0, 0, 0, 1, 0, 0, 0, 0, 1] };
    expect(verticalSurfaceGap(upper, lower, -0.5)).toBe(0.5);
    expect(syntheticGapMeasurements(-0.01)).toEqual({ classification: "penetrating", clearanceMeters: 0, penetrationDepthMeters: 0.01 });
  });
});
