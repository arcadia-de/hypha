export type ActionState = "Create" | "Update" | "Delete" | "None";

export type ResourceKind =
  | "PackageManager"
  | "Package"
  | "Symlink"
  | "Repository"
  | "Font"
  | "Controller";

export interface HyphaResource {
  id: string;
  kind: ResourceKind;
  name: string;
  labels: Record<string, string>;
  annotations: Record<string, string>;
  spec: Record<string, unknown>;
  action: ActionState;
  reason: string;
  dependsOn: string[];
}

export interface GraphEdge {
  id: string;
  source: string;
  target: string;
}

export interface LaidOutNode extends HyphaResource {
  layer: number;
  order: number;
  x: number;
  y: number;
}

export interface LaidOutEdge extends GraphEdge {
  sourceNode: LaidOutNode;
  targetNode: LaidOutNode;
}
