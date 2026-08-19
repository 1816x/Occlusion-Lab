import {
  isPhysicsWorkerResponse,
  type MandibularPose,
  type PhysicsWorkerRequest,
  type PhysicsWorkerResponse,
} from "./worker-contract";

export type FrameScheduler = {
  request(callback: () => void): number;
  cancel(handle: number): void;
};

type PendingRequest =
  | { kind: "other"; generation: number }
  | {
      kind: "pose";
      generation: number;
      revision: number;
      pose: MandibularPose;
    };

export type CoordinationDecision =
  | { kind: "accepted"; response: PhysicsWorkerResponse }
  | { kind: "ignored" }
  | { kind: "error"; message: string };

export type PoseCoordinatorOptions = {
  fixtureId: string;
  scheduler: FrameScheduler;
  dispatch(request: Extract<PhysicsWorkerRequest, { type: "evaluate-mandibular-pose" }>): void;
  pending(pose: MandibularPose): void;
};

const copyPose = (pose: MandibularPose): MandibularPose => ({ ...pose });
const posesEqual = (left: MandibularPose, right: MandibularPose) =>
  left.openingMeters === right.openingMeters &&
  left.protrusionMeters === right.protrusionMeters &&
  left.lateralMeters === right.lateralMeters;

/** Coordinates desired UI revisions independently of React and rendering state. */
export class PoseResponseCoordinator {
  private generation = 0;
  private desiredRevision = 0;
  private desired: { revision: number; pose: MandibularPose } | undefined;
  private scheduledFrame: number | undefined;
  private readonly pendingRequests = new Map<string, PendingRequest>();
  private readonly invalidatedIds = new Set<string>();

  constructor(private readonly options: PoseCoordinatorOptions) {}

  get currentGeneration() {
    return this.generation;
  }

  registerRequest(id: string) {
    this.pendingRequests.set(id, { kind: "other", generation: this.generation });
  }

  /** Invalidates in-flight work without scheduling a replacement request. */
  invalidatePending() {
    this.generation += 1;
    for (const id of this.pendingRequests.keys()) this.invalidatedIds.add(id);
    this.pendingRequests.clear();
    if (this.scheduledFrame !== undefined) this.options.scheduler.cancel(this.scheduledFrame);
    this.scheduledFrame = undefined;
    this.desired = undefined;
  }

  desire(pose: MandibularPose) {
    const snapshot = copyPose(pose);
    this.desiredRevision += 1;
    this.desired = { revision: this.desiredRevision, pose: snapshot };
    this.options.pending(copyPose(snapshot));

    if (this.scheduledFrame === undefined) {
      this.scheduledFrame = this.options.scheduler.request(() => this.dispatchDesired());
    }
    return this.desiredRevision;
  }

  reset(pose: MandibularPose) {
    this.invalidatePending();
    return this.desire(pose);
  }

  dispose() {
    if (this.scheduledFrame !== undefined) this.options.scheduler.cancel(this.scheduledFrame);
    this.scheduledFrame = undefined;
  }

  receive(data: unknown): CoordinationDecision {
    if (!isPhysicsWorkerResponse(data)) {
      return {
        kind: "error",
        message: "The physics worker returned an invalid response. No result was applied.",
      };
    }

    if (this.invalidatedIds.delete(data.id)) return { kind: "ignored" };

    const pending = this.pendingRequests.get(data.id);
    if (!pending) {
      return {
        kind: "error",
        message: "The physics worker returned an uncorrelated response. No result was applied.",
      };
    }
    this.pendingRequests.delete(data.id);

    if (pending.generation !== this.generation) return { kind: "ignored" };
    if (data.type !== "mandibular-pose-result") return { kind: "accepted", response: data };
    if (pending.kind !== "pose") return this.correlationError();
    if (pending.revision < this.desiredRevision) return { kind: "ignored" };
    if (
      data.sequence !== pending.revision ||
      data.fixtureId !== this.options.fixtureId ||
      !posesEqual(data.requestedPose, pending.pose)
    ) {
      return this.correlationError();
    }
    return { kind: "accepted", response: data };
  }

  private correlationError(): CoordinationDecision {
    return {
      kind: "error",
      message: "The physics worker returned an uncorrelated response. No result was applied.",
    };
  }

  private dispatchDesired() {
    this.scheduledFrame = undefined;
    if (!this.desired) return;
    const { revision, pose } = this.desired;
    const id = `pose-g${this.generation}-r${revision}`;
    const snapshot = copyPose(pose);
    this.pendingRequests.set(id, {
      kind: "pose",
      generation: this.generation,
      revision,
      pose: snapshot,
    });
    this.options.dispatch({
      id,
      type: "evaluate-mandibular-pose",
      fixtureId: this.options.fixtureId,
      sequence: revision,
      pose: copyPose(snapshot),
    });
  }
}

export function workerBoundaryError(event: "error" | "messageerror") {
  return event === "error"
    ? "The physics worker failed. No result was applied."
    : "The physics worker response could not be deserialized. No result was applied.";
}
