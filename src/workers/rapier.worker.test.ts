import { build } from "esbuild";
import { mkdtemp, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import { afterEach, describe, expect, it } from "vitest";
import { SYNTHETIC_COLLISION_EXPECTED, SYNTHETIC_COLLISION_STEPS } from "@/test-fixtures/synthetic-collision";

type CollisionRunnerModule = typeof import("./rapier.worker");

let tempDirectory: string | undefined;

afterEach(async () => {
  if (tempDirectory) {
    await rm(tempDirectory, { recursive: true, force: true });
    tempDirectory = undefined;
  }
});

describe("Rapier synthetic collision fixture", () => {
  it("executes the worker fixture with Rapier and produces the documented deterministic result", async () => {
    tempDirectory = await mkdtemp(join(tmpdir(), "occlusion-rapier-worker-"));
    const outfile = join(tempDirectory, "rapier-worker-test.mjs");

    await build({
      absWorkingDir: process.cwd(),
      bundle: true,
      entryPoints: ["src/workers/rapier.worker.ts"],
      format: "esm",
      outfile,
      platform: "browser",
      sourcemap: false,
      tsconfig: "tsconfig.json",
    });

    const workerModule = await import(pathToFileURL(outfile).href) as CollisionRunnerModule;
    const result = await workerModule.runSyntheticCollisionFixture();

    expect(result.collided).toBe(true);
    expect(result.steps).toBe(SYNTHETIC_COLLISION_STEPS);
    expect(result.finalDynamicY).toBeGreaterThanOrEqual(SYNTHETIC_COLLISION_EXPECTED.minFinalDynamicY);
    expect(result.finalDynamicY).toBeLessThanOrEqual(SYNTHETIC_COLLISION_EXPECTED.maxFinalDynamicY);
  });
});
