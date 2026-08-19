import { describe, expect, it } from "vitest";
import type { SweepResult } from "@/physics/worker-contract";
import {
  clampFrameIndex,
  classificationPresentation,
  formatMetersAsMillimeters,
  formatSweepProgress,
  selectCachedFrame,
} from "./sweep-frame-presentation";

const frame = (frameIndex: number) => ({ frameIndex }) as SweepResult["frames"][number];

describe("sweep frame presentation", () => {
  it("clamps inspection to cached frame bounds", () => {
    const sweep = { frames: [frame(0), frame(1)] } as SweepResult;
    expect(clampFrameIndex(-4, 2)).toBe(0);
    expect(clampFrameIndex(8, 2)).toBe(1);
    expect(selectCachedFrame(sweep, 8)).toBe(sweep.frames[1]);
  });

  it("formats existing values without changing their meaning", () => {
    expect(formatSweepProgress(0.125)).toBe("12.5%");
    expect(formatMetersAsMillimeters(0.00125, 3)).toBe("1.250 mm");
    expect(classificationPresentation("penetrating")).toEqual({
      label: "penetrating",
      cssClass: "penetrating",
    });
  });
});
