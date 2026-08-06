import type { MandibularPose, PoseResult } from "./worker-contract";
export type LessonStage = "start"|"separated"|"contact"|"complete";
export function advanceLesson(stage:LessonStage,result:PoseResult,pose:MandibularPose):LessonStage {
  if(stage==="start" && result.classification==="separated") return "separated";
  if(stage==="separated" && result.classification==="touching" && result.contactCount>0) return "contact";
  if(stage==="contact" && result.contactCount>0 && (Math.abs(pose.lateralMeters)>=0.005||pose.protrusionMeters>=0.005)) return "complete";
  return stage;
}
