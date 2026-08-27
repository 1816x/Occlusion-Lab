import { describe, expect, it } from "vitest";
import { isMandibularPose, SWEEP_FRAME_LIMITS, SWEEP_PRESETS } from "./worker-contract";
import { sweepEndpoints, sweepPoses } from "./mandibular-sweep-reference";

describe("pure mandibular sweep reference", () => {
  it.each([
    ["closing", { openingMeters: 0.25, protrusionMeters: 0, lateralMeters: 0 }, { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 }],
    ["protrusive", { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 }, { openingMeters: 0, protrusionMeters: 0.05, lateralMeters: 0 }],
    ["left-lateral", { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 }, { openingMeters: 0, protrusionMeters: 0, lateralMeters: -0.05 }],
    ["right-lateral", { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 }, { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0.05 }],
  ] as const)("maps %s endpoints", (preset, start, end) => expect(sweepEndpoints(preset)).toEqual([start, end]));

  it.each([SWEEP_FRAME_LIMITS.min, SWEEP_FRAME_LIMITS.max])("accepts frame-count boundary %i", (count) => {
    for (const preset of SWEEP_PRESETS) expect(sweepPoses(preset, count)).toHaveLength(count);
  });

  it.each([SWEEP_FRAME_LIMITS.min - 1, SWEEP_FRAME_LIMITS.max + 1, 2.5, Number.NaN, Number.POSITIVE_INFINITY])(
    "rejects invalid frame count %s",
    (count) => expect(() => sweepPoses("closing", count)).toThrow(RangeError),
  );

  it.each(SWEEP_PRESETS)("generates valid, contiguous, monotonic and deterministic %s frames", (preset) => {
    const frames = sweepPoses(preset, SWEEP_FRAME_LIMITS.default);
    expect(frames).toEqual(sweepPoses(preset, SWEEP_FRAME_LIMITS.default));
    expect(frames[0]).toMatchObject({ frameIndex: 0, progress: 0, pose: sweepEndpoints(preset)[0] });
    expect(frames.at(-1)).toMatchObject({ frameIndex: frames.length - 1, progress: 1, pose: sweepEndpoints(preset)[1] });
    expect(frames.every(({ frameIndex, pose }, index) => frameIndex === index && isMandibularPose(pose))).toBe(true);
    expect(frames.every(({ progress }, index) => index === 0 || progress > frames[index - 1]!.progress)).toBe(true);
  });

  it("quantizes interpolated pose components to six decimal places without rounding progress", () => {
    const frames = sweepPoses("protrusive", 61);
    expect(frames[1]!.progress).toBe(1 / 60);
    expect(frames[1]!.pose.protrusionMeters).toBe(0.000833);
  });
});
