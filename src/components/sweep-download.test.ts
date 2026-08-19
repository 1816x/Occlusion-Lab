import { describe, expect, it, vi } from "vitest";
import { downloadSweepExport, sweepDownloadFilename } from "./sweep-download";

describe("browser sweep downloads", () => {
  it("builds deterministic filenames", () => {
    expect(sweepDownloadFilename("left-lateral", 31, "json"))
      .toBe("occlusion-lab-left-lateral-31-frames.json");
    expect(sweepDownloadFilename("closing", 11, "csv"))
      .toBe("occlusion-lab-closing-11-frames.csv");
  });

  it.each([
    ["json" as const, "application/json;charset=utf-8"],
    ["csv" as const, "text/csv;charset=utf-8"],
  ])("passes exact %s content and MIME type to a cleaned-up URL", async (format, mime) => {
    let receivedBlob: Blob | undefined;
    const revokeObjectURL = vi.fn();
    const clickDownload = vi.fn();
    const boundary = {
      createObjectURL: vi.fn((blob: Blob) => {
        receivedBlob = blob;
        return "blob:test-url";
      }),
      revokeObjectURL,
      clickDownload,
      scheduleCleanup: (cleanup: () => void) => cleanup(),
    };

    downloadSweepExport("exact-content\n", "closing", 21, format, boundary);

    expect(receivedBlob?.type).toBe(mime);
    expect(await receivedBlob?.text()).toBe("exact-content\n");
    expect(clickDownload).toHaveBeenCalledWith(
      "blob:test-url",
      `occlusion-lab-closing-21-frames.${format}`,
    );
    expect(revokeObjectURL).toHaveBeenCalledWith("blob:test-url");
  });
});
