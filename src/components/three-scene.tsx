"use client";

import { useCallback, useEffect, useRef, useState } from "react";
import * as THREE from "three";
import { GLTFLoader } from "three/examples/jsm/loaders/GLTFLoader.js";
import { DRACOLoader } from "three/examples/jsm/loaders/DRACOLoader.js";
import {
  NEUTRAL_POSE,
  POSE_LIMITS,
  metersToMillimeters,
  type AppliedTransform,
  type CollisionMeshPayload,
  type MandibularPose,
  type PoseResult,
} from "@/physics/worker-contract";
import { PoseResponseCoordinator, workerBoundaryError } from "@/physics/ui-response-gate";
import { advanceLesson, type LessonStage } from "@/physics/lesson";

const FIXTURE_ID = "phase1-opposing-occlusal-surfaces";
const SEPARATED = NEUTRAL_POSE;
const FIRST_CONTACT: MandibularPose = {
  openingMeters: 0,
  protrusionMeters: 0,
  lateralMeters: 0,
};

function cloneBuffer(view: ArrayBufferView) {
  const copy = new Uint8Array(view.byteLength);
  copy.set(new Uint8Array(view.buffer, view.byteOffset, view.byteLength));
  return copy.buffer;
}

function collisionPayload(mesh: THREE.Mesh): CollisionMeshPayload {
  const position = mesh.geometry.getAttribute("position");
  const index = mesh.geometry.index;
  if (!position || !index) throw new Error(`Missing collision buffers for ${mesh.name}`);
  return {
    name: mesh.name,
    positions: cloneBuffer(position.array),
    indices: cloneBuffer(index.array),
    indexComponentType: index.array instanceof Uint32Array ? "uint32" : "uint16",
  };
}

function applyTransform(mesh: THREE.Mesh | null, transform: AppliedTransform) {
  mesh?.position.set(
    transform.translationMeters.x,
    transform.translationMeters.y,
    transform.translationMeters.z,
  );
  mesh?.quaternion.set(
    transform.rotationQuaternion.x,
    transform.rotationQuaternion.y,
    transform.rotationQuaternion.z,
    transform.rotationQuaternion.w,
  );
}

