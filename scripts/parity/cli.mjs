import { build } from "esbuild";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";

const command = process.argv[2];
const focus = process.argv[3] ?? "all";
if (command !== "generate" && command !== "verify") throw new Error("Expected generate or verify");
if (focus !== "all" && focus !== "pose" && focus !== "sweep" && focus !== "collision") throw new Error("Expected all, pose, sweep, or collision");

const definitions = [
  { focus: "pose", entry: "scripts/parity/mandibular-pose-fixture.ts", pathExport: "FIXTURE_PATH", serialize: "serializeMandibularPoseFixture", verify: "verifyMandibularPoseFixture" },
  { focus: "sweep", entry: "scripts/parity/mandibular-sweep-fixture.ts", pathExport: "SWEEP_FIXTURE_PATH", serialize: "serializeMandibularSweepFixture", verify: "verifyMandibularSweepFixture" },
  { focus: "collision", entry: "scripts/parity/single-pose-collision-fixture.ts", pathExport: "COLLISION_FIXTURE_PATH", serialize: "serializeSinglePoseCollisionFixture", verify: "verifySinglePoseCollisionFixture" },
];

for (const definition of definitions.filter((item) => focus === "all" || item.focus === focus)) {
  const result = await build({ entryPoints: [definition.entry], bundle: true, format: "esm", platform: "node", write: false });
  const moduleUrl = `data:text/javascript;base64,${Buffer.from(result.outputFiles[0].text).toString("base64")}`;
  const fixture = await import(moduleUrl);
  const fixturePath = fixture[definition.pathExport];
  const path = resolve(fixturePath);
  if (command === "generate") {
    await mkdir(dirname(path), { recursive: true });
    await writeFile(path, fixture[definition.serialize](), "utf8");
    console.log(`Wrote ${fixturePath}`);
  } else {
    fixture[definition.verify](await readFile(path, "utf8"));
    console.log(`Verified ${fixturePath}`);
  }
}
