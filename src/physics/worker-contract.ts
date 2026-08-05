export const WORKER_PROTOCOL_VERSION = 1 as const;

export type PhysicsWorkerRequest =
  | { id: string; type: "health-check" }
  | { id: string; type: "run-synthetic-collision-fixture" };

export type PhysicsWorkerResponse =
  | { id: string; type: "health"; ok: true; protocolVersion: typeof WORKER_PROTOCOL_VERSION; rapierVersion: string; fixtureName: string }
  | { id: string; type: "synthetic-collision-result"; ok: true; fixtureName: string; collided: boolean; steps: number; finalDynamicY: number }
  | { id: string; type: "error"; ok: false; message: string };

export function isPhysicsWorkerResponse(value: unknown): value is PhysicsWorkerResponse {
  if (typeof value !== "object" || value === null) return false;
  const record = value as Record<string, unknown>;
  return typeof record.id === "string" && typeof record.type === "string" && typeof record.ok === "boolean";
}
