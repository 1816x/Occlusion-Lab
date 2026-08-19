"use client";

import { useState } from "react";
import {
  SWEEP_PRESETS,
  type SweepPreset,
  type SweepResult,
} from "@/physics/worker-contract";
import {
  classificationPresentation,
  formatMetersAsMillimeters,
  formatSweepProgress,
  selectCachedFrame,
} from "./sweep-frame-presentation";
import { sweepFrameForNavigationKey } from "./sweep-frame-keyboard-navigation";

type MotionSweepLabProps = {
  workerReady: boolean;
  preset: SweepPreset;
  frameCount: number;
  sweep: SweepResult | null;
  pending: boolean;
  inspectedFrameIndex: number;
  onPresetChange: (preset: SweepPreset) => void;
  onFrameCountChange: (frameCount: number) => void;
  onRunSweep: () => void;
  onInspectFrame: (frameIndex: number) => void;
  onExport: (format: "json" | "csv", sweep: SweepResult) => void;
};

export function MotionSweepLab({
  workerReady,
  preset,
  frameCount,
  sweep,
  pending,
  inspectedFrameIndex,
  onPresetChange,
  onFrameCountChange,
  onRunSweep,
  onInspectFrame,
  onExport,
}: MotionSweepLabProps) {
  const [exportStatus, setExportStatus] = useState<{ sweep: SweepResult; format: "json" | "csv" } | null>(null);
  const inspected = sweep ? selectCachedFrame(sweep, inspectedFrameIndex) : undefined;
  const canExport = sweep !== null && !pending;

  const exportSweep = (format: "json" | "csv") => {
    if (!canExport || !sweep) return;
    onExport(format, sweep);
    setExportStatus({ sweep, format });
  };

  const navigateTimeline = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (!sweep) return;
    const nextFrame = sweepFrameForNavigationKey(event.key, inspectedFrameIndex, sweep.frameCount);
    if (nextFrame === null) return;
    event.preventDefault();
    if (nextFrame !== inspectedFrameIndex) onInspectFrame(nextFrame);
  };

  return (
    <section className="sweepLab" aria-labelledby="sweep-heading">
      <h2 id="sweep-heading">Motion Sweep Lab</h2>
      <p>Inspect a bounded synthetic geometric sequence. These paths are not anatomically accurate.</p>
      <div className="sweepSetup">
        <label>
          Preset
          <select value={preset} onChange={(event) => onPresetChange(event.target.value as SweepPreset)}>
            {SWEEP_PRESETS.map((value) => <option key={value} value={value}>{value}</option>)}
          </select>
        </label>
        <label>
          Frames
          <select value={frameCount} onChange={(event) => onFrameCountChange(Number(event.target.value))}>
            {[11, 21, 31, 61].map((value) => <option key={value}>{value}</option>)}
          </select>
        </label>
        <button disabled={!workerReady || pending} onClick={onRunSweep}>
          {pending ? "Evaluating…" : "Run sweep"}
        </button>
      </div>
      <div className="buttons sweepExports">
        <button
          aria-label="Export latest validated sweep as JSON"
          disabled={!canExport}
          onClick={() => exportSweep("json")}
        >
          Export JSON
        </button>
        <button
          aria-label="Export latest validated sweep as CSV"
          disabled={!canExport}
          onClick={() => exportSweep("csv")}
        >
          Export CSV
        </button>
      </div>
      <p className="exportStatus" aria-live="polite">
        {!pending && exportStatus?.sweep === sweep
          ? `${exportStatus.format.toUpperCase()} export downloaded.`
          : ""}
      </p>
      {sweep && inspected && (
        <>
          <label className="timeline">
            Timeline: frame {inspectedFrameIndex + 1} of {sweep.frameCount}
            <input
              aria-label="Sweep timeline"
              aria-describedby="sweep-timeline-instructions"
              aria-valuetext={`Frame ${inspectedFrameIndex + 1} of ${sweep.frameCount}, contact classification: ${inspected.classification}`}
              type="range"
              min="0"
              max={sweep.frameCount - 1}
              value={inspectedFrameIndex}
              onChange={(event) => onInspectFrame(Number(event.target.value))}
              onKeyDown={navigateTimeline}
            />
          </label>
          <p id="sweep-timeline-instructions" className="timelineInstructions">
            Use arrow keys for adjacent frames, Home or End for the first or last frame, and Page Up or Page Down for larger jumps.
          </p>
          <div className="timelineTicks" role="list" aria-label="Frame contact classifications">
            {sweep.frames.map((frame) => {
              const presentation = classificationPresentation(frame.classification);
              return (
              <span
                role="listitem"
                title={`Frame ${frame.frameIndex + 1}: ${frame.classification}`}
                aria-label={`Frame ${frame.frameIndex + 1}: ${frame.classification}`}
                className={presentation.cssClass}
                key={frame.frameIndex}
              />
              );
            })}
          </div>
          <div className="buttons">
            <button disabled={inspectedFrameIndex === 0} onClick={() => onInspectFrame(inspectedFrameIndex - 1)}>Previous frame</button>
            <button disabled={inspectedFrameIndex === sweep.frameCount - 1} onClick={() => onInspectFrame(inspectedFrameIndex + 1)}>Next frame</button>
          </div>
          <p>Progress {formatSweepProgress(inspected.progress)} · pose {formatMetersAsMillimeters(inspected.requestedPose.openingMeters, 1)} opening, {formatMetersAsMillimeters(inspected.requestedPose.protrusionMeters, 1)} Z, {formatMetersAsMillimeters(inspected.requestedPose.lateralMeters, 1)} X · <b>{classificationPresentation(inspected.classification).label}</b> · {inspected.contactCount} contacts · {formatMetersAsMillimeters(inspected.penetrationDepthMeters, 3)} geometric penetration.</p>
          <p>Summary: {sweep.summary.contactFrameCount}/{sweep.summary.totalFrameCount} frames contain contact; first {sweep.summary.firstContactFrame === null ? "none" : sweep.summary.firstContactFrame + 1}; last {sweep.summary.lastContactFrame === null ? "none" : sweep.summary.lastContactFrame + 1}; maximum geometric penetration {formatMetersAsMillimeters(sweep.summary.maximumPenetrationMeters, 3)} at frame {sweep.summary.maximumPenetrationFrame + 1}; persists through final frame: {sweep.summary.contactPersistsThroughFinalFrame ? "yes" : "no"}.</p>
        </>
      )}
    </section>
  );
}
