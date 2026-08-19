import type { SweepPreset } from "@/physics/worker-contract";

export type SweepDownloadFormat = "json" | "csv";

type DownloadBoundary = {
  createObjectURL: (blob: Blob) => string;
  revokeObjectURL: (url: string) => void;
  clickDownload: (url: string, filename: string) => void;
  scheduleCleanup: (cleanup: () => void) => void;
};

const browserBoundary: DownloadBoundary = {
  createObjectURL: (blob) => URL.createObjectURL(blob),
  revokeObjectURL: (url) => URL.revokeObjectURL(url),
  clickDownload: (url, filename) => {
    const anchor = document.createElement("a");
    anchor.href = url;
    anchor.download = filename;
    anchor.click();
  },
  scheduleCleanup: (cleanup) => window.setTimeout(cleanup, 0),
};

export function sweepDownloadFilename(
  preset: SweepPreset,
  frameCount: number,
  format: SweepDownloadFormat,
) {
  return `occlusion-lab-${preset}-${frameCount}-frames.${format}`;
}

export function downloadSweepExport(
  content: string,
  preset: SweepPreset,
  frameCount: number,
  format: SweepDownloadFormat,
  boundary: DownloadBoundary = browserBoundary,
) {
  const mime = format === "json" ? "application/json;charset=utf-8" : "text/csv;charset=utf-8";
  const blob = new Blob([content], { type: mime });
  const url = boundary.createObjectURL(blob);
  boundary.clickDownload(url, sweepDownloadFilename(preset, frameCount, format));
  boundary.scheduleCleanup(() => boundary.revokeObjectURL(url));
}
