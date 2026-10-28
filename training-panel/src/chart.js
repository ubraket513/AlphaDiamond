export function chartData(metrics, keys) {
  if (!metrics.length) return null;
  const steps = metrics.map((m) => m.training_step);
  const values = metrics.flatMap((m) => keys.map((key) => m[key]));
  const x = [Math.min(...steps), Math.max(...steps)];
  const y = [Math.min(...values), Math.max(...values)];
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
