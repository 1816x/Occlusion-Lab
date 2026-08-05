import RAPIER from "@dimforge/rapier3d-compat";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_FIXTURE_NAME, SYNTHETIC_COLLISION_STEPS, SYNTHETIC_COLLISION_TIMESTEP_SECONDS } from "@/test-fixtures/synthetic-collision";
import { WORKER_PROTOCOL_VERSION, type PhysicsWorkerRequest, type PhysicsWorkerResponse } from "@/physics/worker-contract";

let rapierReady: Promise<typeof RAPIER> | undefined;
const ensureRapier = async () => { rapierReady ??= RAPIER.init().then(() => RAPIER); return rapierReady; };

export async function runSyntheticCollisionFixture() {
  const rapier = await ensureRapier();
  const world = new rapier.World({ x: 0, y: -9.81, z: 0 });
  world.timestep = SYNTHETIC_COLLISION_TIMESTEP_SECONDS;
  const ground = rapier.RigidBodyDesc.fixed().setTranslation(0, 0, 0);
  const groundBody = world.createRigidBody(ground);
  world.createCollider(rapier.ColliderDesc.cuboid(2, 0.1, 2), groundBody);
  const sphere = rapier.RigidBodyDesc.dynamic().setTranslation(0, 2, 0);
  const sphereBody = world.createRigidBody(sphere);
  world.createCollider(rapier.ColliderDesc.ball(0.5), sphereBody);
  for (let step = 0; step < SYNTHETIC_COLLISION_STEPS; step += 1) world.step();
  const finalDynamicY = sphereBody.translation().y;
  return { fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME, collided: finalDynamicY >= SYNTHETIC_COLLISION_EXPECTED.minFinalDynamicY && finalDynamicY <= SYNTHETIC_COLLISION_EXPECTED.maxFinalDynamicY, steps: SYNTHETIC_COLLISION_STEPS, finalDynamicY: Number(finalDynamicY.toFixed(6)) };
}

async function handleMessage(message: PhysicsWorkerRequest): Promise<PhysicsWorkerResponse> {
  const requestId = message.id;
  try {
    if (message.type === "health-check") {
      await ensureRapier();
      return { id: requestId, type: "health", ok: true, protocolVersion: WORKER_PROTOCOL_VERSION, rapierVersion: "@dimforge/rapier3d-compat", fixtureName: SYNTHETIC_COLLISION_FIXTURE_NAME };
    }
    if (message.type === "run-synthetic-collision-fixture") return { id: requestId, type: "synthetic-collision-result", ok: true, ...(await runSyntheticCollisionFixture()) };
    const exhaustive: never = message;
    return { id: requestId, type: "error", ok: false, message: `Unsupported physics worker request: ${JSON.stringify(exhaustive)}` };
  } catch (error) { return { id: requestId, type: "error", ok: false, message: error instanceof Error ? error.message : "Unknown worker error" }; }
}

self.addEventListener("message", (event: MessageEvent<PhysicsWorkerRequest>) => { void handleMessage(event.data).then((response) => self.postMessage(response)); });
