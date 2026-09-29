import { summarizeSweep, type SweepSummaryInput } from "../../src/workers/sweep-science";

export const SUMMARY_FIXTURE_PATH = "fixtures/parity/evaluated-sweep-summary-v1.json";
export const SUMMARY_SOURCE_BASELINE_COMMIT = "2ec46d13a16ad197a397a90bae6d6e29c7016de1";

const definitions: ReadonlyArray<{ id: string; contacts: number[]; depths: number[] }> = [
  { id: "no-contact-two-frames", contacts: [0, 0], depths: [0, 0] },
  { id: "touching-first-only", contacts: [2, 0, 0], depths: [0, 0, 0] },
  { id: "touching-final-only", contacts: [0, 0, 1], depths: [0, 0, 0] },
  { id: "discontinuous-contact", contacts: [0, 1, 0, 3, 0], depths: [0, 0, 0, 0, 0] },
  { id: "penetration-maximum-middle", contacts: [0, 1, 2, 1], depths: [0, 0.001, 0.004, 0.002] },
  { id: "equal-maximum-keeps-earliest", contacts: [1, 1, 1], depths: [0.003, 0.003, 0.001] },
  { id: "depth-without-manifold-contact", contacts: [0, 0, 0], depths: [0, 0.002, 0] },
  { id: "all-contact-with-zero-depth", contacts: [1, 4, 1, 2], depths: [0, 0, 0, 0] },
];

export function createEvaluatedSweepSummaryFixture() {
  const cases = definitions.map(({ id, contacts, depths }) => {
    const frames: SweepSummaryInput[] = contacts.map((contactCount, frameIndex) => ({
      frameIndex,
      contactCount,
      penetrationDepthMeters: depths[frameIndex]!,
    }));
    return { id, frames, expectedSummary: summarizeSweep(frames) };
  });
  return {
    schemaVersion: 1,
    behavioralVersion: "evaluated-sweep-summary-v1",
    sourceImplementation: "src/workers/sweep-science.ts",
    sourceBaselineCommit: SUMMARY_SOURCE_BASELINE_COMMIT,
    internalUnit: "meters",
    contactRule: "contactCount > 0",
    maximumTieRule: "earliest frame",
    manifoldParityExcluded: true,
    cases,
  };
}

export const serializeEvaluatedSweepSummaryFixture = () =>
  `${JSON.stringify(createEvaluatedSweepSummaryFixture(), null, 2)}\n`;

export function verifyEvaluatedSweepSummaryFixture(actual: string) {
  if (actual !== serializeEvaluatedSweepSummaryFixture()) {
    throw new Error(`Golden fixture drift detected: run npm run parity:generate:summary and review ${SUMMARY_FIXTURE_PATH}`);
  }
}
