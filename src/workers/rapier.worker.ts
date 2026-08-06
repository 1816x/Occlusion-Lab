import RAPIER from "@dimforge/rapier3d-compat";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_FIXTURE_NAME, SYNTHETIC_COLLISION_STEPS, SYNTHETIC_COLLISION_TIMESTEP_SECONDS } from "@/test-fixtures/synthetic-collision";
import { CONTACT_DEDUPLICATION_TOLERANCE_METERS, MAX_CONTACT_SAMPLES, WORKER_PROTOCOL_VERSION, isPhysicsWorkerRequest, type CollisionMeshPayload, type ContactSample, type MandibularPose, type Phase1ContactResult, type PhysicsWorkerRequest, type PhysicsWorkerResponse, type PoseResult } from "@/physics/worker-contract";

export const CONTACT_TOLERANCE_METERS = 1e-6;
export const MANDIBLE_CLOSED_TRANSLATION_Y_METERS = 0.16;
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

type Session = { fixtureId: string; world: RAPIER.World; maxilla: RAPIER.Collider; mandible: RAPIER.Collider; mandibleBody: RAPIER.RigidBody };
let session: Session | undefined;
export const resetInteractiveSessionForTests = () => { session = undefined; };
export function mandibularTransform(pose: MandibularPose) { return { translationMeters: { x: pose.lateralMeters, y: MANDIBLE_CLOSED_TRANSLATION_Y_METERS - pose.openingMeters, z: pose.protrusionMeters }, rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 } }; }
export async function initializeOcclusionFixture(request: Extract<PhysicsWorkerRequest,{type:"initialize-occlusion-fixture"}>) {
  const rapier = await ensureRapier();
  const world = new rapier.World({x:0,y:0,z:0});
  const maxillaBody = world.createRigidBody(rapier.RigidBodyDesc.fixed());
  const mandibleBody = world.createRigidBody(rapier.RigidBodyDesc.dynamic().setGravityScale(0));
  const maxilla = world.createCollider(colliderDesc(rapier,request.meshes[0]),maxillaBody);
  const mandible = world.createCollider(colliderDesc(rapier,request.meshes[1]),mandibleBody);
  session = {fixtureId:request.fixtureId,world,maxilla,mandible,mandibleBody};
}
export const worldPoint = (
  local: { x: number; y: number; z: number },
  translation: { x: number; y: number; z: number },
  rotation = { x: 0, y: 0, z: 0, w: 1 },
) => {
  // q * v * q^-1. Rapier manifold points are collider-local, so translation
  // alone would be wrong as soon as the moving body has a non-identity pose.
  const ix = rotation.w * local.x + rotation.y * local.z - rotation.z * local.y;
  const iy = rotation.w * local.y + rotation.z * local.x - rotation.x * local.z;
  const iz = rotation.w * local.z + rotation.x * local.y - rotation.y * local.x;
  const iw = -rotation.x * local.x - rotation.y * local.y - rotation.z * local.z;
  return {
    x: round(ix * rotation.w + iw * -rotation.x + iy * -rotation.z - iz * -rotation.y + translation.x),
    y: round(iy * rotation.w + iw * -rotation.y + iz * -rotation.x - ix * -rotation.z + translation.y),
    z: round(iz * rotation.w + iw * -rotation.z + ix * -rotation.y - iy * -rotation.x + translation.z),
  };
};
const keyFor = (point:{x:number;y:number;z:number}) => [point.x,point.y,point.z].map(n=>Math.round(n/CONTACT_DEDUPLICATION_TOLERANCE_METERS)).join(":");
export async function evaluateMandibularPose(request: Extract<PhysicsWorkerRequest,{type:"evaluate-mandibular-pose"}>): Promise<PoseResult> {
  if (!session) throw new Error("not-initialized");
  if (session.fixtureId !== request.fixtureId) throw new Error("fixture-mismatch");
  const requestedTransform=mandibularTransform(request.pose);
  session.mandibleBody.setLinvel({x:0,y:0,z:0},false); session.mandibleBody.setAngvel({x:0,y:0,z:0},false);
  session.mandibleBody.setTranslation(requestedTransform.translationMeters, false);
  session.world.step();
  const applied=session.mandibleBody.translation(),rotation=session.mandibleBody.rotation();
  const transform={translationMeters:{x:round(applied.x),y:round(applied.y),z:round(applied.z)},rotationQuaternion:{x:round(rotation.x),y:round(rotation.y),z:round(rotation.z),w:round(rotation.w)}};
  const contacts: ContactSample[]=[]; const seen=new Set<string>();
  session.world.contactPair(session.maxilla,session.mandible,(manifold,flipped)=>{
    const normal=manifold.localNormal1();
    for(let i=0;i<manifold.numContacts() && contacts.length<MAX_CONTACT_SAMPLES;i++){
      const a=manifold.localContactPoint1(i), b=manifold.localContactPoint2(i); if(!a||!b) continue;
      const upperWorld=worldPoint(a,{x:0,y:0,z:0});
      const lowerWorld=worldPoint(b,transform.translationMeters,transform.rotationQuaternion);
      const point={x:round((upperWorld.x+lowerWorld.x)/2),y:round((upperWorld.y+lowerWorld.y)/2),z:round((upperWorld.z+lowerWorld.z)/2)};
      const key=keyFor(point); if(seen.has(key)) continue; seen.add(key);
      const signed=round(manifold.contactDist(i)); const n={x:round(flipped?-normal.x:normal.x),y:round(flipped?-normal.y:normal.y),z:round(flipped?-normal.z:normal.z)};
      if(![point.x,point.y,point.z,n.x,n.y,n.z,signed].every(Number.isFinite)) continue;
      contacts.push({id:`contact-${key}`,pointWorldMeters:point,normalWorld:n,signedDistanceMeters:signed,penetrationDepthMeters:round(Math.max(0,-signed)),surfaces:["maxilla","mandible"],units:"meters"});
    }
  });
  contacts.sort((a,b)=>a.pointWorldMeters.x-b.pointWorldMeters.x||a.pointWorldMeters.y-b.pointWorldMeters.y||a.pointWorldMeters.z-b.pointWorldMeters.z||a.id.localeCompare(b.id));
  const maximumContactPenetration=round(Math.max(0,...contacts.map(c=>c.penetrationDepthMeters)));
  const classification=contacts.length===0?"separated":maximumContactPenetration>CONTACT_TOLERANCE_METERS?"penetrating":"touching";
  // Touching is tolerance-normalized to zero so classification and the typed
  // measurement contract cannot disagree at the 1e-6 m boundary.
  const penetrationDepthMeters=classification==="penetrating"?maximumContactPenetration:0;
  if(classification==="touching") contacts.forEach((contact)=>{contact.penetrationDepthMeters=0;});
  return {fixtureId:request.fixtureId,sequence:request.sequence,requestedPose:request.pose,appliedTransform:transform,classification,measurementStatus:classification==="separated"?"unavailable-separated":"rapier-contact",clearanceMeters:classification==="separated"?null:0,penetrationDepthMeters,contactCount:contacts.length,contactSamples:contacts};
}

