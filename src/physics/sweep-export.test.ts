import { describe, expect, it } from "vitest";
import type { SweepResult } from "./worker-contract";
import { createSweepCsvExport, createSweepJsonExport } from "./sweep-export";

export const completedSweep: SweepResult = {
  fixtureId: "fixture-a",
  sequence: 7,
  preset: "closing",
  frameCount: 2,
  finalPose: { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 },
  summary: {
    totalFrameCount: 2,
    firstContactFrame: 1,
    lastContactFrame: 1,
    contactFrameCount: 1,
    maximumPenetrationMeters: 0,
    maximumPenetrationFrame: 0,
    contactPersistsThroughFinalFrame: true,
  },
  frames: [
    {
      frameIndex: 0,
      progress: 0,
      requestedPose: { openingMeters: 0.25, protrusionMeters: 0, lateralMeters: 0 },
      appliedTransform: {
        translationMeters: { x: 0, y: -0.25, z: 0 },
        rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
      },
      classification: "separated",
      measurementStatus: "unavailable-separated",
      clearanceMeters: null,
      contactCount: 0,
      penetrationDepthMeters: 0,
      contactSamples: [],
    },
    {
      frameIndex: 1,
      progress: 1,
      requestedPose: { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 },
      appliedTransform: {
        translationMeters: { x: 0, y: 0, z: 0 },
        rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
      },
      classification: "touching",
      measurementStatus: "rapier-contact",
      clearanceMeters: 0,
      contactCount: 1,
      penetrationDepthMeters: 0,
      contactSamples: [{
        id: "contact-0",
        pointWorldMeters: { x: 0.1, y: 0, z: -0.1 },
        normalWorld: { x: 0, y: 1, z: 0 },
        signedDistanceMeters: 0,
        penetrationDepthMeters: 0,
        surfaces: ["maxilla", "mandible"],
        units: "meters",
      }],
    },
  ],
};

describe("deterministic sweep JSON export", () => {
  it("uses a versioned, ordered schema and preserves Worker values", () => {
    const output = createSweepJsonExport(completedSweep);
    const parsed = JSON.parse(output);

    expect(Object.keys(parsed)).toEqual([
      "schemaVersion", "workerProtocolVersion", "fixtureId", "preset", "frameCount",
      "summary", "finalPose", "frames",
    ]);
    expect(parsed.schemaVersion).toBe(1);
    expect(parsed.workerProtocolVersion).toBe(4);
    expect(parsed.frames).toHaveLength(2);
    expect(parsed.frames[1].contactSamples).toEqual(completedSweep.frames[1]!.contactSamples);
    expect(parsed.frames[1].appliedTransform).toEqual(completedSweep.frames[1]!.appliedTransform);
    expect(output.endsWith("\n")).toBe(true);
    expect(output.includes("\n  \"workerProtocolVersion\"")).toBe(true);
  });

  it("is byte-identical for identical completed results", () => {
    expect(createSweepJsonExport(completedSweep)).toBe(createSweepJsonExport(completedSweep));
    expect(createSweepJsonExport(completedSweep)).not.toContain("sequence");
  });
});

describe("deterministic sweep CSV export", () => {
  it("uses fixed English columns and one compact row per frame", () => {
    expect(createSweepCsvExport(completedSweep)).toBe(
      "frame_index,progress,opening_meters,protrusion_meters,lateral_meters,translation_x_meters,translation_y_meters,translation_z_meters,classification,contact_count,penetration_depth_meters\n" +
      "0,0,0.25,0,0,0,-0.25,0,separated,0,0\n" +
      "1,1,0,0,0,0,0,0,touching,1,0\n",
    );
  });

  it("is byte-identical, LF terminated, and excludes contact samples", () => {
    const output = createSweepCsvExport(completedSweep);
    expect(output).toBe(createSweepCsvExport(completedSweep));
    expect(output).not.toContain("\r");
    expect(output.endsWith("\n")).toBe(true);
    expect(output).not.toContain("contact-0");
  });
});
