export const WORKER_PROTOCOL_VERSION = 4 as const;
export const PHASE1_SCENARIOS = ["separated", "touching"] as const;
export type Phase1Scenario = (typeof PHASE1_SCENARIOS)[number];
export type ContactClassification = "separated" | "touching" | "penetrating";
export const POSE_LIMITS = Object.freeze({ openingMeters: { min: 0, max: 0.25 }, protrusionMeters: { min: 0, max: 0.05 }, lateralMeters: { min: -0.05, max: 0.05 } });
export const NEUTRAL_POSE: MandibularPose = Object.freeze({ openingMeters: 0.25, protrusionMeters: 0, lateralMeters: 0 });
export const MAX_CONTACT_SAMPLES = 32;
export const SWEEP_PRESETS = ["closing", "protrusive", "left-lateral", "right-lateral"] as const;
export type SweepPreset = (typeof SWEEP_PRESETS)[number];
export const SWEEP_FRAME_LIMITS = Object.freeze({ min: 2, max: 61, default: 31 });
export const CONTACT_DEDUPLICATION_TOLERANCE_METERS = 1e-5;
export type MandibularPose = { openingMeters: number; protrusionMeters: number; lateralMeters: number };
export type CollisionMeshPayload = { name: string; positions: ArrayBuffer; indices: ArrayBuffer; indexComponentType: "uint16" | "uint32" };
export type Vector3 = { x: number; y: number; z: number };
export type Quaternion = { x: number; y: number; z: number; w: number };
export type AppliedTransform = { translationMeters: Vector3; rotationQuaternion: Quaternion };
export type ContactSample = { id: string; pointWorldMeters: Vector3; normalWorld: Vector3; signedDistanceMeters: number; penetrationDepthMeters: number; surfaces: ["maxilla", "mandible"]; units: "meters" };
export type PoseResult = { fixtureId: string; sequence: number; requestedPose: MandibularPose; appliedTransform: AppliedTransform; classification: ContactClassification; measurementStatus: "rapier-contact" | "unavailable-separated"; clearanceMeters: number | null; penetrationDepthMeters: number; contactCount: number; contactSamples: ContactSample[] };
export type SweepFrame = Omit<PoseResult, "fixtureId" | "sequence"> & { frameIndex: number; progress: number };
export type SweepSummary = { totalFrameCount: number; firstContactFrame: number | null; lastContactFrame: number | null; contactFrameCount: number; maximumPenetrationMeters: number; maximumPenetrationFrame: number; contactPersistsThroughFinalFrame: boolean };
export type SweepResult = { fixtureId: string; sequence: number; preset: SweepPreset; frameCount: number; finalPose: MandibularPose; frames: SweepFrame[]; summary: SweepSummary };
export type Phase1ContactResult = { fixtureId: string; scenario: Phase1Scenario; classification: ContactClassification; clearanceMeters: number; penetrationDepthMeters: number; contactCount: number; point: Vector3 | null; normal: Vector3 | null };
export type PhysicsWorkerRequest =
  | { id: string; type: "health-check" }
  | { id: string; type: "run-synthetic-collision-fixture" }
  | { id: string; type: "run-phase1-contact-query"; fixtureId: string; scenario: Phase1Scenario; upperTranslationYMeters: number; meshes: [CollisionMeshPayload, CollisionMeshPayload] }
  | { id: string; type: "initialize-occlusion-fixture"; fixtureId: string; meshes: [CollisionMeshPayload, CollisionMeshPayload] }
  | { id: string; type: "evaluate-mandibular-pose"; fixtureId: string; sequence: number; pose: MandibularPose }
  | { id: string; type: "evaluate-mandibular-sweep"; fixtureId: string; sequence: number; preset: SweepPreset; frameCount: number };
export type PhysicsWorkerResponse =
  | { id: string; type: "health"; ok: true; protocolVersion: typeof WORKER_PROTOCOL_VERSION; rapierVersion: string; fixtureName: string }
  | { id: string; type: "synthetic-collision-result"; ok: true; fixtureName: string; collided: boolean; steps: number; finalDynamicY: number }
  | ({ id: string; type: "phase1-contact-result"; ok: true } & Phase1ContactResult)
  | { id: string; type: "fixture-ready"; ok: true; fixtureId: string; protocolVersion: typeof WORKER_PROTOCOL_VERSION }
  | ({ id: string; type: "mandibular-pose-result"; ok: true } & PoseResult)
  | ({ id: string; type: "mandibular-sweep-result"; ok: true } & SweepResult)
  | { id: string; type: "error"; ok: false; code: "invalid-request" | "not-initialized" | "fixture-mismatch" | "worker-error"; message: string };