export async function handleMessage(message: PhysicsWorkerRequest): Promise<PhysicsWorkerResponse> { const requestId = message.id; try { if (message.type === "health-check") { await ensureRapier(); return { id: requestId, type: "health", ok: true, protocolVersion: WORKER_PROTOCOL_VERSION, rapierVersion: "@dimforge/rapier3d-compat", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME }; } if (message.type === "run-synthetic-collision-fixture") return { id: requestId, type: "synthetic-collision-result", ok: true, ...(await runSyntheticCollisionFixture()) }; if (message.type === "run-phase1-contact-query") return { id: requestId, type: "phase1-contact-result", ok: true, ...(await runPhase1ContactQuery(message)) }; if(message.type==="initialize-occlusion-fixture"){await initializeOcclusionFixture(message);return{id:requestId,type:"fixture-ready",ok:true,fixtureId:message.fixtureId,protocolVersion:WORKER_PROTOCOL_VERSION};} if(message.type==="evaluate-mandibular-pose") return{id:requestId,type:"mandibular-pose-result",ok:true,...await evaluateMandibularPose(message)}; const exhaustive: never = message; return { id: requestId, type: "error", ok: false, code:"invalid-request", message: `Unsupported physics worker request: ${JSON.stringify(exhaustive)}` }; } catch (error) { const message=error instanceof Error?error.message:"Unknown worker error"; const code=message==="not-initialized"||message==="fixture-mismatch"?message:"worker-error"; return { id: requestId, type: "error", ok: false, code, message: code==="not-initialized"?"Initialize the synthetic fixture before evaluating a pose.":code==="fixture-mismatch"?"The pose fixture does not match the initialized fixture.":message }; } }
const invalidRequestResponse = (value: unknown): PhysicsWorkerResponse => ({ id: isPhysicsWorkerRequest(value) ? value.id : "invalid-request", type: "error", ok: false, code:"invalid-request", message: "Invalid physics worker request" });
if (typeof self !== "undefined") self.addEventListener("message", (event: MessageEvent<unknown>) => { if (!isPhysicsWorkerRequest(event.data)) { self.postMessage(invalidRequestResponse(event.data)); return; } void handleMessage(event.data).then((response) => self.postMessage(response)); });
