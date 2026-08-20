import type { HyphaResource } from "../types";
import styles from './NodeDetail.module.scss';

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
    <aside className={styles['detail-panel']} aria-live="polite">
      <button className={styles['detail-close']} onClick={onClose} aria-label="Close detail panel">
        ×
      </button>

      <div className={styles['detail-kind']}>{resource.kind}</div>
      <h2 className={styles['detail-name']}>{resource.name}</h2>
      <div className={styles['detail-id']}>{resource.id}</div>

      <div className={`${styles[`detail-action`]} ${styles[`detail-action-${resource.action.toLowerCase()}`]}`}>
        <span className={styles['detail-action-label']}>{resource.action}</span>
        <span className={styles['detail-action-reason']}>{resource.reason}</span>
      </div>

      {labelEntries.length > 0 && (
        <section className={styles['detail-section']}>
          <h3>Labels</h3>
          <dl className="kv-list">
            {labelEntries.map(([k, v]) => (
              <div className={styles['kv-row']} key={k}>
                <dt>{k}</dt>
                <dd>{v}</dd>
              </div>
            ))}
          </dl>
        </section>
      )}

      {annotationEntries.length > 0 && (
        <section className={styles['detail-section']}>
          <h3>Annotations</h3>
          <dl className={styles['kv-list']}>
            {annotationEntries.map(([k, v]) => (
              <div className={styles['kv-row']} key={k}>
                <dt>{k}</dt>
                <dd>{v}</dd>
              </div>
            ))}
          </dl>
        </section>
      )}

      <section className={styles['detail-section']}>
        <h3>Spec</h3>
        <pre className={styles['spec-block']}>{JSON.stringify(resource.spec, null, 2)}</pre>
      </section>

      <section className={styles['detail-section']}>
        <h3>Depends on ({dependsOnNames.length})</h3>
        {dependsOnNames.length === 0 ? (
          <p className={styles['detail-empty']}>nothing — this is a root of the graph</p>
        ) : (
          <ul className={styles['link-list']}>
            {dependsOnNames.map((d) => (
              <li key={d.id}>
                <button className={styles['link-item']} onClick={() => onSelect(d.id)}>
                  {d.name}
                </button>
              </li>
            ))}
          </ul>
        )}
      </section>

      <section className={styles['detail-section']}>
        <h3>Depended on by ({dependentsNames.length})</h3>
        {dependentsNames.length === 0 ? (
          <p className={styles['detail-empty']}>nothing schedules after this resource</p>
        ) : (
          <ul className={styles['link-list']}>
            {dependentsNames.map((d) => (
              <li key={d.id}>
                <button className={styles['link-item']} onClick={() => onSelect(d.id)}>
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
