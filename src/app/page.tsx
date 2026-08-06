import { ThreeScene } from "@/components/three-scene";
import { WorkerHealthPanel } from "@/components/worker-health-panel";

export default function Home() {
  return (
    <main>
      <section className="hero">
        <span className="badge">Phase 1 vertical slice · educational only</span>
        <h1>Occlusion Lab</h1>
        <p>
          A work-in-progress browser research sandbox for visualizing synthetic occlusion concepts. Phase 1 loads an original compressed GLB fixture, transfers collision buffers to a Rapier Web Worker, and reports deterministic separated/contact outcomes. Educational only: no patient data, no clinical claims, no LLM calculation path, and no complete simulation.
        </p>
        <div className="grid">
          <div className="panel scene" aria-label="Synthetic Three.js scene"><ThreeScene /></div>
          <div className="panel"><WorkerHealthPanel /></div>
        </div>
      </section>
    </main>
  );
}
