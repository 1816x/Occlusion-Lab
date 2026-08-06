import { isPhysicsWorkerResponse, type PhysicsWorkerResponse } from "./worker-contract";
export type ResponseGate = { pendingIds: Set<string>; latestPoseSequence: number };
export type GateDecision = { response?: PhysicsWorkerResponse; error?: string };
export function acceptWorkerResponse(data: unknown, gate: ResponseGate): GateDecision {
  if (!isPhysicsWorkerResponse(data)) return {error:"The physics worker returned an invalid response. No result was applied."};
  if (!gate.pendingIds.delete(data.id)) return {error:"The physics worker returned an uncorrelated response. No result was applied."};
  if (data.type === "mandibular-pose-result" && data.sequence < gate.latestPoseSequence) return {};
  return {response:data};
}
