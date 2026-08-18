import type { PoseResult } from "./worker-contract";

export type LessonStage = "start" | "separated" | "contact" | "complete";

/** Progress is based only on the pose that the worker says it evaluated. */
export function advanceLesson(stage: LessonStage, result: PoseResult): LessonStage {
  if (stage === "start" && result.classification === "separated") return "separated";
  if (stage === "separated" && result.classification === "touching" && result.contactCount > 0) {
    return "contact";
  }
  if (
    stage === "contact" &&
    result.contactCount > 0 &&
    (Math.abs(result.requestedPose.lateralMeters) >= 0.005 ||
      result.requestedPose.protrusionMeters >= 0.005)
  ) {
    return "complete";
  }
  return stage;
}
