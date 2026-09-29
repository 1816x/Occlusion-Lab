import { describe, expect, it } from "vitest";
import {
  createEvaluatedSweepSummaryFixture,
  serializeEvaluatedSweepSummaryFixture,
  verifyEvaluatedSweepSummaryFixture,
} from "../../scripts/parity/evaluated-sweep-summary-fixture";

describe("evaluated sweep summary golden generator", () => {
  it("serializes deterministically with canonical line endings", () => {
    const first = serializeEvaluatedSweepSummaryFixture();
    expect(first).toBe(serializeEvaluatedSweepSummaryFixture());
    expect(first.endsWith("\n")).toBe(true);
    expect(first).not.toContain("\r");
  });

  it("detects drift without rewriting the fixture", () => {
    expect(() => verifyEvaluatedSweepSummaryFixture(`${serializeEvaluatedSweepSummaryFixture()} `))
      .toThrow("Golden fixture drift detected");
  });

  it("covers the portable semantic rules without manifold data", () => {
    const fixture = createEvaluatedSweepSummaryFixture();
    expect(fixture).toMatchObject({
      schemaVersion: 1,
      behavioralVersion: "evaluated-sweep-summary-v1",
      contactRule: "contactCount > 0",
      maximumTieRule: "earliest frame",
      manifoldParityExcluded: true,
    });
    expect(fixture.cases).toHaveLength(8);
    expect(new Set(fixture.cases.map(({ id }) => id)).size).toBe(fixture.cases.length);
  });
});
