import { describe, expect, it } from "vitest";
import { isPhysicsWorkerRequest, isPhysicsWorkerResponse, WORKER_PROTOCOL_VERSION, type PhysicsWorkerRequest } from "./worker-contract";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_FIXTURE_NAME, SYNTHETIC_COLLISION_STEPS } from "@/test-fixtures/synthetic-collision";

describe("physics worker contract", () => {
  it("accepts typed requests", () => {
    const messages: PhysicsWorkerRequest[] = [{ id: "a", type: "health-check" }, { id: "b", type: "run-synthetic-collision-fixture" }];
    expect(messages.every(isPhysicsWorkerRequest)).toBe(true);
  });

  it("rejects malformed requests and unknown message types", () => {
    expect(isPhysicsWorkerRequest({ id: "a", type: "unknown" })).toBe(false);
    expect(isPhysicsWorkerRequest({ type: "health-check" })).toBe(false);
    expect(isPhysicsWorkerRequest({ id: "", type: "health-check" })).toBe(false);
    expect(isPhysicsWorkerRequest(null)).toBe(false);
  });

  it("accepts every typed response variant", () => {
    expect(isPhysicsWorkerResponse({ id: "1", type: "health", ok: true, protocolVersion: WORKER_PROTOCOL_VERSION, rapierVersion: "rapier", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME })).toBe(true);
    expect(isPhysicsWorkerResponse({ id: "2", type: "synthetic-collision-result", ok: true, fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME, collided: true, steps: SYNTHETIC_COLLISION_STEPS, finalDynamicY: 0.6 })).toBe(true);
    expect(isPhysicsWorkerResponse({ id: "3", type: "error", ok: false, message: "Invalid physics worker request" })).toBe(true);
  });

  it("rejects malformed responses with missing variant-specific fields", () => {
    expect(isPhysicsWorkerResponse({ id: "1", type: "health", ok: true, rapierVersion: "rapier", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME })).toBe(false);
    expect(isPhysicsWorkerResponse({ id: "2", type: "synthetic-collision-result", ok: true, fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME, collided: true, finalDynamicY: 0.6 })).toBe(false);
    expect(isPhysicsWorkerResponse({ id: "3", type: "error", ok: false })).toBe(false);
    expect(isPhysicsWorkerResponse({ id: "4", type: "unknown", ok: true })).toBe(false);
  });

  it("documents the synthetic collision fixture bounds", () => {
    expect(SYNTHETIC_COLLISION_EXPECTED.collided).toBe(true);
    expect(SYNTHETIC_COLLISION_EXPECTED.minFinalDynamicY).toBeLessThan(SYNTHETIC_COLLISION_EXPECTED.maxFinalDynamicY);
  });
});