export function ThreeScene() {
  const hostRef = useRef<HTMLDivElement>(null);
  const workerRef = useRef<Worker | null>(null);
  const mandibleRef = useRef<THREE.Mesh | null>(null);
  const clearContactsRef = useRef<() => void>(() => undefined);
  const separatedTransformRef = useRef<AppliedTransform | null>(null);

  const [asset, setAsset] = useState("loading");
  const [workerState, setWorkerState] = useState("loading");
  const [error, setError] = useState<string | null>(null);
  const [pose, setPose] = useState<MandibularPose>(NEUTRAL_POSE);
  const [result, setResult] = useState<PoseResult | null>(null);
  const [pending, setPending] = useState(true);
  const [lesson, setLesson] = useState<LessonStage>("start");

  const [coordinator] = useState(
    () =>
      new PoseResponseCoordinator({
        fixtureId: FIXTURE_ID,
        scheduler: {
          request: (callback) => requestAnimationFrame(callback),
          cancel: (handle) => cancelAnimationFrame(handle),
        },
        dispatch: (request) => workerRef.current?.postMessage(request),
        pending: (desiredPose) => {
          setPose(desiredPose);
          setPending(true);
          setResult(null);
          clearContactsRef.current();
        },
      }),
  );

  const sendPose = useCallback(
    (next: MandibularPose) => {
      coordinator.desire(next);
    },
    [coordinator],
  );

  const resetLesson = () => {
    setLesson("start");
    setResult(null);
    setPending(true);
    clearContactsRef.current();
    const separatedTransform = separatedTransformRef.current;
    if (separatedTransform) applyTransform(mandibleRef.current, separatedTransform);
    coordinator.reset(SEPARATED);
  };

  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    if (!renderer.capabilities.isWebGL2) {
      queueMicrotask(() => setError("WebGL2 is required for this educational visualization."));
      return;
    }
    renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
    host.appendChild(renderer.domElement);

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 100);
    camera.position.set(3, 2.2, 4.8);
    camera.lookAt(0, 0, 0);
    scene.add(
      new THREE.HemisphereLight(0xffffff, 0x203040, 2.2),
      new THREE.GridHelper(5, 10, 0x36506f, 0x20354f),
    );
    const light = new THREE.DirectionalLight(0x7dd3fc, 2.5);
    light.position.set(4, 6, 3);
    scene.add(light);

    const markerGeometry = new THREE.BufferGeometry();
    const markerMaterial = new THREE.PointsMaterial({
      size: 0.075,
      vertexColors: true,
      sizeAttenuation: true,
    });
    const markers = new THREE.Points(markerGeometry, markerMaterial);
    clearContactsRef.current = () => {
      markerGeometry.setDrawRange(0, 0);
      markerGeometry.deleteAttribute("position");
      markerGeometry.deleteAttribute("color");
    };
    scene.add(markers);

    const draco = new DRACOLoader();
    draco.setDecoderPath("/draco/");
    draco.setDecoderConfig({ type: "js" });
    const loader = new GLTFLoader();
    loader.setDRACOLoader(draco);

    const failWorkerBoundary = (message: string) => {
      setError(message);
      setWorkerState("error");
      setPending(false);
      setResult(null);
      clearContactsRef.current();
    };

    const worker = new Worker(new URL("../workers/rapier.worker.ts", import.meta.url), {
      type: "module",
    });
    workerRef.current = worker;
    worker.onerror = () => failWorkerBoundary(workerBoundaryError("error"));
    worker.onmessageerror = () => failWorkerBoundary(workerBoundaryError("messageerror"));
    worker.onmessage = (event: MessageEvent<unknown>) => {
      const decision = coordinator.receive(event.data);
      if (decision.kind === "ignored") return;
      if (decision.kind === "error") {
        failWorkerBoundary(decision.message);
        return;
      }

      const response = decision.response;
      if (response.type === "error") {
        failWorkerBoundary(response.message);
        return;
      }
      if (response.type === "fixture-ready") {
        setWorkerState("ready");
        coordinator.desire(NEUTRAL_POSE);
        return;
      }
      if (response.type !== "mandibular-pose-result") return;

      setPose(response.requestedPose);
      setResult(response);
      setPending(false);
      setLesson((stage) => advanceLesson(stage, response));
      applyTransform(mandibleRef.current, response.appliedTransform);
      if (
        response.requestedPose.openingMeters === SEPARATED.openingMeters &&
        response.requestedPose.protrusionMeters === SEPARATED.protrusionMeters &&
        response.requestedPose.lateralMeters === SEPARATED.lateralMeters
      ) {
        separatedTransformRef.current = response.appliedTransform;
      }

      const positions = new Float32Array(
        response.contactSamples.flatMap((contact) => [
          contact.pointWorldMeters.x,
          contact.pointWorldMeters.y,
          contact.pointWorldMeters.z,
        ]),
      );
      const colors = new Float32Array(
        response.contactSamples.flatMap((contact) =>
          contact.penetrationDepthMeters > 0 ? [1, 0.25, 0.2] : [0.2, 1, 0.7],
        ),
      );
      markerGeometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
      markerGeometry.setAttribute("color", new THREE.BufferAttribute(colors, 3));
      markerGeometry.setDrawRange(0, response.contactSamples.length);
    };

    loader.load(
      "/generated/opposing-occlusal-surfaces.glb",
      (gltf) => {
        const found: THREE.Mesh[] = [];
        gltf.scene.traverse((object) => {
          if (
            object instanceof THREE.Mesh &&
            (object.name === "OL_COLLISION_UPPER" || object.name === "OL_COLLISION_LOWER")
          ) {
            object.material = new THREE.MeshStandardMaterial({
              color: object.name.includes("UPPER") ? 0x9bd5ff : 0xffd6a5,
              roughness: 0.48,
              transparent: true,
              opacity: 0.9,
            });
            found.push(object);
          }
        });
        if (found.length !== 2) {
          setError("The synthetic fixture could not be validated.");
          setAsset("error");
          return;
        }

        scene.add(gltf.scene);
        const upper = found.find((mesh) => mesh.name.includes("UPPER"))!;
        const lower = found.find((mesh) => mesh.name.includes("LOWER"))!;
        mandibleRef.current = lower;
        setAsset("ready");
        const meshes: [CollisionMeshPayload, CollisionMeshPayload] = [
          collisionPayload(upper),
          collisionPayload(lower),
        ];
        const id = "initialize-1";
        coordinator.registerRequest(id);
        worker.postMessage(
          { id, type: "initialize-occlusion-fixture", fixtureId: FIXTURE_ID, meshes },
          meshes.flatMap((mesh) => [mesh.positions, mesh.indices]),
        );
      },
      undefined,
      () => {
        setAsset("error");
        setError("The synthetic fixture could not be loaded.");
      },
    );

    const resize = () => {
      renderer.setSize(host.clientWidth, host.clientHeight, false);
      camera.aspect = host.clientWidth / Math.max(host.clientHeight, 1);
      camera.updateProjectionMatrix();
    };
    const observer = new ResizeObserver(resize);
    observer.observe(host);
    resize();
    let renderFrame = 0;
    const animate = () => {
      renderer.render(scene, camera);
      renderFrame = requestAnimationFrame(animate);
    };
    animate();

    return () => {
      coordinator.dispose();
      worker.terminate();
      workerRef.current = null;
      cancelAnimationFrame(renderFrame);
      observer.disconnect();
      draco.dispose();
      markerGeometry.dispose();
      markerMaterial.dispose();
      renderer.dispose();
      clearContactsRef.current = () => undefined;
      host.replaceChildren();
    };
  }, [coordinator]);

  const field = (key: keyof MandibularPose, label: string, min: number, max: number) => (
    <label>
      {label}
      <span>{metersToMillimeters(pose[key]).toFixed(0)} mm</span>
      <input
        aria-label={label}
        type="range"
        min={min}
        max={max}
        step={0.005}
        value={pose[key]}
        disabled={workerState !== "ready"}
        onChange={(event) => sendPose({ ...pose, [key]: Number(event.target.value) })}
      />
    </label>
  );

  return (
    <div className="phase2">
      <div ref={hostRef} className="sceneHost" aria-label="Synthetic Three.js contact scene" />
      <section className="controls" aria-label="Mandibular movement controls">
        <p aria-live="polite">
          Status: asset {asset}; worker {workerState}.{" "}
          <b>{pending ? "evaluating newest pose…" : (result?.classification ?? "awaiting validated result")}</b>
          {!pending && result ? ` · contacts ${result.contactCount}` : ""}
        </p>
        {error && (
          <p role="alert" className="error">
            {error}
          </p>
        )}
        {field("openingMeters", "Opening / closing", 0, POSE_LIMITS.openingMeters.max)}
        {field("protrusionMeters", "Protrusion (+Z anterior)", 0, POSE_LIMITS.protrusionMeters.max)}
        {field(
          "lateralMeters",
          "Lateral (− left / + right)",
          POSE_LIMITS.lateralMeters.min,
          POSE_LIMITS.lateralMeters.max,
        )}
        <div className="buttons">
          <button disabled={workerState !== "ready"} onClick={resetLesson}>
            Reset neutral
          </button>
          <button disabled={workerState !== "ready"} onClick={() => sendPose(SEPARATED)}>
            Separated preset
          </button>
          <button disabled={workerState !== "ready"} onClick={() => sendPose(FIRST_CONTACT)}>
            First-contact preset
          </button>
        </div>
        <p className="legend">
          <i className="touch" /> touching <i className="penetrate" /> geometric penetration ·{" "}
          {pending || !result
            ? "measurement pending"
            : result.measurementStatus === "unavailable-separated"
              ? "clearance unavailable for interactive separated poses"
              : `penetration ${metersToMillimeters(result.penetrationDepthMeters).toFixed(3)} mm`}
        </p>
      </section>
      <section className="lesson">
        <h2>Lesson: move from separation to first contact</h2>
        <p>
          1. Begin separated. 2. Close to a validated touching result and inspect the markers. 3.
          Apply at least 5 mm lateral or protrusive movement while contacts remain.
        </p>
        <p>
          <strong>Progress: {lesson}</strong>
          {pending && " (advancement paused while the newest pose is evaluated)"}
        </p>
        <button disabled={workerState !== "ready"} onClick={resetLesson}>
          Reset / retry lesson
        </button>
        <p>
          Results describe deterministic synthetic geometry only—not anatomy, biomechanics,
          diagnosis, or treatment planning.
        </p>
      </section>
    </div>
  );
}
