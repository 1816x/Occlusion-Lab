export const WORKER_PROTOCOL_VERSION = 1 as const;
export const PHASE1_SCENARIOS = ["separated", "contact"] as const;
export type Phase1Scenario = (typeof PHASE1_SCENARIOS)[number];
export type CollisionMeshPayload = { name: string; positions: ArrayBuffer; indices: ArrayBuffer; indexComponentType: "uint16" | "uint32" };
export type ContactPoint = { x: number; y: number; z: number };
export type ContactVector = ContactPoint;
export type Phase1ContactResult = { fixtureId: string; scenario: Phase1Scenario; intersecting: boolean; contactCount: number; point: ContactPoint | null; normal: ContactVector | null; penetrationDepth: number | null; distance: number | null };
export type PhysicsWorkerRequest =
  | { id: string; type: "health-check" }
  | { id: string; type: "run-synthetic-collision-fixture" }
  | { id: string; type: "run-phase1-contact-query"; fixtureId: string; scenario: Phase1Scenario; meshes: [CollisionMeshPayload, CollisionMeshPayload] };
export type PhysicsWorkerResponse =
  | { id: string; type: "health"; ok: true; protocolVersion: typeof WORKER_PROTOCOL_VERSION; rapierVersion: string; fixtureName: string }
  | { id: string; type: "synthetic-collision-result"; ok: true; fixtureName: string; collided: boolean; steps: number; finalDynamicY: number }
  | ({ id: string; type: "phase1-contact-result"; ok: true } & Phase1ContactResult)
  | { id: string; type: "error"; ok: false; message: string };
const isRecord = (value: unknown): value is Record<string, unknown> => typeof value === "object" && value !== null;
const hasStringId = (record: Record<string, unknown>) => typeof record.id === "string" && record.id.length > 0;
const hasStringField = (record: Record<string, unknown>, field: string) => typeof record[field] === "string" && record[field].length > 0;
const isScenario = (value: unknown): value is Phase1Scenario => value === "separated" || value === "contact";
const isMesh = (value: unknown): value is CollisionMeshPayload => isRecord(value) && hasStringField(value,"name") && value.positions instanceof ArrayBuffer && value.indices instanceof ArrayBuffer && (value.indexComponentType === "uint16" || value.indexComponentType === "uint32");
export function isPhysicsWorkerRequest(value: unknown): value is PhysicsWorkerRequest {
  if (!isRecord(value) || !hasStringId(value)) return false;
  if (value.type === "health-check" || value.type === "run-synthetic-collision-fixture") return true;
  return value.type === "run-phase1-contact-query" && hasStringField(value,"fixtureId") && isScenario(value.scenario) && Array.isArray(value.meshes) && value.meshes.length === 2 && value.meshes.every(isMesh);
}
const isPoint = (value: unknown): value is ContactPoint | null => value === null || (isRecord(value) && typeof value.x === "number" && typeof value.y === "number" && typeof value.z === "number");
export function isPhysicsWorkerResponse(value: unknown): value is PhysicsWorkerResponse {
  if (!isRecord(value) || !hasStringId(value)) return false;
  if (value.type === "health") return value.ok === true && value.protocolVersion === WORKER_PROTOCOL_VERSION && hasStringField(value, "rapierVersion") && hasStringField(value, "fixtureName");
  if (value.type === "synthetic-collision-result") return value.ok === true && hasStringField(value, "fixtureName") && typeof value.collided === "boolean" && Number.isInteger(value.steps) && typeof value.steps === "number" && value.steps > 0 && typeof value.finalDynamicY === "number" && Number.isFinite(value.finalDynamicY);
  if (value.type === "phase1-contact-result") return value.ok === true && hasStringField(value,"fixtureId") && isScenario(value.scenario) && typeof value.intersecting === "boolean" && Number.isInteger(value.contactCount) && isPoint(value.point) && isPoint(value.normal) && (value.penetrationDepth === null || typeof value.penetrationDepth === "number") && (value.distance === null || typeof value.distance === "number");
  if (value.type === "error") return value.ok === false && hasStringField(value, "message");
  return false;
}
