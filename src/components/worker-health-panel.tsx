"use client";

import { useEffect, useState } from "react";
import { isPhysicsWorkerResponse, type PhysicsWorkerResponse } from "@/physics/worker-contract";

export function WorkerHealthPanel() {
  const [response, setResponse] = useState<PhysicsWorkerResponse | null>(null);
  useEffect(() => {
    const worker = new Worker(new URL("../workers/rapier.worker.ts", import.meta.url), { type: "module" });
    worker.onmessage = (event: MessageEvent<unknown>) => { if (isPhysicsWorkerResponse(event.data)) setResponse(event.data); };
    worker.postMessage({ id: crypto.randomUUID(), type: "health-check" });
    return () => worker.terminate();
  }, []);
  return <><h2>Physics worker</h2><p>Rapier WASM is initialized in a dedicated Web Worker, not on the browser main thread.</p><dl><dt>Status</dt><dd>{response?.ok ? "healthy" : "pending"}</dd><dt>Protocol</dt><dd>{response?.type === "health" ? response.protocolVersion : "—"}</dd><dt>Fixture</dt><dd>{response?.type === "health" ? response.fixtureName : "—"}</dd></dl></>;
}
