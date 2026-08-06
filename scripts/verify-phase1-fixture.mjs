import { createHash } from "node:crypto";
import { readFile } from "node:fs/promises";

const manifestPath = "public/assets/fixtures/opposing-occlusal-surfaces.manifest.json";
const manifest = JSON.parse(await readFile(manifestPath, "utf8"));
const asset = await readFile(manifest.file);
const actualHash = createHash("sha256").update(asset).digest("hex");

if (actualHash !== manifest.sha256) {
  console.error(`Expected SHA-256: ${manifest.sha256}`);
  console.error(`Actual SHA-256:   ${actualHash}`);
  process.exitCode = 1;
} else {
  console.log(`Verified ${manifest.file}`);
  console.log(`Expected SHA-256: ${manifest.sha256}`);
  console.log(`Actual SHA-256:   ${actualHash}`);
}
