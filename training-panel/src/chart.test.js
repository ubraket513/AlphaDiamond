import test from 'node:test';
import assert from 'node:assert/strict';
import { chartData } from './chart.js';

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
