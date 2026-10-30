import { test } from 'node:test';
import assert from 'node:assert/strict';
import { chartData, completedLossAverage, formatBytes, zoomRange } from './chart.js';

test('keeps numeric steps in order and missing measurements as gaps, not zeroes', () => {
  const metrics = [
    { training_step: 3, loss: 0 },
    { training_step: 1, loss: 2 },
    { training_step: 2, loss: null },
    { training_step: 4, loss: Infinity },
    { training_step: NaN, loss: 8 },
  ];
  assert.deepEqual(chartData(metrics, ['loss']), [{
    key: 'loss', data: [{ x: 1, y: 2 }, { x: 2, y: null }, { x: 3, y: 0 }, { x: 4, y: null }],
  }]);
  assert.equal(metrics[0].training_step, 3);
});

test('supports empty data and independent series at the same steps', () => {
  assert.deepEqual(chartData([], ['loss']), [{ key: 'loss', data: [] }]);
  assert.deepEqual(chartData([{ training_step: 7, total: 3, value: 1 }], ['total', 'value']), [
    { key: 'total', data: [{ x: 7, y: 3 }] },
    { key: 'value', data: [{ x: 7, y: 1 }] },
  ]);
});

test('converts throughput to seconds, minutes or hours while preserving native values and gaps', () => {
  const metrics = [
    { training_step: 1, samples_per_second: 100, loss: 2 },
    { training_step: 2, samples_per_second: 200, loss: 4 },
    { training_step: 3, samples_per_second: null, loss: 1 },
    { training_step: 4, samples_per_second: 0, loss: 0 },
  ];
  for (const factor of [1, 60, 3600]) {
    const [throughput] = chartData(metrics, ['samples_per_second'], 1, 600, factor);
    assert.deepEqual(throughput.data, [{ x: 1.5, y: 150 * factor }, { x: 3, y: null }, { x: 4, y: 0 }]);
  }
  const [loss] = chartData(metrics, ['loss'], 1, 600);
  assert.deepEqual(loss.data, [{ x: 2.5, y: 1.75 }]);
  assert.equal(metrics[0].samples_per_second, 100);
});

test('reports only completed global 1,000-step loss averages and holds through a partial bucket', () => {
  const metrics = Array.from({ length: 2400 }, (_, index) => ({
    training_step: index + 1, total_loss: index < 1000 ? 6 : 3, policy_loss: 2, value_loss: 1,
  }));
  assert.equal(completedLossAverage([]), null);
  assert.equal(completedLossAverage(metrics.slice(0, 999)), null);
  assert.deepEqual(completedLossAverage(metrics.slice(0, 1000)), {
    training_step: 1000, total_loss: 6, policy_loss: 2, value_loss: 1,
  });
  assert.deepEqual(completedLossAverage(metrics), {
    training_step: 2000, total_loss: 3, policy_loss: 2, value_loss: 1, previous_total_loss: 6,
  });
  assert.equal(metrics[0].total_loss, 6);
  assert.equal(metrics.at(-1).training_step, 2400);
});

test('skips loss buckets with gaps, duplicates or missing measurements', () => {
  const metrics = Array.from({ length: 2000 }, (_, index) => ({
    training_step: index + 1, total_loss: 3, policy_loss: 2, value_loss: 1,
  }));
  assert.equal(completedLossAverage(metrics.filter(metric => metric.training_step !== 1500)).training_step, 1000);
  assert.equal(completedLossAverage([...metrics, metrics.at(-1)]).training_step, 1000);
  metrics[1499].value_loss = null;
  assert.equal(completedLossAverage(metrics).training_step, 1000);
  assert.equal(completedLossAverage(metrics.slice(1000)), null);
});

test('compares only immediately consecutive completed loss buckets', () => {
  const metrics = Array.from({ length: 3000 }, (_, index) => ({
    training_step: index + 1, total_loss: index < 2000 ? 4 : 3, policy_loss: 2, value_loss: 1,
  }));
  assert.equal(completedLossAverage(metrics).previous_total_loss, 4);
  assert.equal(completedLossAverage(metrics.slice(0, 1000)).previous_total_loss, undefined);
  const gap = metrics.filter(metric => metric.training_step !== 1500);
  assert.equal(completedLossAverage(gap).training_step, 3000);
  assert.equal(completedLossAverage(gap).previous_total_loss, undefined);
  metrics[1499].total_loss = Infinity;
  assert.equal(completedLossAverage(metrics).previous_total_loss, undefined);
});

test('formats memory with a readable SI unit without treating missing data as zero', () => {
  assert.equal(formatBytes(0), '0 B');
  assert.equal(formatBytes(999), '999 B');
  assert.equal(formatBytes(1000), '1 KB');
  assert.equal(formatBytes(1250000), '1.25 MB');
  assert.equal(formatBytes(1500000000), '1.5 GB');
  assert.equal(formatBytes(2000000000000), '2 TB');
  assert.equal(formatBytes(null), '—');
  assert.equal(formatBytes(Infinity), '—');
});

test('averages equal step intervals and recomputes detail for the visible range', () => {
  const metrics = Array.from({ length: 600 }, (_, i) => ({ training_step: i + 1, loss: i % 2 ? 4 : 0 }));
  const full = chartData(metrics, ['loss'], 1, 600)[0].data;
  assert.equal(full.length, 150);
  assert.deepEqual(full[0], { x: 2.5, y: 2 });
  assert.deepEqual(full.at(-1), { x: 598.5, y: 2 });
  assert.deepEqual(chartData(metrics, ['loss'], 2, 3)[0].data, [{ x: 2, y: 4 }, { x: 3, y: 0 }]);
  assert.equal(metrics[0].loss, 0);
});

test('does not average across missing values or manufacture points in empty intervals', () => {
  const metrics = [
    { training_step: 1, loss: 2 }, { training_step: 2, loss: 4 },
    { training_step: 3, loss: null }, { training_step: 4, loss: 10 },
    { training_step: 600, loss: 8 },
  ];
  assert.deepEqual(chartData(metrics, ['loss'], 1, 600)[0].data, [
    { x: 1.5, y: 3 }, { x: 3, y: null }, { x: 4, y: 10 }, { x: 600, y: 8 },
  ]);
});

test('zooms around the cursor while keeping its relative position', () => {
  assert.deepEqual(zoomRange(100, 500, 200, 0.5, 0, 1000), [150, 350]);
  assert.deepEqual(zoomRange(150, 350, 200, 2, 0, 1000), [100, 500]);
});

test('keeps zoom inside recorded steps and stops at one step or the full range', () => {
  assert.deepEqual(zoomRange(0, 100, 20, 2, 0, 1000), [0, 200]);
  assert.deepEqual(zoomRange(900, 1000, 980, 2, 0, 1000), [800, 1000]);
  assert.deepEqual(zoomRange(100, 500, 200, 100, 0, 1000), [0, 1000]);
  assert.deepEqual(zoomRange(100, 101, 100.5, 0.1, 0, 1000), [100, 101]);
  assert.deepEqual(zoomRange(7, 7, 7, 0.5, 7, 7), [7, 7]);
});
