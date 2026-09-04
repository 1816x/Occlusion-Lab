export const EVALUATED_SWEEP_SUMMARY_FIXTURE_PATH =
  "fixtures/parity/evaluated-sweep-summary-v1.json";

type State = "separated" | "touching" | "penetrating";
type Input = { classification: State; penetrationDepthMeters: number };
const separated = (): Input => ({ classification: "separated", penetrationDepthMeters: 0 });
const touching = (): Input => ({ classification: "touching", penetrationDepthMeters: 0 });
const penetrating = (depth: number): Input => ({
  classification: "penetrating",
  penetrationDepthMeters: depth,
});
const repeat = (count: number, make: (index: number) => Input) =>
  Array.from({ length: count }, (_, index) => make(index));

// Dependency-free reference preserving summarizeSweep()'s ordering, strict-greater tie,
// contact-count, and final-frame semantics. Native absence is serialized as JSON null.
const summarize = (frames: Input[]) => {
  const contacts = frames
    .map((frame, frameIndex) => ({ ...frame, frameIndex }))
    .filter((frame) => frame.classification !== "separated")
    .map((frame) => frame.frameIndex);
  let maximumPenetrationMeters = 0;
  let maximumPenetrationFrame: number | null = null;
  frames.forEach((frame, frameIndex) => {
    if (frame.penetrationDepthMeters > maximumPenetrationMeters) {
      maximumPenetrationMeters = frame.penetrationDepthMeters;
      maximumPenetrationFrame = frameIndex;
    }
  });
  return {
    totalFrameCount: frames.length,
    firstContactFrame: contacts[0] ?? null,
    lastContactFrame: contacts.at(-1) ?? null,
    contactFrameCount: contacts.length,
    maximumPenetrationMeters,
    maximumPenetrationFrame,
    contactPersistsThroughFinalFrame: contacts.at(-1) === frames.length - 1,
  };
};

const definitions: { id: string; frames: Input[] }[] = [
  { id: "no-contact", frames: repeat(3, separated) },
  { id: "all-touching", frames: repeat(3, touching) },
  { id: "all-penetrating", frames: repeat(3, (i) => penetrating((i + 1) * 0.001)) },
  { id: "contact-begins-midway", frames: [separated(), separated(), touching(), penetrating(0.002)] },
  { id: "contact-ends-before-final", frames: [touching(), penetrating(0.003), separated()] },
  { id: "contact-persists-final", frames: [separated(), touching(), touching()] },
  { id: "multiple-penetration-peaks", frames: [penetrating(0.001), penetrating(0.004), penetrating(0.002), penetrating(0.006)] },
  { id: "equal-maximum-earliest", frames: [penetrating(0.005), penetrating(0.002), penetrating(0.005)] },
  { id: "mixed-states", frames: [separated(), touching(), penetrating(0.003), separated(), touching()] },
  { id: "minimum-two-frames", frames: [separated(), touching()] },
  { id: "default-31-frames", frames: repeat(31, (i) => (i < 15 ? separated() : penetrating((i - 14) * 0.0001))) },
  { id: "maximum-61-frames", frames: repeat(61, (i) => (i % 3 === 0 ? penetrating(0.001 + i * 0.00001) : i % 3 === 1 ? touching() : separated())) },
];

export const serializeEvaluatedSweepSummaryFixture = () =>
  `${JSON.stringify(
    {
      schemaVersion: "1.0.0",
      units: { penetrationDepth: "meters", normalizedProgress: "unitless [0, 1]" },
      sourceBaselineCommit: "46ffc9524e781fe2e8d8c269027434f422c2abf7",
      parityScope: "deterministic sweep summary semantics; excludes live collision manifolds",
      cases: definitions.map(({ id, frames }) => ({ id, frames, expected: summarize(frames) })),
    },
    null,
    2,
  )}\n`;

export const verifyEvaluatedSweepSummaryFixture = (actual: string) => {
  const expected = serializeEvaluatedSweepSummaryFixture();
  if (actual !== expected)
    throw new Error("Evaluated sweep summary fixture drift; run parity:generate only for an intentional rebaseline");
};
