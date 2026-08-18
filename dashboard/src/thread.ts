interface Point {
  x: number;
  y: number;
}

function seededJitter(seed: string, amplitude: number): number {
  let h = 0;
  for (let i = 0; i < seed.length; i++) {
    h = (h * 31 + seed.charCodeAt(i)) >>> 0;
  }
  const unit = (h % 1000) / 1000 - 0.5; 
  return unit * amplitude * 2;
}

function cubicPoint(p0: Point, p1: Point, p2: Point, p3: Point, t: number): Point {
  const mt = 1 - t;
  const a = mt * mt * mt;
  const b = 3 * mt * mt * t;
  const c = 3 * mt * t * t;
  const d = t * t * t;
  return {
    x: a * p0.x + b * p1.x + c * p2.x + d * p3.x,
    y: a * p0.y + b * p1.y + c * p2.y + d * p3.y,
  };
}

function cubicTangent(p0: Point, p1: Point, p2: Point, p3: Point, t: number): Point {
  const mt = 1 - t;
  const a = 3 * mt * mt;
  const b = 6 * mt * t;
  const c = 3 * t * t;
  return {
    x: a * (p1.x - p0.x) + b * (p2.x - p1.x) + c * (p3.x - p2.x),
    y: a * (p1.y - p0.y) + b * (p2.y - p1.y) + c * (p3.y - p2.y),
  };
}

export function threeadControlPoints(source: Point, target: Point, seed: string): [Point, Point, Point, Point] {
  const dx = target.x - source.x;
  const jitter = seededJitter(seed, Math.min(24, Math.abs(dx) * 0.08));
  const p1 = { x: source.x + dx * 0.42, y: source.y + jitter };
  const p2 = { x: source.x + dx * 0.58, y: target.y - jitter };
  return [source, p1, p2, target];
}

export function taperedThreadPath(
  source: Point,
  target: Point,
  seed: string,
  w0: number,
  w1: number,
  samples = 24
): string {
  const [p0, p1, p2, p3] = threeadControlPoints(source, target, seed);

  const top: Point[] = [];
  const bottom: Point[] = [];

  for (let i = 0; i <= samples; i++) {
    const t = i / samples;
    const pt = cubicPoint(p0, p1, p2, p3, t);
    const tan = cubicTangent(p0, p1, p2, p3, t);
    const len = Math.hypot(tan.x, tan.y) || 1;
    const nx = -tan.y / len;
    const ny = tan.x / len;
    const width = (w0 + (w1 - w0) * t) / 2;
    top.push({ x: pt.x + nx * width, y: pt.y + ny * width });
    bottom.push({ x: pt.x - nx * width, y: pt.y - ny * width });
  }

  const topPath = top.map((p, i) => `${i === 0 ? "M" : "L"}${p.x.toFixed(2)},${p.y.toFixed(2)}`).join(" ");
  const bottomPath = bottom
    .slice()
    .reverse()
    .map((p) => `L${p.x.toFixed(2)},${p.y.toFixed(2)}`)
    .join(" ");

  return `${topPath} ${bottomPath} Z`;
}

export function threadCenterlinePath(source: Point, target: Point, seed: string): string {
  const [p0, p1, p2, p3] = threeadControlPoints(source, target, seed);
  return `M${p0.x},${p0.y} C${p1.x},${p1.y} ${p2.x},${p2.y} ${p3.x},${p3.y}`;
}
