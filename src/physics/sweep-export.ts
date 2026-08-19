import {
  WORKER_PROTOCOL_VERSION,
  type SweepFrame,
  type SweepResult,
} from "./worker-contract";

export const SWEEP_EXPORT_SCHEMA_VERSION = 1 as const;

export const SWEEP_CSV_COLUMNS = [
  "frame_index",
  "progress",
  "opening_meters",
  "protrusion_meters",
  "lateral_meters",
  "translation_x_meters",
  "translation_y_meters",
  "translation_z_meters",
  "classification",
  "contact_count",
  "penetration_depth_meters",
] as const;

function exportFrame(frame: SweepFrame) {
  return {
    frameIndex: frame.frameIndex,
    progress: frame.progress,
    requestedPose: frame.requestedPose,
    appliedTransform: frame.appliedTransform,
    classification: frame.classification,
    measurementStatus: frame.measurementStatus,
    clearanceMeters: frame.clearanceMeters,
    contactCount: frame.contactCount,
    penetrationDepthMeters: frame.penetrationDepthMeters,
    contactSamples: frame.contactSamples,
  };
}

export function createSweepJsonExport(sweep: SweepResult): string {
  const document = {
    schemaVersion: SWEEP_EXPORT_SCHEMA_VERSION,
    workerProtocolVersion: WORKER_PROTOCOL_VERSION,
    fixtureId: sweep.fixtureId,
    preset: sweep.preset,
    frameCount: sweep.frameCount,
    summary: sweep.summary,
    finalPose: sweep.finalPose,
    frames: sweep.frames.map(exportFrame),
  };

  return `${JSON.stringify(document, null, 2)}\n`;
}

function escapeCsv(value: string | number): string {
  const serialized = String(value);
  return /[",\r\n]/.test(serialized) ? `"${serialized.replaceAll('"', '""')}"` : serialized;
}

export function createSweepCsvExport(sweep: SweepResult): string {
  const rows = sweep.frames.map((frame) => [
    frame.frameIndex,
    frame.progress,
    frame.requestedPose.openingMeters,
    frame.requestedPose.protrusionMeters,
    frame.requestedPose.lateralMeters,
    frame.appliedTransform.translationMeters.x,
    frame.appliedTransform.translationMeters.y,
    frame.appliedTransform.translationMeters.z,
    frame.classification,
    frame.contactCount,
    frame.penetrationDepthMeters,
  ].map(escapeCsv).join(","));

  return `${SWEEP_CSV_COLUMNS.join(",")}\n${rows.join("\n")}\n`;
}
