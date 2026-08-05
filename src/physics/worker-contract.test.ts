import { describe, expect, it } from "vitest";
import { isPhysicsWorkerResponse, WORKER_PROTOCOL_VERSION, type PhysicsWorkerRequest } from "./worker-contract";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_FIXTURE_NAME } from "@/test-fixtures/synthetic-collision";

describe("physics worker contract", () => {
  it("accepts typed health responses", () => {
    expect(isPhysicsWorkerResponse({ id: "1", type: "health", ok: true, protocolVersion: WORKER_PROTOCOL_VERSION, rapierVersion: "rapier", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME })).toBe(true);
  });
  it("rejects malformed responses", () => { expect(isPhysicsWorkerResponse({ type: "health", ok: true })).toBe(false); });
  it("defines health-check and fixture requests", () => {
    const messages: PhysicsWorkerRequest[] = [{ id: "a", type: "health-check" }, { id: "b", type: "run-synthetic-collision-fixture" }];
    expect(messages).toHaveLength(2);
    expect(SYNTHETIC_COLLISION_EXPECTED.collided).toBe(true);
  });
});
