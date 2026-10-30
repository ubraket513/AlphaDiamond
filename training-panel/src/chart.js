export function chartData(metrics, keys, from, to, scale = 1) {
  const ordered = metrics.filter((m) => Number.isFinite(m.training_step)
    && (from == null || m.training_step >= from) && (to == null || m.training_step <= to))
    .sort((a, b) => a.training_step - b.training_step);
  const start = from ?? ordered[0]?.training_step ?? 0;
  const width = Math.max(1, ((to ?? ordered.at(-1)?.training_step ?? start) - start + 1) / 150);
  return keys.map((key) => {
    const groups = [];
    for (const metric of ordered) {
      const x = metric.training_step;
      const y = Number.isFinite(metric[key]) ? metric[key] : null;
      const bucket = Math.min(149, Math.floor((x - start) / width));
      const previous = groups.at(-1);
      // Missing samples split a bucket so the chart keeps visible gaps.
      if (y !== null && previous?.y !== null && previous?.bucket === bucket) {
        previous.x += x;
        previous.y += y;
        previous.count++;
      } else {
        groups.push({ x, y, bucket, count: 1 });
      }
    }
    return { key, data: groups.map(({ x, y, count }) => ({ x: x / count, y: y === null ? null : y / count * scale })) };
  });
}

export function formatBytes(value) {
  if (!Number.isFinite(value) || value < 0) return '—';
  const unit = Math.min(4, Math.floor(Math.log10(Math.max(value, 1)) / 3));
  return `${new Intl.NumberFormat('en', { maximumFractionDigits: 2 }).format(value / 1000 ** unit)} ${['B', 'KB', 'MB', 'GB', 'TB'][unit]}`;
}

export function completedLossAverage(metrics) {
  let end, count, total, policy, value, valid;
  let latest = null;
  for (let index = metrics.length - 1; index >= 0; index--) {
    const metric = metrics[index];
    const step = metric.training_step;
    if (!Number.isSafeInteger(step) || step <= 0) continue;
    const bucket = Math.ceil(step / 1000) * 1000;
    if (bucket !== end) {
      end = bucket;
      count = total = policy = value = 0;
      valid = true;
    }
    valid &&= step === end - count
      && [metric.total_loss, metric.policy_loss, metric.value_loss].every(Number.isFinite);
    count++;
    total += metric.total_loss;
    policy += metric.policy_loss;
    value += metric.value_loss;
    if (valid && count === 1000) {
      const average = {
        training_step: end, total_loss: total / 1000, policy_loss: policy / 1000, value_loss: value / 1000,
      };
      if (!latest) latest = average;
      else {
        if (latest.training_step - end === 1000) latest.previous_total_loss = average.total_loss;
        return latest;
      }
    }
  }
  return latest;
}

export function appendResourceSample(history, sample) {
  const points = history.length ? history : Array.from({ length: 119 }, (_, index) => ({
    timestamp: sample.timestamp - (119 - index) * 1000,
    processor: { usage: 0, cores: sample.processor?.cores?.map(({ id }) => ({ id, usage: 0 })) ?? [] },
    memory: { usage: 0, swapUsage: 0 }, gpu: { usage: 0, vramUsage: 0, encoderUsage: 0, decoderUsage: 0 },
  }));
  return [...points, sample].slice(-120);
}

export function zoomRange(from, to, anchor, factor, minimum, maximum) {
  if (to <= from || maximum <= minimum) return [from, to];
  const width = Math.min(maximum - minimum, Math.max(1, Math.round((to - from) * factor)));
  const start = Math.round(Math.max(minimum, Math.min(maximum - width,
    anchor - (anchor - from) / (to - from) * width)));
  return [start, start + width];
}
