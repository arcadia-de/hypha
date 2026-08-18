import type { HyphaResource } from "../types";

interface Props {
  resource: HyphaResource | null;
  dependsOnNames: { id: string; name: string }[];
  dependentsNames: { id: string; name: string }[];
  onSelect: (id: string) => void;
  onClose: () => void;
}

export function NodeDetail({ resource, dependsOnNames, dependentsNames, onSelect, onClose }: Props) {
  if (!resource) return null;

  const labelEntries = Object.entries(resource.labels);
  const annotationEntries = Object.entries(resource.annotations);

  return (
    <aside className="detail-panel" aria-live="polite">
      <button className="detail-close" onClick={onClose} aria-label="Close detail panel">
        ×
      </button>

      <div className="detail-kind">{resource.kind}</div>
      <h2 className="detail-name">{resource.name}</h2>
      <div className="detail-id">{resource.id}</div>

      <div className={`detail-action detail-action-${resource.action.toLowerCase()}`}>
        <span className="detail-action-label">{resource.action}</span>
        <span className="detail-action-reason">{resource.reason}</span>
      </div>

      {labelEntries.length > 0 && (
        <section className="detail-section">
          <h3>Labels</h3>
          <dl className="kv-list">
            {labelEntries.map(([k, v]) => (
              <div className="kv-row" key={k}>
                <dt>{k}</dt>
                <dd>{v}</dd>
              </div>
            ))}
          </dl>
        </section>
      )}

      {annotationEntries.length > 0 && (
        <section className="detail-section">
          <h3>Annotations</h3>
          <dl className="kv-list">
            {annotationEntries.map(([k, v]) => (
              <div className="kv-row" key={k}>
                <dt>{k}</dt>
                <dd>{v}</dd>
              </div>
            ))}
          </dl>
        </section>
      )}

      <section className="detail-section">
        <h3>Spec</h3>
        <pre className="spec-block">{JSON.stringify(resource.spec, null, 2)}</pre>
      </section>

      <section className="detail-section">
        <h3>Depends on ({dependsOnNames.length})</h3>
        {dependsOnNames.length === 0 ? (
          <p className="detail-empty">nothing — this is a root of the graph</p>
        ) : (
          <ul className="link-list">
            {dependsOnNames.map((d) => (
              <li key={d.id}>
                <button className="link-item" onClick={() => onSelect(d.id)}>
                  {d.name}
                </button>
              </li>
            ))}
          </ul>
        )}
      </section>

      <section className="detail-section">
        <h3>Depended on by ({dependentsNames.length})</h3>
        {dependentsNames.length === 0 ? (
          <p className="detail-empty">nothing schedules after this resource</p>
        ) : (
          <ul className="link-list">
            {dependentsNames.map((d) => (
              <li key={d.id}>
                <button className="link-item" onClick={() => onSelect(d.id)}>
                  {d.name}
                </button>
              </li>
            ))}
          </ul>
        )}
      </section>
    </aside>
  );
}
