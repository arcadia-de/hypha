import styles from './Sidebar.module.scss';
import type { ResourceKind } from "../types";
import {
  CircleQuestionMark
} from "lucide-react";
 
interface Props {
  kinds: ResourceKind[];
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
  kinds,
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
    <aside className={styles.sidebar}>
      <div className={styles['sidebar-brand']}>
        <img src="/favicon.ico" className={styles['sidebar-brand-mark']}></img>
        <div className={styles['sidebar-brand-title']}>Hypha</div>
      </div>

      <div className={styles['sidebar-section']}>
        <label className={styles['sidebar-label']}
               htmlFor="search">
          Find a resource
        </label>
        <input
          id="search"
          className={styles['sidebar-search']}
          type="text"
          placeholder="name contains…"
          value={search}
          onChange={(e) => onSearch(e.target.value)}/>
      </div>

      <div className={styles['sidebar-section']}>
        <div className={styles['sidebar-label']}>Kind</div>
        <div className={styles['kind-list']}>
          {kinds.map((k) => (
            <button
              key={k}
              className={`${styles['kind-row']} ${activeKinds.has(k) ? styles['on'] : styles['off']}`}
              onClick={() => onToggleKind(k)}
              aria-pressed={activeKinds.has(k)}>
              <span className={`dot-sm kind-${k.toLowerCase()}`}></span>
              <span className={styles['kind-name']}>{k}</span>
              <span className={styles['kind-count']}>{counts[k] ?? 0}</span>
            </button>
          ))}
        </div>
      </div>

      <div className={styles['sidebar-section']}>
        <label className={styles['sidebar-label']}
               htmlFor="label-filter">
          Label selector
        </label>
        <select
          id="label-filter"
          className={styles['sidebar-select']}
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

      <div className={styles['sidebar-section']}>
        <div className={styles['sidebar-label']}>Plan, on next apply</div>
        <div className={styles['action-legend']}>
          {(["Create", "Update", "Delete", "None"] as const).map((a) => (
            <div className={styles['action-legend-row']} key={a}>
              <span className={`dot-sm action-${a.toLowerCase()}`} />
              <span className={styles['action-name']}>{a}</span>
              <span className={styles['action-count']}>{actionCounts[a] ?? 0}</span>
            </div>
          ))}
        </div>
      </div>

      <div className={styles['sidebar-footer']}>
        <button className={styles['sidebar-help']}>
          <CircleQuestionMark size={24} strokeWidth={2} />
          <span>Help</span>
        </button>
      </div>
    </aside>
  );
}
