// @vitest-environment jsdom
import { afterEach, describe, expect, it, vi } from "vitest";
import { cleanup, fireEvent, render, screen } from "@testing-library/react";
import type { SweepResult } from "@/physics/worker-contract";
import { MotionSweepLab } from "./motion-sweep-lab";

afterEach(cleanup);

const sweep = {
  fixtureId: "fixture-a",
  sequence: 1,
  preset: "closing",
  frameCount: 2,
  finalPose: { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 },
  summary: { totalFrameCount: 2, firstContactFrame: null, lastContactFrame: null, contactFrameCount: 0, maximumPenetrationMeters: 0, maximumPenetrationFrame: 0, contactPersistsThroughFinalFrame: false },
  frames: [0, 1].map((frameIndex) => ({
    frameIndex,
    progress: frameIndex,
    requestedPose: { openingMeters: 0, protrusionMeters: 0, lateralMeters: 0 },
    appliedTransform: { translationMeters: { x: 0, y: 0, z: 0 }, rotationQuaternion: { x: 0, y: 0, z: 0, w: 1 } },
    classification: "separated" as const,
    measurementStatus: "unavailable-separated" as const,
    clearanceMeters: null,
    penetrationDepthMeters: 0,
    contactCount: 0,
    contactSamples: [],
  })),
} satisfies SweepResult;

function renderLab(result: SweepResult | null, pending = false, onExport = vi.fn(), onInspectFrame = vi.fn()) {
  render(<MotionSweepLab workerReady preset="closing" frameCount={2} sweep={result} pending={pending} inspectedFrameIndex={0} onPresetChange={vi.fn()} onFrameCountChange={vi.fn()} onRunSweep={vi.fn()} onInspectFrame={onInspectFrame} onExport={onExport} />);
  return { onExport, onInspectFrame };
}

describe("Motion Sweep Lab exports", () => {
  it("disables exports without a validated result and during a newer request", () => {
    const { rerender } = render(<MotionSweepLab workerReady preset="closing" frameCount={2} sweep={null} pending={false} inspectedFrameIndex={0} onPresetChange={vi.fn()} onFrameCountChange={vi.fn()} onRunSweep={vi.fn()} onInspectFrame={vi.fn()} onExport={vi.fn()} />);
    expect((screen.getByRole("button", { name: /as JSON/ }) as HTMLButtonElement).disabled).toBe(true);
    rerender(<MotionSweepLab workerReady preset="closing" frameCount={2} sweep={sweep} pending inspectedFrameIndex={0} onPresetChange={vi.fn()} onFrameCountChange={vi.fn()} onRunSweep={vi.fn()} onInspectFrame={vi.fn()} onExport={vi.fn()} />);
    expect((screen.getByRole("button", { name: /as CSV/ }) as HTMLButtonElement).disabled).toBe(true);
  });

  it("exports only the completed result and announces success", () => {
    const { onExport } = renderLab(sweep);
    const button = screen.getByRole("button", { name: /as JSON/ });
    expect((button as HTMLButtonElement).disabled).toBe(false);
    fireEvent.click(button);
    expect(onExport).toHaveBeenCalledWith("json", sweep);
    expect(screen.getByText("JSON export downloaded.").textContent).toBe("JSON export downloaded.");
  });
});

describe("Motion Sweep Lab timeline accessibility", () => {
  it("describes the current value with frame position and contact classification", () => {
    renderLab(sweep);
    const timeline = screen.getByRole("slider", { name: "Sweep timeline" });
    expect(timeline.getAttribute("aria-valuetext")).toBe("Frame 1 of 2, contact classification: separated");
    const instructions = screen.getByText(/Use arrow keys for adjacent frames/);
    expect(timeline.getAttribute("aria-describedby")).toBe(instructions.id);
  });

  it("handles recognized keys, prevents their defaults, and inspects changed frames", () => {
    const { onInspectFrame } = renderLab(sweep);
    const timeline = screen.getByRole("slider", { name: "Sweep timeline" });
    const event = new KeyboardEvent("keydown", { key: "ArrowRight", bubbles: true, cancelable: true });
    timeline.dispatchEvent(event);
    expect(event.defaultPrevented).toBe(true);
    expect(onInspectFrame).toHaveBeenCalledOnce();
    expect(onInspectFrame).toHaveBeenCalledWith(1);
  });

  it("does not inspect an unchanged boundary frame but still prevents recognized defaults", () => {
    const { onInspectFrame } = renderLab(sweep);
    const timeline = screen.getByRole("slider", { name: "Sweep timeline" });
    const event = new KeyboardEvent("keydown", { key: "ArrowLeft", bubbles: true, cancelable: true });
    timeline.dispatchEvent(event);
    expect(event.defaultPrevented).toBe(true);
    expect(onInspectFrame).not.toHaveBeenCalled();
  });

  it("ignores unknown keys without preventing their defaults or inspecting a frame", () => {
    const { onInspectFrame } = renderLab(sweep);
    const timeline = screen.getByRole("slider", { name: "Sweep timeline" });
    const event = new KeyboardEvent("keydown", { key: "Enter", bubbles: true, cancelable: true });
    timeline.dispatchEvent(event);
    expect(event.defaultPrevented).toBe(false);
    expect(onInspectFrame).not.toHaveBeenCalled();
  });
});
