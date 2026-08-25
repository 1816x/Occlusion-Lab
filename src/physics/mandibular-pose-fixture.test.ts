import { readFileSync } from "node:fs";
import { describe, expect, it } from "vitest";
import { createMandibularPoseFixture, serializeMandibularPoseFixture, verifyMandibularPoseFixture } from "../../scripts/parity/mandibular-pose-fixture";
import { isMandibularPose } from "./worker-contract";

describe("versioned mandibular pose fixture", () => {
  it("is canonical, deterministic, LF-only, and drift-sensitive", () => {
    const tracked = readFileSync("fixtures/parity/mandibular-pose-v1.json", "utf8");
    expect(serializeMandibularPoseFixture()).toBe(serializeMandibularPoseFixture());
    expect(tracked.endsWith("\n")).toBe(true);
    expect(tracked.includes("\r")).toBe(false);
    expect(() => verifyMandibularPoseFixture(tracked)).not.toThrow();
    expect(() => verifyMandibularPoseFixture(`${tracked} `)).toThrow("Golden fixture drift detected");
  });
  it("contains complete metadata and unique IDs", () => {
    const fixture = createMandibularPoseFixture();
    expect(fixture).toMatchObject({ schemaVersion: 1, legacyBehaviorVersion: "mandibular-pose-v1", workerProtocolVersion: 4, internalUnit: "meters", sourceBaselineCommit: "11ca272b18467ef0a44140b14e075a311dbb6139", closedMandibleTranslationYMeters: 0.16, numericComparisonTolerance: 1e-12 });
    const ids = [...fixture.validCases, ...fixture.invalidCases].map(({ id }) => id);
    expect(new Set(ids).size).toBe(ids.length);
  });
  it("accepts every exact boundary case and rejects finite out-of-range cases", () => {
    const { validCases, invalidCases } = createMandibularPoseFixture();
    for (const fixtureCase of validCases) expect(isMandibularPose(fixtureCase.inputPose), fixtureCase.id).toBe(true);
    for (const fixtureCase of invalidCases) expect(isMandibularPose(fixtureCase.inputPose), fixtureCase.id).toBe(false);
  });
});
