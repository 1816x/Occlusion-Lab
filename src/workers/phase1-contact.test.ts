import { build } from "esbuild";
import { createHash } from "node:crypto";
import { readFileSync } from "node:fs";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import { afterEach, describe, expect, it } from "vitest";
import type { CollisionMeshPayload, Phase1Scenario } from "@/physics/worker-contract";
type WorkerModule = typeof import("./rapier.worker");
let tempDirectory: string | undefined;
afterEach(async () => { if (tempDirectory) { await rm(tempDirectory, { recursive: true, force: true }); tempDirectory = undefined; } });
async function loadWorkerModule() { tempDirectory = await mkdtemp(join(tmpdir(), "occlusion-phase1-worker-")); const outfile = join(tempDirectory, "rapier-worker-test.mjs"); await build({ absWorkingDir: process.cwd(), bundle: true, entryPoints: ["src/workers/rapier.worker.ts"], format: "esm", outfile, platform: "browser", sourcemap: false, tsconfig: "tsconfig.json" }); return await import(pathToFileURL(outfile).href) as WorkerModule; }
const upperY = 0.08;
const lowerY = -0.08;
const positionsUpper = new Float32Array([-1,upperY,-1, 1,upperY,-1, 1,upperY,1, -1,upperY,1, 0,upperY,0]);
const positionsLower = new Float32Array([-1,lowerY,-1, 1,lowerY,-1, 1,lowerY,1, -1,lowerY,1, 0,lowerY,0]);
const indices = new Uint16Array([0,1,4,1,2,4,2,3,4,3,0,4]);
const meshes = (): [CollisionMeshPayload, CollisionMeshPayload] => [{ name:"OL_COLLISION_UPPER", positions: positionsUpper.buffer.slice(0), indices: indices.buffer.slice(0), indexComponentType:"uint16" }, { name:"OL_COLLISION_LOWER", positions: positionsLower.buffer.slice(0), indices: indices.buffer.slice(0), indexComponentType:"uint16" }];
const request = (scenario: Phase1Scenario, upperTranslationYMeters: number) => ({ id:"test", type:"run-phase1-contact-query" as const, fixtureId:"phase1-opposing-occlusal-surfaces", scenario, upperTranslationYMeters, meshes:meshes() });
describe("Phase 1.1 worker contact metrics", () => {
  it("calculates separated clearance independently from vertices and transform", async () => { const mod = await loadWorkerModule(); const translation = 0.25; const result = await mod.runPhase1ContactQuery(request("separated", translation)); const independentlyExpected = (upperY + translation) - lowerY; expect(result).toMatchObject({ classification:"separated", contactCount:0, clearanceMeters:0.41, penetrationDepthMeters:0, point:null, normal:null }); expect(result.clearanceMeters).toBeCloseTo(independentlyExpected, 6); });
  it("changes clearance when the upper transform changes", async () => { const mod = await loadWorkerModule(); const first = await mod.runPhase1ContactQuery(request("separated", 0.25)); const second = await mod.runPhase1ContactQuery(request("separated", 0.35)); expect(second.clearanceMeters - first.clearanceMeters).toBeCloseTo(0.1, 6); });
  it("classifies coplanar surfaces as touching, never penetrating", async () => { const mod = await loadWorkerModule(); const result = await mod.runPhase1ContactQuery(request("touching", lowerY - upperY)); expect(result.classification).toBe("touching"); expect(result.clearanceMeters).toBeCloseTo(0, 6); expect(result.penetrationDepthMeters).toBe(0); expect(result.contactCount).toBeGreaterThan(0); });
  it("validates generated asset manifest hash", () => { const manifest = JSON.parse(readFileSync("public/assets/fixtures/opposing-occlusal-surfaces.manifest.json", "utf8")); expect(createHash("sha256").update(readFileSync(manifest.file)).digest("hex")).toBe(manifest.sha256); });
});
