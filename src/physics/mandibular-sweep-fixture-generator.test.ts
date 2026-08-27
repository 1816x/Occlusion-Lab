import { describe, expect, it } from "vitest";
import {
  createMandibularSweepFixture,
  serializeMandibularSweepFixture,
  verifyMandibularSweepFixture,
} from "../../scripts/parity/mandibular-sweep-fixture";

describe("mandibular sweep golden generator", () => {
  it("serializes byte-identically with LF and a final newline", () => {
    const first = serializeMandibularSweepFixture();
    expect(first).toBe(serializeMandibularSweepFixture());
    expect(first.endsWith("\n")).toBe(true);
    expect(first).not.toContain("\r");
  });

  it("detects manual fixture drift without rewriting", () => {
    expect(() => verifyMandibularSweepFixture(`${serializeMandibularSweepFixture()} `)).toThrow("Golden fixture drift detected");
  });

  it("contains complete metadata and 20 uniquely identified cases", () => {
    const fixture = createMandibularSweepFixture();
    expect(fixture).toMatchObject({
      schemaVersion: 1,
      legacyBehaviorVersion: "mandibular-sweep-generation-v1",
      workerProtocolVersion: 4,
      sourceImplementation: "src/physics/mandibular-sweep-reference.ts",
      sourceBaselineCommit: "39e206e48599333d2fa76947352e09be04c39dee",
      internalUnit: "meters",
      supportedPresets: ["closing", "protrusive", "left-lateral", "right-lateral"],
      frameCountLimits: { minimum: 2, maximum: 61 },
      defaultFrameCount: 31,
      poseRoundingPrecisionDecimalPlaces: 6,
      progressFormula: "frameIndex / (frameCount - 1)",
      numericComparisonTolerance: 1e-12,
      rejectedFrameCounts: { belowMinimum: 1, aboveMaximum: 62 },
    });
    expect(fixture.cases).toHaveLength(20);
    const ids = fixture.cases.map(({ id }) => id);
    expect(new Set(ids).size).toBe(ids.length);
  });
});
