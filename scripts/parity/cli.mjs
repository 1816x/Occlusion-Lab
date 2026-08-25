import { build } from "esbuild";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import { dirname, resolve } from "node:path";
import { pathToFileURL } from "node:url";

const result = await build({ entryPoints: ["scripts/parity/mandibular-pose-fixture.ts"], bundle: true, format: "esm", platform: "node", write: false });
const moduleUrl = `data:text/javascript;base64,${Buffer.from(result.outputFiles[0].text).toString("base64")}`;
const fixture = await import(moduleUrl);
const path = resolve(fixture.FIXTURE_PATH);
const command = process.argv[2];
if (command === "generate") {
  await mkdir(dirname(path), { recursive: true });
  await writeFile(path, fixture.serializeMandibularPoseFixture(), "utf8");
  console.log(`Wrote ${fixture.FIXTURE_PATH}`);
} else if (command === "verify") {
  fixture.verifyMandibularPoseFixture(await readFile(path, "utf8"));
  console.log(`Verified ${fixture.FIXTURE_PATH}`);
} else throw new Error("Expected generate or verify");
