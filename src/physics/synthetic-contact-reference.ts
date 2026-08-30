export const CONTACT_TOLERANCE_METERS = 1e-6;

export type SyntheticContactClassification = "separated" | "touching" | "penetrating";
export type IndexedMeshPositions = { positions: ArrayLike<number> };

export function verticalSurfaceGap(
  upper: IndexedMeshPositions,
  lower: IndexedMeshPositions,
  upperTranslationY: number,
): number {
  if (!Number.isFinite(upperTranslationY)) throw new Error("upperTranslationY must be finite");
  if (upper.positions.length < 9 || upper.positions.length % 3 !== 0 ||
      lower.positions.length < 9 || lower.positions.length % 3 !== 0 ||
      !Array.from(upper.positions).every(Number.isFinite) || !Array.from(lower.positions).every(Number.isFinite)) {
    throw new Error("Mesh positions must contain finite XYZ triples");
  }
  let lowestUpper = Infinity;
  let highestLower = -Infinity;
  for (let index = 1; index < upper.positions.length; index += 3)
    lowestUpper = Math.min(lowestUpper, upper.positions[index]! + upperTranslationY);
  for (let index = 1; index < lower.positions.length; index += 3)
    highestLower = Math.max(highestLower, lower.positions[index]!);
  return lowestUpper - highestLower;
}

export function classifySyntheticGap(gapMeters: number): SyntheticContactClassification {
  if (!Number.isFinite(gapMeters)) throw new Error("gapMeters must be finite");
  return gapMeters > CONTACT_TOLERANCE_METERS
    ? "separated"
    : gapMeters < -CONTACT_TOLERANCE_METERS
      ? "penetrating"
      : "touching";
}

export function syntheticGapMeasurements(gapMeters: number) {
  const classification = classifySyntheticGap(gapMeters);
  return {
    classification,
    clearanceMeters: classification === "separated" ? gapMeters : 0,
    penetrationDepthMeters: classification === "penetrating" ? -gapMeters : 0,
  };
}
