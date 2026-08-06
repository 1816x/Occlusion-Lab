import { build } from "esbuild";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import { afterEach, describe, expect, it } from "vitest";
import type { CollisionMeshPayload } from "@/physics/worker-contract";
type WorkerModule = typeof import("./rapier.worker");
let tempDirectory: string | undefined;
afterEach(async () => { if (tempDirectory) { await rm(tempDirectory, { recursive: true, force: true }); tempDirectory = undefined; } });
async function loadWorkerModule() { tempDirectory = await mkdtemp(join(tmpdir(), "occlusion-phase1-worker-")); const outfile = join(tempDirectory, "rapier-worker-test.mjs"); await build({ absWorkingDir: process.cwd(), bundle: true, entryPoints: ["src/workers/rapier.worker.ts"], format: "esm", outfile, platform: "browser", sourcemap: false, tsconfig: "tsconfig.json" }); return await import(pathToFileURL(outfile).href) as WorkerModule; }
const positionsUpper = new Float32Array([-1,0.08,-1, 1,0.08,-1, 1,0.08,1, -1,0.08,1, 0,0.08,0]);
const positionsLower = new Float32Array([-1,-0.08,-1, 1,-0.08,-1, 1,-0.08,1, -1,-0.08,1, 0,-0.08,0]);
const indices = new Uint16Array([0,1,4,1,2,4,2,3,4,3,0,4]);
const meshes = (): [CollisionMeshPayload, CollisionMeshPayload] => [{ name:"OL_COLLISION_UPPER", positions: positionsUpper.buffer.slice(0), indices: indices.buffer.slice(0), indexComponentType:"uint16" }, { name:"OL_COLLISION_LOWER", positions: positionsLower.buffer.slice(0), indices: indices.buffer.slice(0), indexComponentType:"uint16" }];
describe("Phase 1 Rapier contact query", () => {
  it("returns deterministic no-contact for separated surfaces", async () => { const mod = await loadWorkerModule(); const result = await mod.runPhase1ContactQuery({ id:"t1", type:"run-phase1-contact-query", fixtureId:"phase1-opposing-occlusal-surfaces", scenario:"separated", meshes: meshes() }); expect(result).toMatchObject({ scenario:"separated", intersecting:false, contactCount:0, point:null, normal:null, penetrationDepth:null, distance:0.17 }); });
  it("returns deterministic contact for controlled penetration", async () => { const mod = await loadWorkerModule(); const result = await mod.runPhase1ContactQuery({ id:"t2", type:"run-phase1-contact-query", fixtureId:"phase1-opposing-occlusal-surfaces", scenario:"contact", meshes: meshes() }); expect(result.intersecting).toBe(true); expect(result.contactCount).toBeGreaterThan(0); expect(result.penetrationDepth ?? -1).toBeCloseTo(0, 6); expect(result.normal?.y).toBeCloseTo(1, 5); });
  it("validates generated asset manifest hash", () => { const manifest = JSON.parse(readFileSync("public/assets/fixtures/opposing-occlusal-surfaces.manifest.json", "utf8")); expect(createHash("sha256").update(readFileSync(manifest.file)).digest("hex")).toBe(manifest.sha256); });
});
