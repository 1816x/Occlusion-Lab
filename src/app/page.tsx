import { ThreeScene } from "@/components/three-scene";

export default function Home() {
  return (
    <main>
      <section className="hero">
        <span className="badge">Phase 2 interaction · educational synthetic geometry only</span>
        <h1>Occlusion Lab</h1>
        <p>
          Explore a simplified maxilla-fixed, mandible-moving geometric model. Movement and contacts are calculated by Rapier in a persistent Web Worker. This is not an anatomical jaw model, medical device, diagnosis, or treatment-planning tool.
        </p>
        <div className="panel scene" aria-label="Synthetic Three.js scene"><ThreeScene /></div>
      </section>
    </main>
  );
}
