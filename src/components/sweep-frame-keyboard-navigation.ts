const FRAME_DELTAS: Readonly<Record<string, number>> = {
  ArrowLeft: -1,
  ArrowDown: -1,
  ArrowRight: 1,
  ArrowUp: 1,
};

export function sweepPageJump(frameCount: number): number {
  return Math.max(1, Math.round((frameCount - 1) / 10));
}

/** Returns null for keys that are not sweep timeline navigation commands. */
export function sweepFrameForNavigationKey(
  key: string,
  currentFrame: number,
  frameCount: number,
): number | null {
  if (frameCount < 1) return null;

  const lastFrame = frameCount - 1;
  let nextFrame: number;

  if (key === "Home") nextFrame = 0;
  else if (key === "End") nextFrame = lastFrame;
  else if (key === "PageUp") nextFrame = currentFrame + sweepPageJump(frameCount);
  else if (key === "PageDown") nextFrame = currentFrame - sweepPageJump(frameCount);
  else if (key in FRAME_DELTAS) nextFrame = currentFrame + FRAME_DELTAS[key]!;
  else return null;

  return Math.min(lastFrame, Math.max(0, nextFrame));
}
