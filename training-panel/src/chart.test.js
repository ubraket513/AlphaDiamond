import test from 'node:test';
import assert from 'node:assert/strict';
import { chartData } from './chart.js';
import { readFile, writeFile, rm } from 'node:fs/promises';
import { randomUUID } from 'node:crypto';
import { fileURLToPath } from 'node:url';
import { compile } from 'svelte/compiler';
import { render } from 'svelte/server';

// Equal spacing would misrepresent sparse durable training-step samples.
test('chart x positions use actual training steps and a shared loss scale', () => {
  const result = chartData([
    { training_step: 10, total_loss: 4, policy_loss: 3 },
    { training_step: 30, total_loss: 2, policy_loss: 1 },
    { training_step: 90, total_loss: 0, policy_loss: 0 },
  ], ['total_loss', 'policy_loss']);
  assert.deepEqual(result.x, [10, 90]);
  assert.deepEqual(result.y, [0, 4]);
  assert.equal(result.lines[0].points, '0,0 25,50 100,100');
  assert.equal(result.lines[1].points, '0,25 25,75 100,100');
});

test('a single durable metric produces finite visible chart coordinates', () => {
  const result = chartData([{ training_step: 128, total_loss: 0 }], ['total_loss']);
  assert.match(result.lines[0].points, /^\d+(\.\d+)?,\d+(\.\d+)?$/);
  assert.doesNotMatch(result.lines[0].points, /NaN|Infinity/);
});

test('empty metrics retain an empty chart instead of invented training data', () => {
  assert.equal(chartData([], ['total_loss']), null);
});

test('charts all 50000 valid native rows without exceeding function argument limits', () => {
  const metrics = Array.from({ length: 50000 }, (_, index) => ({
    training_step: index + 1, total_loss: 3, policy_loss: 2, value_loss: 1,
  }));
  const result = chartData(metrics, ['total_loss', 'policy_loss', 'value_loss']);
  assert.deepEqual(result.x, [1, 50000]);
  assert.deepEqual(result.y, [1, 3]);
  assert.equal(result.lines.length, 3);
  for (const [index, line] of result.lines.entries()) {
    const points = line.points.split(' ');
    assert.equal(points.length, 50000);
    assert.equal(points[0], `0,${index * 50}`);
    assert.equal(points.at(-1), `100,${index * 50}`);
  }
});

test('single-sample loss markers render at each actual series coordinate', async (t) => {
  const filename = fileURLToPath(new URL('./Chart.svelte', import.meta.url));
  const { js } = compile(await readFile(filename, 'utf8'), { filename, generate: 'server' });
  // Compile beside the component so its real imports resolve, and remove the temporary module.
  const compiled = new URL(`./.chart-test-${randomUUID()}.mjs`, import.meta.url);
  t.after(() => rm(compiled, { force: true }));
  await writeFile(compiled, js.code);
  const { default: Chart } = await import(compiled.href);
  const { body } = render(Chart, { props: {
    metrics: [{ training_step: 128, total_loss: 3, policy_loss: 2, value_loss: 1 }],
    keys: ['total_loss', 'policy_loss', 'value_loss'], labels: ['Total', 'Policy', 'Value'], title: 'Loss',
  } });
  const markers = [...body.matchAll(/<circle\b[^>]*\bcx="([^"]+)"[^>]*\bcy="([^"]+)"/g)]
    .map(([, x, y]) => [Number(x), Number(y)]);
  assert.deepEqual(markers, [[50, 0], [50, 50], [50, 100]]);
});
