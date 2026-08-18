import type { ResourceKind } from "../types";

const ALL_KINDS: ResourceKind[] = [
  "PackageManager",
  "Package",
  "Symlink",
  "Repository",
  "Font",
  "Controller",
];

const KIND_COLOR: Record<ResourceKind, string> = {
  PackageManager: "var(--kind-packagemanager)",
  Package: "var(--kind-package)",
  Symlink: "var(--kind-symlink)",
  Repository: "var(--kind-repository)",
  Font: "var(--kind-font)",
  Controller: "var(--kind-controller)",
};

interface Props {
  search: string;
  onSearch: (v: string) => void;
  activeKinds: Set<ResourceKind>;
  onToggleKind: (k: ResourceKind) => void;
  labelOptions: string[];
  labelFilter: string | null;
  onLabelFilter: (v: string | null) => void;
  counts: Record<string, number>;
  actionCounts: Record<string, number>;
}

export function Sidebar({
  search,
  onSearch,
  activeKinds,
  onToggleKind,
  labelOptions,
  labelFilter,
  onLabelFilter,
  counts,
  actionCounts,
}: Props) {
  return (
    <aside className="sidebar">
      <div className="sidebar-brand">
        <span className="sidebar-brand-mark">⌗</span>
        <div>
          <div className="sidebar-brand-title">Hypha</div>
          <div className="sidebar-brand-sub">resource graph</div>
        </div>
      </div>

      <div className="sidebar-section">
        <label className="sidebar-label" htmlFor="search">
          Find a resource
        </label>
        <input
          id="search"
          className="sidebar-search"
          type="text"
          placeholder="name contains…"
          value={search}
          onChange={(e) => onSearch(e.target.value)}
        />
      </div>

      <div className="sidebar-section">
        <div className="sidebar-label">Kind</div>
        <div className="kind-list">
          {ALL_KINDS.map((k) => (
            <button
              key={k}
              className={`kind-row ${activeKinds.has(k) ? "on" : "off"}`}
              onClick={() => onToggleKind(k)}
              aria-pressed={activeKinds.has(k)}
            >
              <span className="kind-dot" style={{ background: KIND_COLOR[k] }} />
              <span className="kind-name">{k}</span>
              <span className="kind-count">{counts[k] ?? 0}</span>
            </button>
          ))}
        </div>
      </div>

      <div className="sidebar-section">
        <label className="sidebar-label" htmlFor="label-filter">
          Label selector
        </label>
        <select
          id="label-filter"
          className="sidebar-select"
          value={labelFilter ?? ""}
          onChange={(e) => onLabelFilter(e.target.value || null)}
        >
          <option value="">any label</option>
          {labelOptions.map((opt) => (
            <option key={opt} value={opt}>
              {opt}
            </option>
          ))}
        </select>
      </div>

      <div className="sidebar-section">
        <div className="sidebar-label">Plan, on next apply</div>
        <div className="action-legend">
          {(["Create", "Update", "Delete", "None"] as const).map((a) => (
            <div className="action-legend-row" key={a}>
              <span className={`action-dot action-${a.toLowerCase()}`} />
              <span className="action-name">{a}</span>
              <span className="action-count">{actionCounts[a] ?? 0}</span>
            </div>
          ))}
        </div>
      </div>

      <div className="sidebar-footer">
        drag to pan · scroll to zoom · click a node to inspect
      </div>
    </aside>
  );
}
