import { describe, expect, it, vi } from "vitest";
import { advanceLesson } from "./lesson";
import {
  PoseResponseCoordinator,
  workerBoundaryError,
  type FrameScheduler,
} from "./ui-response-gate";
import {
  NEUTRAL_POSE,
  type ContactSample,
  type MandibularPose,
  type PhysicsWorkerRequest,
  type PhysicsWorkerResponse,
} from "./worker-contract";

class DeterministicFrames implements FrameScheduler {
  private nextHandle = 1;
  private callbacks = new Map<number, () => void>();

  request(callback: () => void) {
    const handle = this.nextHandle++;
    this.callbacks.set(handle, callback);
    return handle;
  }

  cancel(handle: number) {
    this.callbacks.delete(handle);
  }

  flush() {
    const callbacks = [...this.callbacks.values()];
    this.callbacks.clear();
    callbacks.forEach((callback) => callback());
  }
}

const contact: ContactSample = {
  id: "contact-1",
  pointWorldMeters: { x: 0, y: 0, z: 0 },
  normalWorld: { x: 0, y: 1, z: 0 },
  signedDistanceMeters: 0,
  penetrationDepthMeters: 0,
  surfaces: ["maxilla", "mandible"],
  units: "meters",
};

function poseResult(
  request: Extract<PhysicsWorkerRequest, { type: "evaluate-mandibular-pose" }>,
  options: { classification?: "separated" | "touching"; pose?: MandibularPose } = {},
): Extract<PhysicsWorkerResponse, { type: "mandibular-pose-result" }> {
  const classification = options.classification ?? "separated";
  const contactSamples = classification === "touching" ? [contact] : [];
  return {
    id: request.id,
    type: "mandibular-pose-result",
    ok: true,
    fixtureId: request.fixtureId,
    sequence: request.sequence,
    requestedPose: options.pose ?? request.pose,
    appliedTransform: {
      translationMeters: { x: 0, y: -0.09, z: 0 },
      rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 },
    },
    classification,
    measurementStatus: classification === "separated" ? "unavailable-separated" : "rapier-contact",
    clearanceMeters: classification === "separated" ? null : 0,
    penetrationDepthMeters: 0,
    contactCount: contactSamples.length,
    contactSamples,
  };
}

function harness() {
  const scheduler = new DeterministicFrames();
  const dispatched: Extract<PhysicsWorkerRequest, { type: "evaluate-mandibular-pose" }>[] = [];
  const pending = vi.fn();
  const coordinator = new PoseResponseCoordinator({
    fixtureId: "fixture",
    scheduler,
    dispatch: (request) => dispatched.push(request),
    pending,
  });
  return { coordinator, scheduler, dispatched, pending };
}

describe("desired pose coordination", () => {
  it("ignores revision N after N+1 becomes desired, before N+1 is dispatched", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    coordinator.desire({ ...NEUTRAL_POSE, lateralMeters: 0.005 });

    expect(coordinator.receive(poseResult(dispatched[0]!))).toEqual({ kind: "ignored" });
  });

  it("coalesces one animation frame to only the latest desired pose", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    coordinator.desire({ ...NEUTRAL_POSE, protrusionMeters: 0.005 });
    coordinator.desire({ ...NEUTRAL_POSE, protrusionMeters: 0.01 });
    scheduler.flush();

    expect(dispatched).toHaveLength(1);
    expect(dispatched[0]).toMatchObject({ sequence: 3, pose: { protrusionMeters: 0.01 } });
  });

  it("accepts sequence gaps created by coalescing", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    coordinator.desire({ ...NEUTRAL_POSE, lateralMeters: 0.005 });
    scheduler.flush();

    expect(dispatched[0]?.sequence).toBe(2);
    expect(coordinator.receive(poseResult(dispatched[0]!)).kind).toBe("accepted");
  });

  it("clears contact/result state as soon as a newer pose is desired", () => {
    const { coordinator, pending } = harness();
    const newest = { ...NEUTRAL_POSE, lateralMeters: 0.01 };
    coordinator.desire(newest);

    expect(pending).toHaveBeenCalledOnce();
    expect(pending).toHaveBeenCalledWith(newest);
  });

  it("accepts the latest valid response exactly once", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    const response = poseResult(dispatched[0]!);

    expect(coordinator.receive(response).kind).toBe("accepted");
    expect(coordinator.receive(response)).toMatchObject({ kind: "error", message: expect.stringMatching(/uncorrelated/) });
  });
});

describe("generation and boundary hardening", () => {
  it("reset invalidates every earlier in-flight response without a fatal error", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    const oldResponse = poseResult(dispatched[0]!);

    coordinator.reset(NEUTRAL_POSE);
    expect(coordinator.currentGeneration).toBe(1);
    expect(coordinator.receive(oldResponse)).toEqual({ kind: "ignored" });
  });

  it("surfaces an unsolicited response in the current generation", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    const unsolicited = { ...poseResult(dispatched[0]!), id: "not-pending" };

    expect(coordinator.receive(unsolicited)).toMatchObject({
      kind: "error",
      message: expect.stringMatching(/uncorrelated/),
    });
  });

  it("visibly rejects malformed payloads", () => {
    const { coordinator } = harness();
    expect(coordinator.receive({ bad: true })).toMatchObject({
      kind: "error",
      message: expect.stringMatching(/invalid response/),
    });
  });

  it("rejects a validated response whose requested pose differs from its immutable dispatch snapshot", () => {
    const { coordinator, scheduler, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    const mismatched = poseResult(dispatched[0]!, {
      pose: { ...NEUTRAL_POSE, lateralMeters: 0.005 },
    });

    expect(coordinator.receive(mismatched)).toMatchObject({ kind: "error" });
  });

  it("provides safe visible errors for worker and deserialization failures", () => {
    expect(workerBoundaryError("error")).toMatch(/worker failed/);
    expect(workerBoundaryError("messageerror")).toMatch(/could not be deserialized/);
  });
});

describe("guided lesson correlation", () => {
  it("uses response.requestedPose for progression", () => {
    const { scheduler, coordinator, dispatched } = harness();
    coordinator.desire({ ...NEUTRAL_POSE, lateralMeters: 0.005 });
    scheduler.flush();
    const response = poseResult(dispatched[0]!, { classification: "touching" });

    expect(advanceLesson("contact", response)).toBe("complete");
  });

  it("a late result cannot complete the lesson using a newer slider pose", () => {
    const { scheduler, coordinator, dispatched } = harness();
    coordinator.desire(NEUTRAL_POSE);
    scheduler.flush();
    const oldResponse = poseResult(dispatched[0]!, { classification: "touching" });
    coordinator.desire({ ...NEUTRAL_POSE, lateralMeters: 0.005 });

    expect(coordinator.receive(oldResponse)).toEqual({ kind: "ignored" });
    expect(advanceLesson("contact", oldResponse)).toBe("contact");
  });
});
