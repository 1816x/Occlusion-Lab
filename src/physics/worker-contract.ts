export const WORKER_PROTOCOL_VERSION = 1 as const;

export type PhysicsWorkerRequest =
  | { id: string; type: "health-check" }
  | { id: string; type: "run-synthetic-collision-fixture" };

export type PhysicsWorkerResponse =
  | { id: string; type: "health"; ok: true; protocolVersion: typeof WORKER_PROTOCOL_VERSION; rapierVersion: string; fixtureName: string }
  | { id: string; type: "synthetic-collision-result"; ok: true; fixtureName: string; collided: boolean; steps: number; finalDynamicY: number }
  | { id: string; type: "error"; ok: false; message: string };

const isRecord = (value: unknown): value is Record<string, unknown> => typeof value === "object" && value !== null;
const hasStringId = (record: Record<string, unknown>) => typeof record.id === "string" && record.id.length > 0;
const hasStringField = (record: Record<string, unknown>, field: string) => typeof record[field] === "string" && record[field].length > 0;

export function isPhysicsWorkerRequest(value: unknown): value is PhysicsWorkerRequest {
  if (!isRecord(value) || !hasStringId(value)) return false;
  return value.type === "health-check" || value.type === "run-synthetic-collision-fixture";
}

export function isPhysicsWorkerResponse(value: unknown): value is PhysicsWorkerResponse {
  if (!isRecord(value) || !hasStringId(value)) return false;

  if (value.type === "health") {
    return value.ok === true
      && value.protocolVersion === WORKER_PROTOCOL_VERSION
      && hasStringField(value, "rapierVersion")
      && hasStringField(value, "fixtureName");
  }

  if (value.type === "synthetic-collision-result") {
    return value.ok === true
      && hasStringField(value, "fixtureName")
      && typeof value.collided === "boolean"
      && typeof value.steps === "number"
      && Number.isInteger(value.steps)
      && value.steps > 0
      && typeof value.finalDynamicY === "number"
      && Number.isFinite(value.finalDynamicY);
  }

  if (value.type === "error") {
    return value.ok === false && hasStringField(value, "message");
  }

  return false;
}
