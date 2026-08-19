import type { ContactClassification, SweepResult } from "@/physics/worker-contract";

export function clampFrameIndex(index: number, frameCount: number): number {
  if (frameCount <= 0) return 0;
  return Math.max(0, Math.min(Math.trunc(index), frameCount - 1));
}

export function selectCachedFrame(sweep: SweepResult, index: number) {
  return sweep.frames[clampFrameIndex(index, sweep.frames.length)];
}

export const formatSweepProgress = (progress: number) => `${(progress * 100).toFixed(1)}%`;

export const formatMetersAsMillimeters = (meters: number, fractionDigits: number) =>
  `${(meters * 1000).toFixed(fractionDigits)} mm`;

export function classificationPresentation(classification: ContactClassification) {
  return { label: classification, cssClass: classification } as const;
}
