export const WORKER_PROTOCOL_VERSION = 2 as const;
export const PHASE1_SCENARIOS = ["separated", "touching"] as const;
export type Phase1Scenario = (typeof PHASE1_SCENARIOS)[number];
export type ContactClassification = "separated" | "touching" | "penetrating";
export type CollisionMeshPayload = { name: string; positions: ArrayBuffer; indices: ArrayBuffer; indexComponentType: "uint16" | "uint32" };
export type ContactPoint = { x: number; y: number; z: number };
export type ContactVector = ContactPoint;
export type Phase1ContactResult = {
  fixtureId: string;
  scenario: Phase1Scenario;
  classification: ContactClassification;
  clearanceMeters: number;
  penetrationDepthMeters: number;
  contactCount: number;
  point: ContactPoint | null;
  normal: ContactVector | null;
};
export type PhysicsWorkerRequest =
  | { id: string; type: "health-check" }
  | { id: string; type: "run-synthetic-collision-fixture" }
  | { id: string; type: "run-phase1-contact-query"; fixtureId: string; scenario: Phase1Scenario; upperTranslationYMeters: number; meshes: [CollisionMeshPayload, CollisionMeshPayload] };
export type PhysicsWorkerResponse =
  | { id: string; type: "health"; ok: true; protocolVersion: typeof WORKER_PROTOCOL_VERSION; rapierVersion: string; fixtureName: string }
  | { id: string; type: "synthetic-collision-result"; ok: true; fixtureName: string; collided: boolean; steps: number; finalDynamicY: number }
  | ({ id: string; type: "phase1-contact-result"; ok: true } & Phase1ContactResult)
  | { id: string; type: "error"; ok: false; message: string };

const isRecord = (value: unknown): value is Record<string, unknown> => typeof value === "object" && value !== null;
const hasStringId = (record: Record<string, unknown>) => typeof record.id === "string" && record.id.length > 0;
const hasStringField = (record: Record<string, unknown>, field: string) => typeof record[field] === "string" && record[field].length > 0;
const isFiniteNumber = (value: unknown): value is number => typeof value === "number" && Number.isFinite(value);
const isNonNegativeFinite = (value: unknown): value is number => isFiniteNumber(value) && value >= 0;
const isScenario = (value: unknown): value is Phase1Scenario => value === "separated" || value === "touching";
const isMesh = (value: unknown): value is CollisionMeshPayload => isRecord(value) && hasStringField(value, "name") && value.positions instanceof ArrayBuffer && value.indices instanceof ArrayBuffer && (value.indexComponentType === "uint16" || value.indexComponentType === "uint32");

export function isPhysicsWorkerRequest(value: unknown): value is PhysicsWorkerRequest {
  if (!isRecord(value) || !hasStringId(value)) return false;
  if (value.type === "health-check" || value.type === "run-synthetic-collision-fixture") return true;
  return value.type === "run-phase1-contact-query" && hasStringField(value, "fixtureId") && isScenario(value.scenario) && isFiniteNumber(value.upperTranslationYMeters) && Array.isArray(value.meshes) && value.meshes.length === 2 && value.meshes.every(isMesh);
}

const isPoint = (value: unknown): value is ContactPoint | null => value === null || (isRecord(value) && isFiniteNumber(value.x) && isFiniteNumber(value.y) && isFiniteNumber(value.z));
function measurementsMatchClassification(record: Record<string, unknown>) {
  if (!isNonNegativeFinite(record.clearanceMeters) || !isNonNegativeFinite(record.penetrationDepthMeters)) return false;
  if (record.classification === "separated") return record.clearanceMeters > 0 && record.penetrationDepthMeters === 0 && record.contactCount === 0;
  if (record.classification === "touching") return record.clearanceMeters === 0 && record.penetrationDepthMeters === 0;
  if (record.classification === "penetrating") return record.clearanceMeters === 0 && record.penetrationDepthMeters > 0;
  return false;
}

export function isPhysicsWorkerResponse(value: unknown): value is PhysicsWorkerResponse {
  if (!isRecord(value) || !hasStringId(value)) return false;
  if (value.type === "health") return value.ok === true && value.protocolVersion === WORKER_PROTOCOL_VERSION && hasStringField(value, "rapierVersion") && hasStringField(value, "fixtureName");
  if (value.type === "synthetic-collision-result") return value.ok === true && hasStringField(value, "fixtureName") && typeof value.collided === "boolean" && Number.isInteger(value.steps) && isFiniteNumber(value.steps) && value.steps > 0 && isFiniteNumber(value.finalDynamicY);
  if (value.type === "phase1-contact-result") return value.ok === true && hasStringField(value, "fixtureId") && isScenario(value.scenario) && Number.isInteger(value.contactCount) && isNonNegativeFinite(value.contactCount) && isPoint(value.point) && isPoint(value.normal) && measurementsMatchClassification(value);
  if (value.type === "error") return value.ok === false && hasStringField(value, "message");
  return false;
}
