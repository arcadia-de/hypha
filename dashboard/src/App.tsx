import { useMemo, useState, useEffect } from "react";
import { GraphCanvas } from "./components/GraphCanvas";
import { Sidebar } from "./components/Sidebar";
import { NodeDetail } from "./components/NodeDetail";
import { resources } from "./data/mockGraph";
import { layoutGraph } from "./layout";
import type { ResourceKind } from "./types";

function computeRelated(selectedId: string, edges: { source: string; target: string }[]) {
  const upstream = new Set<string>();
  const downstream = new Set<string>();

  const parentsOf = new Map<string, string[]>();
  const childrenOf = new Map<string, string[]>();
  edges.forEach((e) => {
    if (!parentsOf.has(e.target)) parentsOf.set(e.target, []);
    parentsOf.get(e.target)!.push(e.source);
    if (!childrenOf.has(e.source)) childrenOf.set(e.source, []);
    childrenOf.get(e.source)!.push(e.target);
  });

  const stackUp = [...(parentsOf.get(selectedId) ?? [])];
  while (stackUp.length) {
    const id = stackUp.pop()!;
    if (upstream.has(id)) continue;
    upstream.add(id);
    stackUp.push(...(parentsOf.get(id) ?? []));
  }

  const stackDown = [...(childrenOf.get(selectedId) ?? [])];
  while (stackDown.length) {
    const id = stackDown.pop()!;
    if (downstream.has(id)) continue;
    downstream.add(id);
    stackDown.push(...(childrenOf.get(id) ?? []));
  }

  return new Set([...upstream, ...downstream]);
}

export default function App() {
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  const { nodes, edges } = useMemo(() => layoutGraph(resources), []);
  const resourceById = useMemo(() => new Map(resources.map((r) => [r.id, r])), []);

  const [search, setSearch] = useState("");
  const [activeKinds, setActiveKinds] = useState<Set<ResourceKind>>(new Set());
  const [labelFilter, setLabelFilter] = useState<string | null>(null);
  const [selectedId, setSelectedId] = useState<string | null>(null);
  const [hoveredId, setHoveredId] = useState<string | null>(null);

  useEffect(() => {
    fetch('/api/kinds')
      .then((res) => {
        if(!res.ok)
          throw new Error("failed to get kinds");
        return res.json();
      })
      .then((json) => {
        setActiveKinds(new Set(json.data || []));
        setLoading(false);
      })
      .catch(async(err) => {
        console.error(`failed to get kinds from api: `, err);
        setError(err.message);
        setLoading(false);

        if(import.meta.env.DEV || process.env.ENV == "development") {
          try {
            const {default: kinds } = await import("./kinds.json");
            setActiveKinds(new Set(kinds));
          } catch(importErr) {
            setActiveKinds(new Set());
          }
        } else {
          setActiveKinds(new Set());
        }
      });
  }, []);

  const labelOptions = useMemo(() => {
    const set = new Set<string>();
    resources.forEach((r) =>
      Object.entries(r.labels).forEach(([k, v]) => set.add(`${k}=${v}`))
    );
    return Array.from(set).sort();
  }, []);

  const visibleNodeIds = useMemo(() => {
    const set = new Set<string>();
    nodes.forEach((n) => {
      if (!activeKinds.has(n.kind)) return;
      if (search.trim() && !n.name.toLowerCase().includes(search.trim().toLowerCase())) return;
      if (labelFilter) {
        const [k, v] = labelFilter.split("=");
        if (n.labels[k] !== v) return;
      }
      set.add(n.id);
    });
    return set;
  }, [nodes, activeKinds, search, labelFilter]);

  const counts = useMemo(() => {
    const c: Record<string, number> = {};
    resources.forEach((r) => (c[r.kind] = (c[r.kind] ?? 0) + 1));
    return c;
  }, []);

  const actionCounts = useMemo(() => {
    const c: Record<string, number> = {};
    resources.forEach((r) => (c[r.action] = (c[r.action] ?? 0) + 1));
    return c;
  }, []);

  const relatedIds = useMemo(
    () => (selectedId ? computeRelated(selectedId, edges) : null),
    [selectedId, edges]
  );

  const selectedResource = selectedId ? resourceById.get(selectedId) ?? null : null;

  const dependsOnNames = useMemo(() => {
    if (!selectedResource) return [];
    return selectedResource.dependsOn.map((id) => ({
      id,
      name: resourceById.get(id)?.name ?? id,
    }));
  }, [selectedResource, resourceById]);

  const dependentsNames = useMemo(() => {
    if (!selectedId) return [];
    return resources
      .filter((r) => r.dependsOn.includes(selectedId))
      .map((r) => ({ id: r.id, name: r.name }));
  }, [selectedId]);

  function toggleKind(k: ResourceKind) {
    setActiveKinds((prev) => {
      const next = new Set(prev);
      if (next.has(k)) next.delete(k);
      else next.add(k);
      return next;
    });
  }

  return (
    <div className={`app ${selectedResource ? "with-detail" : ""}`}>
      <Sidebar
        kinds={Array.from(activeKinds)}
        search={search}
        onSearch={setSearch}
        activeKinds={activeKinds}
        onToggleKind={toggleKind}
        labelOptions={labelOptions}
        labelFilter={labelFilter}
        onLabelFilter={setLabelFilter}
        counts={counts}
        actionCounts={actionCounts} />

      <main className="canvas-wrap">
        <GraphCanvas
          nodes={nodes}
          edges={edges}
          visibleNodeIds={visibleNodeIds}
          selectedId={selectedId}
          hoveredId={hoveredId}
          relatedIds={relatedIds}
          onSelect={setSelectedId}
          onHover={setHoveredId} />
        {hoveredId && hoveredId !== selectedId && (
          <div className="hover-chip">
            {resourceById.get(hoveredId)?.kind} · {resourceById.get(hoveredId)?.name}
          </div>
        )}
      </main>

      <NodeDetail
        resource={selectedResource}
        dependsOnNames={dependsOnNames}
        dependentsNames={dependentsNames}
        onSelect={setSelectedId}
        onClose={() => setSelectedId(null)} />
    </div>
  );
}
