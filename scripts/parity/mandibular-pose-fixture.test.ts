import { describe, expect, it } from "vitest";
import { createMandibularPoseFixture, serializeMandibularPoseFixture, verifyMandibularPoseFixture } from "./mandibular-pose-fixture";

describe("mandibular pose golden generator", () => {
  it("serializes byte-identically across calls with LF and a final newline", () => {
    const first = serializeMandibularPoseFixture();
    expect(first).toBe(serializeMandibularPoseFixture());
    expect(first.endsWith("\n")).toBe(true);
    expect(first.includes("\r")).toBe(false);
  });
  it("detects manual drift without rewriting", () => {
    expect(() => verifyMandibularPoseFixture(`${serializeMandibularPoseFixture()} `)).toThrow("Golden fixture drift detected");
  });
  it("has complete metadata and unique stable case IDs", () => {
    const fixture = createMandibularPoseFixture();
    expect(fixture).toMatchObject({ schemaVersion: 1, legacyBehaviorVersion: "mandibular-pose-v1", workerProtocolVersion: 4, internalUnit: "meters", sourceBaselineCommit: "11ca272b18467ef0a44140b14e075a311dbb6139" });
    const ids = [...fixture.validCases, ...fixture.invalidCases].map(({ id }) => id);
    expect(new Set(ids).size).toBe(ids.length);
    expect(fixture.poseLimits).toBeDefined();
    expect(fixture.numericComparisonTolerance).toBe(1e-12);
  });
});
