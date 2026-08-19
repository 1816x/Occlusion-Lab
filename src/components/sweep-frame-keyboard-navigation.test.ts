import { describe, expect, it } from "vitest";
import {
  sweepFrameForNavigationKey,
  sweepPageJump,
} from "./sweep-frame-keyboard-navigation";

describe("sweep frame keyboard navigation", () => {
  it.each([
    ["ArrowLeft", 4],
    ["ArrowDown", 4],
    ["ArrowRight", 6],
    ["ArrowUp", 6],
    ["Home", 0],
    ["End", 20],
    ["PageUp", 7],
    ["PageDown", 3],
  ])("maps %s to its frame", (key, expected) => {
    expect(sweepFrameForNavigationKey(key, 5, 21)).toBe(expected);
  });

  it("clamps every navigation command to the frame range", () => {
    expect(sweepFrameForNavigationKey("ArrowLeft", 0, 11)).toBe(0);
    expect(sweepFrameForNavigationKey("PageDown", 0, 11)).toBe(0);
    expect(sweepFrameForNavigationKey("ArrowRight", 10, 11)).toBe(10);
    expect(sweepFrameForNavigationKey("PageUp", 10, 11)).toBe(10);
    expect(sweepFrameForNavigationKey("Home", 20, 11)).toBe(0);
    expect(sweepFrameForNavigationKey("End", -4, 11)).toBe(10);
  });

  it("uses approximately ten percent of the sweep and at least one frame for page jumps", () => {
    expect(sweepPageJump(61)).toBe(6);
    expect(sweepPageJump(31)).toBe(3);
    expect(sweepPageJump(2)).toBe(1);
    expect(sweepPageJump(1)).toBe(1);
  });

  it("returns null for unknown keys and invalid empty sweeps", () => {
    expect(sweepFrameForNavigationKey("Enter", 5, 11)).toBeNull();
    expect(sweepFrameForNavigationKey("ArrowRight", 0, 0)).toBeNull();
  });

  it("keeps every recognized key on the only frame in a single-frame sweep", () => {
    for (const key of ["ArrowLeft", "ArrowDown", "ArrowRight", "ArrowUp", "Home", "End", "PageUp", "PageDown"]) {
      expect(sweepFrameForNavigationKey(key, 0, 1)).toBe(0);
    }
  });
});
