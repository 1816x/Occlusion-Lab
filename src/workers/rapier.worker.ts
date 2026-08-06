import RAPIER from "@dimforge/rapier3d-compat";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_FIXTURE_NAME, SYNTHETIC_COLLISION_STEPS, SYNTHETIC_COLLISION_TIMESTEP_SECONDS } from "@/test-fixtures/synthetic-collision";
import { WORKER_PROTOCOL_VERSION, isPhysicsWorkerRequest, type CollisionMeshPayload, type Phase1ContactResult, type PhysicsWorkerRequest, type PhysicsWorkerResponse } from "@/physics/worker-contract";

export const CONTACT_TOLERANCE_METERS = 1e-6;
let rapierReady: Promise<typeof RAPIER> | undefined;
const ensureRapier = async () => { rapierReady ??= RAPIER.init().then(() => RAPIER); return rapierReady; };
const round = (n: number) => Number(n.toFixed(6));

export async function runSyntheticCollisionFixture() { const rapier = await ensureRapier(); const world = new rapier.World({ x: 0, y: -9.81, z: 0 }); world.timestep = SYNTHETIC_COLLISION_TIMESTEP_SECONDS; const groundBody = world.createRigidBody(rapier.RigidBodyDesc.fixed().setTranslation(0, 0, 0)); world.createCollider(rapier.ColliderDesc.cuboid(2, 0.1, 2), groundBody); const sphereBody = world.createRigidBody(rapier.RigidBodyDesc.dynamic().setTranslation(0, 2, 0)); world.createCollider(rapier.ColliderDesc.ball(0.5), sphereBody); for (let step = 0; step < SYNTHETIC_COLLISION_STEPS; step += 1) world.step(); const finalDynamicY = sphereBody.translation().y; return { fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME, collided: finalDynamicY >= SYNTHETIC_COLLISION_EXPECTED.minFinalDynamicY && finalDynamicY <= SYNTHETIC_COLLISION_EXPECTED.maxFinalDynamicY, steps: SYNTHETIC_COLLISION_STEPS, finalDynamicY: round(finalDynamicY) }; }

function meshArrays(mesh: CollisionMeshPayload) {
  const positions = new Float32Array(mesh.positions);
  const indices = mesh.indexComponentType === "uint16" ? new Uint16Array(mesh.indices) : new Uint32Array(mesh.indices);
  if (positions.length < 9 || positions.length % 3 !== 0 || indices.length < 3 || indices.length % 3 !== 0 || !positions.every(Number.isFinite)) throw new Error(`Malformed collision mesh: ${mesh.name}`);
  if (!indices.every((index) => index < positions.length / 3)) throw new Error(`Out-of-range collision mesh index: ${mesh.name}`);
  return { positions, indices };
}
function colliderDesc(rapier: typeof RAPIER, mesh: CollisionMeshPayload) { const { positions, indices } = meshArrays(mesh); return rapier.ColliderDesc.trimesh(positions, Uint32Array.from(indices)); }
function verticalSurfaceGap(upper: CollisionMeshPayload, lower: CollisionMeshPayload, upperTranslationY: number) {
  const upperPositions = meshArrays(upper).positions;
  const lowerPositions = meshArrays(lower).positions;
  let lowestUpper = Infinity;
  let highestLower = -Infinity;
  for (let i = 1; i < upperPositions.length; i += 3) lowestUpper = Math.min(lowestUpper, upperPositions[i]! + upperTranslationY);
  for (let i = 1; i < lowerPositions.length; i += 3) highestLower = Math.max(highestLower, lowerPositions[i]!);
  return lowestUpper - highestLower;
}

export async function runPhase1ContactQuery(request: Extract<PhysicsWorkerRequest, { type: "run-phase1-contact-query" }>): Promise<Phase1ContactResult> {
  const rapier = await ensureRapier();
  const world = new rapier.World({ x: 0, y: 0, z: 0 });
  const upperBody = world.createRigidBody(rapier.RigidBodyDesc.dynamic().setTranslation(0, request.upperTranslationYMeters, 0));
  const lowerBody = world.createRigidBody(rapier.RigidBodyDesc.fixed());
  const upper = world.createCollider(colliderDesc(rapier, request.meshes[0]), upperBody);
  const lower = world.createCollider(colliderDesc(rapier, request.meshes[1]), lowerBody);
  world.step();
  let contactCount = 0;
  let point: Phase1ContactResult["point"] = null;
  let normal: Phase1ContactResult["normal"] = null;
  world.contactPair(upper, lower, (manifold) => {
    contactCount += manifold.numContacts();
    if (!point && manifold.numContacts() > 0) {
      const p = manifold.localContactPoint1(0);
      point = p ? { x: round(p.x), y: round(p.y), z: round(p.z) } : null;
      const n = manifold.localNormal1();
      normal = { x: round(n.x), y: round(n.y), z: round(n.z) };
    }
  });
  const gap = verticalSurfaceGap(request.meshes[0], request.meshes[1], request.upperTranslationYMeters);
  const classification = gap > CONTACT_TOLERANCE_METERS ? "separated" : gap < -CONTACT_TOLERANCE_METERS ? "penetrating" : "touching";
  return {
    fixtureId: request.fixtureId,
    scenario: request.scenario,
    classification,
    clearanceMeters: classification === "separated" ? round(gap) : 0,
    penetrationDepthMeters: classification === "penetrating" ? round(-gap) : 0,
    contactCount,
    point,
    normal,
  };
}

async function handleMessage(message: PhysicsWorkerRequest): Promise<PhysicsWorkerResponse> { const requestId = message.id; try { if (message.type === "health-check") { await ensureRapier(); return { id: requestId, type: "health", ok: true, protocolVersion: WORKER_PROTOCOL_VERSION, rapierVersion: "@dimforge/rapier3d-compat", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME }; } if (message.type === "run-synthetic-collision-fixture") return { id: requestId, type: "synthetic-collision-result", ok: true, ...(await runSyntheticCollisionFixture()) }; if (message.type === "run-phase1-contact-query") return { id: requestId, type: "phase1-contact-result", ok: true, ...(await runPhase1ContactQuery(message)) }; const exhaustive: never = message; return { id: requestId, type: "error", ok: false, message: `Unsupported physics worker request: ${JSON.stringify(exhaustive)}` }; } catch (error) { return { id: requestId, type: "error", ok: false, message: error instanceof Error ? error.message : "Unknown worker error" }; } }
const invalidRequestResponse = (value: unknown): PhysicsWorkerResponse => ({ id: isPhysicsWorkerRequest(value) ? value.id : "invalid-request", type: "error", ok: false, message: "Invalid physics worker request" });
if (typeof self !== "undefined") self.addEventListener("message", (event: MessageEvent<unknown>) => { if (!isPhysicsWorkerRequest(event.data)) { self.postMessage(invalidRequestResponse(event.data)); return; } void handleMessage(event.data).then((response) => self.postMessage(response)); });
