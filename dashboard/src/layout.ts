import type { GraphEdge, HyphaResource, LaidOutEdge, LaidOutNode } from "./types";

export const LAYER_SPACING = 260;
export const NODE_SPACING = 108;
export const NODE_RADIUS = 30;

function computeLayers(resources: HyphaResource[]): Map<string, number> {
  const byId = new Map(resources.map((r) => [r.id, r]));
  const layer = new Map<string, number>();
  const visiting = new Set<string>();

  function resolve(id: string): number {
    if (layer.has(id)) return layer.get(id)!;
    if (visiting.has(id)) return 0; 
    visiting.add(id);
    const res = byId.get(id);
    const deps = res?.dependsOn ?? [];
    const depth = deps.length === 0 ? 0 : 1 + Math.max(...deps.map(resolve));
    visiting.delete(id);
    layer.set(id, depth);
    return depth;
  }

  resources.forEach((r) => resolve(r.id));
  return layer;
}

export function layoutGraph(
  resources: HyphaResource[]
): { nodes: LaidOutNode[]; edges: LaidOutEdge[] } {
  const layers = computeLayers(resources);
  const maxLayer = Math.max(0, ...Array.from(layers.values()));

  const byLayer = new Map<number, HyphaResource[]>();
  for (let l = 0; l <= maxLayer; l++) byLayer.set(l, []);
  resources.forEach((r) => byLayer.get(layers.get(r.id)!)!.push(r));

  byLayer.forEach((list) =>
    list.sort((a, b) => a.kind.localeCompare(b.kind) || a.name.localeCompare(b.name))
  );

  const nodes: LaidOutNode[] = [];
  const nodeById = new Map<string, LaidOutNode>();

  for (let l = 0; l <= maxLayer; l++) {
    const list = byLayer.get(l)!;
    const totalHeight = (list.length - 1) * NODE_SPACING;
    list.forEach((res, i) => {
      const node: LaidOutNode = {
        ...res,
        layer: l,
        order: i,
        x: l * LAYER_SPACING,
        y: i * NODE_SPACING - totalHeight / 2,
      };
      nodes.push(node);
      nodeById.set(node.id, node);
    });
  }

  const edges: LaidOutEdge[] = [];
  resources.forEach((r) => {
    r.dependsOn.forEach((depId) => {
      const sourceNode = nodeById.get(depId);
      const targetNode = nodeById.get(r.id);
      if (!sourceNode || !targetNode) return;
      const edge: GraphEdge = { id: `${depId}->${r.id}`, source: depId, target: r.id };
      edges.push({ ...edge, sourceNode, targetNode });
    });
  });

  return { nodes, edges };
}
