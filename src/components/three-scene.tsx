"use client";

import { useEffect, useRef } from "react";
import * as THREE from "three";

export function ThreeScene() {
  const hostRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const host = hostRef.current;
    if (!host) return;
    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true, powerPreference: "high-performance" });
    if (!renderer.capabilities.isWebGL2) {
      host.textContent = "WebGL2 is required for Occlusion Lab.";
      return;
    }
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    host.appendChild(renderer.domElement);

    const scene = new THREE.Scene();
    const camera = new THREE.PerspectiveCamera(45, 1, 0.1, 100);
    camera.position.set(3, 2.2, 4.8);
    camera.lookAt(0, 0, 0);
    scene.add(new THREE.HemisphereLight(0xffffff, 0x203040, 2.2));
    const light = new THREE.DirectionalLight(0x7dd3fc, 2.5); light.position.set(4, 6, 3); scene.add(light);
    const upper = new THREE.Mesh(new THREE.BoxGeometry(2.8, .35, 1.1), new THREE.MeshStandardMaterial({ color: 0x9bd5ff, roughness: .45 }));
    upper.position.y = .42;
    const lower = new THREE.Mesh(new THREE.BoxGeometry(2.8, .35, 1.1), new THREE.MeshStandardMaterial({ color: 0xffd6a5, roughness: .5 }));
    lower.position.y = -.42;
    scene.add(upper, lower, new THREE.GridHelper(5, 10, 0x36506f, 0x20354f));

    const resize = () => { const { clientWidth, clientHeight } = host; renderer.setSize(clientWidth, clientHeight, false); camera.aspect = clientWidth / Math.max(clientHeight, 1); camera.updateProjectionMatrix(); };
    const observer = new ResizeObserver(resize); observer.observe(host); resize();
    let frame = 0; let raf = 0;
    const animate = () => { frame += 0.01; upper.rotation.y = frame; lower.rotation.y = -frame * .8; renderer.render(scene, camera); raf = requestAnimationFrame(animate); };
    animate();
    return () => { cancelAnimationFrame(raf); observer.disconnect(); renderer.dispose(); upper.geometry.dispose(); lower.geometry.dispose(); host.replaceChildren(); };
  }, []);

  return <div ref={hostRef} style={{ width: "100%", height: "100%" }} />;
}
