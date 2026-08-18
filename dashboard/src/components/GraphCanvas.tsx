import { useEffect, useMemo, useRef } from "react";
import { select } from "d3-selection";
import { zoom, zoomIdentity, type D3ZoomEvent } from "d3-zoom";
import type { LaidOutEdge, LaidOutNode } from "../types";
import { NODE_RADIUS } from "../layout";
import { taperedThreadPath } from "../thread";

const KIND_COLOR: Record<string, string> = {
  PackageManager: "var(--kind-packagemanager)",
  Package: "var(--kind-package)",
  Symlink: "var(--kind-symlink)",
  Repository: "var(--kind-repository)",
  Font: "var(--kind-font)",
  Controller: "var(--kind-controller)",
};

const ACTION_COLOR: Record<string, string> = {
  Create: "var(--action-create)",
  Update: "var(--action-update)",
  Delete: "var(--action-delete)",
  None: "var(--action-none)",
};

interface Props {
  nodes: LaidOutNode[];
  edges: LaidOutEdge[];
  visibleNodeIds: Set<string>;
  selectedId: string | null;
  hoveredId: string | null;
  relatedIds: Set<string> | null;
  onSelect: (id: string | null) => void;
  onHover: (id: string | null) => void;
}

export function GraphCanvas({
  nodes,
  edges,
  visibleNodeIds,
  selectedId,
  hoveredId,
  relatedIds,
  onSelect,
  onHover,
}: Props) {
  const svgRef = useRef<SVGSVGElement | null>(null);
  const gRef = useRef<SVGGElement | null>(null);

  const bounds = useMemo(() => {
    if (nodes.length === 0) return { minX: 0, maxX: 0, minY: 0, maxY: 0 };
    const xs = nodes.map((n) => n.x);
    const ys = nodes.map((n) => n.y);
    return {
      minX: Math.min(...xs),
      maxX: Math.max(...xs),
      minY: Math.min(...ys),
      maxY: Math.max(...ys),
    };
  }, [nodes]);

  const layerCount = useMemo(
    () => (nodes.length ? Math.max(...nodes.map((n) => n.layer)) + 1 : 0),
    [nodes]
  );

  useEffect(() => {
    if (!svgRef.current || !gRef.current) return;
    const svgSel = select(svgRef.current);
    const gSel = select(gRef.current);

    const behavior = zoom<SVGSVGElement, unknown>()
      .scaleExtent([0.35, 2.2])
      .on("zoom", (event: D3ZoomEvent<SVGSVGElement, unknown>) => {
        gSel.attr("transform", event.transform.toString());
      });

    svgSel.call(behavior);

    const svgEl = svgRef.current;
    const rect = svgEl.getBoundingClientRect();
    const contentW = bounds.maxX - bounds.minX + 220;
    const contentH = bounds.maxY - bounds.minY + 160;
    const scale = Math.min(1, rect.width / contentW, rect.height / contentH) || 1;
    const tx = rect.width / 2 - ((bounds.minX + bounds.maxX) / 2) * scale;
    const ty = rect.height / 2 - ((bounds.minY + bounds.maxY) / 2) * scale;

    svgSel.call(behavior.transform, zoomIdentity.translate(tx, ty).scale(scale));

    return () => {
      svgSel.on(".zoom", null);
    };
  }, [bounds.minX, bounds.maxX, bounds.minY, bounds.maxY]);

  function nodeOpacity(id: string) {
    if (!visibleNodeIds.has(id)) return 0.06;
    if (!selectedId || !relatedIds) return 1;
    if (id === selectedId || relatedIds.has(id)) return 1;
    return 0.22;
  }

  function edgeState(e: LaidOutEdge) {
    const bothVisible = visibleNodeIds.has(e.source) && visibleNodeIds.has(e.target);
    let opacity = bothVisible ? 1 : 0.04;
    let active = false;
    if (selectedId && relatedIds) {
      const involved =
        e.source === selectedId ||
        e.target === selectedId ||
        (relatedIds.has(e.source) && relatedIds.has(e.target));
      if (!involved) opacity = Math.min(opacity, 0.08);
      else active = true;
    }
    return { opacity, active };
  }

  return (
    <svg ref={svgRef} className="graph-canvas" role="img" aria-label="Hypha resource dependency graph">
      <defs>
        <radialGradient id="node-glow" cx="50%" cy="35%" r="65%">
          <stop offset="0%" stopColor="rgba(255,255,255,0.16)" />
          <stop offset="100%" stopColor="rgba(255,255,255,0)" />
        </radialGradient>
      </defs>
      <g ref={gRef}>
        <g className="layer-guides">
          {Array.from({ length: layerCount }).map((_, l) => (
            <text
              key={l}
              x={l * 260}
              y={bounds.minY - 90}
              className="layer-label"
              textAnchor="middle"
            >
              {l === 0 ? "roots" : `depth ${l}`}
            </text>
          ))}
        </g>

        <g className="threads">
          {edges.map((e) => {
            const { opacity, active } = edgeState(e);
            const targetColor = ACTION_COLOR[e.targetNode.action] ?? "var(--thread-dormant)";
            const d = taperedThreadPath(
              { x: e.sourceNode.x + NODE_RADIUS, y: e.sourceNode.y },
              { x: e.targetNode.x - NODE_RADIUS, y: e.targetNode.y },
              e.id,
              active ? 4.5 : 3,
              active ? 2.2 : 1.1
            );
            return (
              <path
                key={e.id}
                d={d}
                fill={active ? targetColor : "var(--thread-dormant-fill)"}
                stroke={active ? targetColor : "var(--thread-dormant)"}
                strokeWidth={0.5}
                opacity={opacity}
                style={{ transition: "opacity 220ms ease, fill 220ms ease" }}
              />
            );
          })}
        </g>

        <g className="nodes">
          {nodes.map((n) => {
            const isSelected = n.id === selectedId;
            const isHovered = n.id === hoveredId;
            const ringColor = KIND_COLOR[n.kind] ?? "var(--text-muted)";
            const fillColor = ACTION_COLOR[n.action] ?? "var(--action-none)";
            const radius = n.kind === "Package" || n.kind === "Symlink" ? NODE_RADIUS - 4 : NODE_RADIUS;
            return (
              <g
                key={n.id}
                className="node-hit"
                transform={`translate(${n.x}, ${n.y})`}
                opacity={nodeOpacity(n.id)}
                style={{ cursor: "pointer", transition: "opacity 220ms ease" }}
                onClick={() => onSelect(isSelected ? null : n.id)}
                onKeyDown={(e) => {
                  if (e.key === "Enter" || e.key === " ") {
                    e.preventDefault();
                    onSelect(isSelected ? null : n.id);
                  }
                }}
                onMouseEnter={() => onHover(n.id)}
                onMouseLeave={() => onHover(null)}
                tabIndex={0}
                role="button"
                aria-label={`${n.kind} ${n.name}, action ${n.action}`}
              >
                {n.action === "Create" && (
                  <circle r={radius + 6} className="pulse-ring" stroke={fillColor} fill="none" />
                )}
                <circle
                  r={radius}
                  fill="var(--surface-raised)"
                  stroke={ringColor}
                  strokeWidth={isSelected || isHovered ? 3 : 2}
                />
                <circle r={radius} fill="url(#node-glow)" />
                <circle r={Math.max(4, radius - 10)} fill={fillColor} opacity={0.9} />
                <text className="node-glyph" textAnchor="middle" dominantBaseline="central">
                  {n.kind[0]}
                </text>
                <text className="node-label" textAnchor="middle" y={radius + 18}>
                  {n.name}
                </text>
              </g>
            );
          })}
        </g>
      </g>
    </svg>
  );
}
