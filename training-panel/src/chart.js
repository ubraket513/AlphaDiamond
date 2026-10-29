export function chartData(metrics, keys) {
  if (!metrics.length) return null;
  const x = [Infinity, -Infinity];
  const y = [Infinity, -Infinity];
  for (const metric of metrics) {
    x[0] = Math.min(x[0], metric.training_step);
    x[1] = Math.max(x[1], metric.training_step);
    for (const key of keys) {
      y[0] = Math.min(y[0], metric[key]);
      y[1] = Math.max(y[1], metric[key]);
    }
  }
  const lines = keys.map((key) => ({
    key,
    points: metrics.map((m) => {
      const px = x[0] === x[1] ? 50 : (m.training_step - x[0]) / (x[1] - x[0]) * 100;
      const py = y[0] === y[1] ? 50 : 100 - (m[key] - y[0]) / (y[1] - y[0]) * 100;
      return `${px},${py}`;
    }).join(' '),
  }));
  return { x, y, lines };
}