const record = (v: unknown): v is Record<string, unknown> => typeof v === "object" && v !== null;
const finite = (v: unknown): v is number => typeof v === "number" && Number.isFinite(v);
const stringField = (r: Record<string, unknown>, key: string) => typeof r[key] === "string" && (r[key] as string).length > 0;
const vector = (v: unknown): v is Vector3 => record(v) && finite(v.x) && finite(v.y) && finite(v.z);
const quaternion = (v: unknown): v is Quaternion => record(v) && finite(v.x) && finite(v.y) && finite(v.z) && finite(v.w) && Math.abs(Math.hypot(v.x, v.y, v.z, v.w) - 1) <= 1e-6;
const mesh = (v: unknown): v is CollisionMeshPayload => record(v) && stringField(v,"name") && v.positions instanceof ArrayBuffer && v.indices instanceof ArrayBuffer && (v.indexComponentType === "uint16" || v.indexComponentType === "uint32");
export const isMandibularPose = (v: unknown): v is MandibularPose => record(v) && finite(v.openingMeters) && finite(v.protrusionMeters) && finite(v.lateralMeters) && v.openingMeters >= POSE_LIMITS.openingMeters.min && v.openingMeters <= POSE_LIMITS.openingMeters.max && v.protrusionMeters >= POSE_LIMITS.protrusionMeters.min && v.protrusionMeters <= POSE_LIMITS.protrusionMeters.max && v.lateralMeters >= POSE_LIMITS.lateralMeters.min && v.lateralMeters <= POSE_LIMITS.lateralMeters.max;
const base = (v: unknown): v is Record<string, unknown> => record(v) && stringField(v,"id");
export function isPhysicsWorkerRequest(v: unknown): v is PhysicsWorkerRequest { if (!base(v)) return false; if (v.type === "health-check" || v.type === "run-synthetic-collision-fixture") return true; if (v.type === "run-phase1-contact-query") return stringField(v,"fixtureId") && (v.scenario === "separated" || v.scenario === "touching") && finite(v.upperTranslationYMeters) && Array.isArray(v.meshes) && v.meshes.length === 2 && v.meshes.every(mesh); if (v.type === "initialize-occlusion-fixture") return stringField(v,"fixtureId") && Array.isArray(v.meshes) && v.meshes.length === 2 && v.meshes.every(mesh); if (v.type === "evaluate-mandibular-sweep") return stringField(v,"fixtureId") && Number.isSafeInteger(v.sequence) && (v.sequence as number)>=0 && (SWEEP_PRESETS as readonly unknown[]).includes(v.preset) && Number.isInteger(v.frameCount) && (v.frameCount as number)>=SWEEP_FRAME_LIMITS.min && (v.frameCount as number)<=SWEEP_FRAME_LIMITS.max; return v.type === "evaluate-mandibular-pose" && stringField(v,"fixtureId") && Number.isSafeInteger(v.sequence) && (v.sequence as number) >= 0 && isMandibularPose(v.pose); }
const classification = (v: unknown): v is ContactClassification => v === "separated" || v === "touching" || v === "penetrating";
const sample = (v: unknown): v is ContactSample => record(v) && stringField(v,"id") && vector(v.pointWorldMeters) && vector(v.normalWorld) && finite(v.signedDistanceMeters) && finite(v.penetrationDepthMeters) && v.penetrationDepthMeters >= 0 && Array.isArray(v.surfaces) && v.surfaces[0] === "maxilla" && v.surfaces[1] === "mandible" && v.units === "meters";
function validPoseResult(v: Record<string,unknown>) {
  if (!stringField(v,"fixtureId") || !Number.isSafeInteger(v.sequence) || !isMandibularPose(v.requestedPose) || !record(v.appliedTransform) || !vector(v.appliedTransform.translationMeters) || !quaternion(v.appliedTransform.rotationQuaternion) || !classification(v.classification) || !finite(v.penetrationDepthMeters) || (v.penetrationDepthMeters as number) < 0 || !Number.isSafeInteger(v.contactCount) || (v.contactCount as number) < 0 || !Array.isArray(v.contactSamples) || v.contactSamples.length > MAX_CONTACT_SAMPLES || !v.contactSamples.every(sample)) return false;
  if (v.measurementStatus === "unavailable-separated") return v.classification === "separated" && v.clearanceMeters === null && v.penetrationDepthMeters === 0 && v.contactCount === 0 && v.contactSamples.length === 0;
  if (v.measurementStatus !== "rapier-contact" || v.clearanceMeters !== 0 || v.classification === "separated" || v.contactCount !== v.contactSamples.length) return false;
  const maximumSamplePenetration = Math.max(0, ...(v.contactSamples as ContactSample[]).map((contact) => contact.penetrationDepthMeters));
  return v.classification === "penetrating"
    ? v.penetrationDepthMeters === maximumSamplePenetration && maximumSamplePenetration > 0
    : v.penetrationDepthMeters === 0 && maximumSamplePenetration === 0;
}
function validSweep(v: Record<string, unknown>) {
  if (!stringField(v,"fixtureId") || !Number.isSafeInteger(v.sequence) || !(SWEEP_PRESETS as readonly unknown[]).includes(v.preset) || !Number.isInteger(v.frameCount) || !Array.isArray(v.frames) || v.frames.length !== v.frameCount || v.frames.length < SWEEP_FRAME_LIMITS.min || v.frames.length > SWEEP_FRAME_LIMITS.max || !isMandibularPose(v.finalPose) || !record(v.summary)) return false;
  const frames=v.frames as Record<string,unknown>[];
  if (!frames.every((f,i)=>record(f) && f.frameIndex===i && finite(f.progress) && f.progress>=0 && f.progress<=1 && (i===0 ? f.progress===0 : (f.progress as number)>(frames[i-1]!.progress as number)) && validPoseResult({...f,fixtureId:v.fixtureId,sequence:v.sequence}))) return false;
  if (frames.at(-1)!.progress!==1 || JSON.stringify(frames.at(-1)!.requestedPose)!==JSON.stringify(v.finalPose)) return false;
  const contacts=frames.filter(f=>(f.contactCount as number)>0).map(f=>f.frameIndex as number); let maximum=0, maximumFrame=0; frames.forEach((f,i)=>{if((f.penetrationDepthMeters as number)>maximum){maximum=f.penetrationDepthMeters as number;maximumFrame=i;}}); const s=v.summary;
  return s.totalFrameCount===frames.length && s.firstContactFrame===(contacts[0]??null) && s.lastContactFrame===(contacts.at(-1)??null) && s.contactFrameCount===contacts.length && s.maximumPenetrationMeters===maximum && s.maximumPenetrationFrame===maximumFrame && s.contactPersistsThroughFinalFrame===(contacts.at(-1)===frames.length-1);
}
export function isPhysicsWorkerResponse(v: unknown): v is PhysicsWorkerResponse { if (!base(v)) return false; if (v.type === "health") return v.ok === true && v.protocolVersion === WORKER_PROTOCOL_VERSION && stringField(v,"rapierVersion") && stringField(v,"fixtureName"); if (v.type === "synthetic-collision-result") return v.ok === true && stringField(v,"fixtureName") && typeof v.collided === "boolean" && Number.isInteger(v.steps) && (v.steps as number)>0 && finite(v.finalDynamicY); if (v.type === "fixture-ready") return v.ok === true && stringField(v,"fixtureId") && v.protocolVersion === WORKER_PROTOCOL_VERSION; if (v.type === "mandibular-pose-result") return v.ok === true && validPoseResult(v); if (v.type === "mandibular-sweep-result") return v.ok === true && validSweep(v); if (v.type === "error") return v.ok === false && ["invalid-request","not-initialized","fixture-mismatch","worker-error"].includes(v.code as string) && stringField(v,"message"); if (v.type === "phase1-contact-result") { const common=v.ok === true && stringField(v,"fixtureId") && classification(v.classification) && finite(v.clearanceMeters) && (v.clearanceMeters as number)>=0 && finite(v.penetrationDepthMeters) && (v.penetrationDepthMeters as number)>=0 && Number.isInteger(v.contactCount) && (v.contactCount as number)>=0 && (v.point===null||vector(v.point)) && (v.normal===null||vector(v.normal)); if(!common)return false; if(v.classification==="separated")return (v.clearanceMeters as number)>0&&v.penetrationDepthMeters===0&&v.contactCount===0; if(v.classification==="touching")return v.clearanceMeters===0&&v.penetrationDepthMeters===0; return v.clearanceMeters===0&&(v.penetrationDepthMeters as number)>0; } return false; }
export const metersToMillimeters = (meters: number) => meters * 1000;
