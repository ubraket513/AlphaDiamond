import { test } from 'node:test';
import assert from 'node:assert/strict';
import { cpuCounters, cpuUsage, logicalCpuUsage, memoryUsage, memoryProperties, gpuUsage } from '../resources.mjs';
import { appendResourceSample } from './chart.js';

test('resource plots start with a zero baseline, retain real measurements and bound history', () => {
  const sample = { timestamp: 120000, processor: { usage: 17, cores: [{ id: 0, usage: 40 }, { id: 2, usage: 60 }] }, memory: { usage: 30 }, gpu: { usage: 98 } };
  const history = appendResourceSample([], sample);
  assert.equal(history.length, 120);
  assert.equal(history[0].timestamp, 1000);
  for (const point of history.slice(0, -1)) {
    assert.equal(point.processor.usage, 0);
    assert.equal(point.memory.swapUsage, 0);
    assert.equal(point.gpu.vramUsage, 0);
    assert.equal(point.gpu.encoderUsage, 0);
    assert.equal(point.gpu.decoderUsage, 0);
    assert.deepEqual(point.processor.cores, [{ id: 0, usage: 0 }, { id: 2, usage: 0 }]);
  }
  assert.equal(history.at(-1), sample);
  const next = { ...sample, timestamp: 121000 };
  const updated = appendResourceSample(history, next);
  assert.equal(updated.length, 120);
  assert.equal(updated.at(-2), sample);
  assert.equal(updated.at(-1), next);
  assert.equal(history[0].timestamp, 1000);
});

test('logical CPUs use their own counter deltas and hardware IDs, including newly online CPUs', () => {
  const before = 'cpu 0 0 0 0 0 0 0 0\ncpu0 10 0 0 90 0 0 0 0\ncpu2 50 0 0 50 0 0 0 0\n';
  const after = 'cpu 0 0 0 0 0 0 0 0\ncpu0 30 0 0 170 0 0 0 0\ncpu2 130 0 0 70 0 0 0 0\ncpu3 10 0 0 90 0 0 0 0\n';
  const info = 'processor : 0\ncpu MHz : 5500.25\n\nprocessor : 2\ncpu MHz : 624.19\n';
  const cores = logicalCpuUsage(after, before, info);
  assert.deepEqual(cores.map(core => core.id), [0, 2, 3]);
  assert.ok(Math.abs(cores[0].usage - 20) < 1e-9);
  assert.equal(cores[1].usage, 80);
  assert.equal(cores[2].usage, null);
  assert.equal(cores[0].frequencyMHz, 5500.25);
  assert.equal(cores[1].frequencyMHz, 624.19);
  assert.equal(cores[2].frequencyMHz, null);
  assert.equal(logicalCpuUsage(after, '', info)[0].usage, null);
});

test('memory properties count populated slots and preserve mixed device specifications', () => {
  const properties = memoryProperties('MEMORY_ARRAY_NUM_DEVICES=4\nMEMORY_DEVICE_0_PRESENT=0\nMEMORY_DEVICE_0_TYPE=Unknown\nMEMORY_DEVICE_1_SIZE=34359738368\nMEMORY_DEVICE_1_CONFIGURED_SPEED_MTS=5600\nMEMORY_DEVICE_1_FORM_FACTOR=DIMM\nMEMORY_DEVICE_1_TYPE=DDR5\nMEMORY_DEVICE_1_TYPE_DETAIL=Synchronous Unbuffered (Unregistered)\nMEMORY_DEVICE_3_SIZE=34359738368\nMEMORY_DEVICE_3_CONFIGURED_SPEED_MTS=5200\nMEMORY_DEVICE_3_FORM_FACTOR=DIMM\nMEMORY_DEVICE_3_TYPE=DDR5\n');
  assert.deepEqual(properties, { slotsUsed: 2, slotsTotal: 4, speed: '5600 / 5200 MT/s', formFactor: 'DIMM', type: 'DDR5', typeDetail: 'Synchronous Unbuffered (Unregistered)' });
  assert.equal(memoryProperties(''), null);
});

test('CPU uses interval deltas, excludes guest double counting and treats iowait as idle', () => {
  const before = cpuCounters('cpu  10 0 10 70 10 0 0 0 4 0\ncpu0 1 2 3');
  const after = cpuCounters('cpu  30 0 20 120 30 0 0 0 10 0\n');
  assert.deepEqual(before, { total: 100, idle: 80 });
  assert.ok(Math.abs(cpuUsage(after, before) - 30) < 1e-9);
  assert.equal(cpuUsage(after, null), null);
  assert.equal(cpuUsage(before, before), null);
});

test('memory uses MemAvailable and handles disabled swap without dividing by zero', () => {
  const memory = memoryUsage('MemTotal: 1000 kB\nMemAvailable: 250 kB\nSwapTotal: 0 kB\nSwapFree: 0 kB\n');
  assert.equal(memory.used, 750 * 1024);
  assert.equal(memory.usage, 75);
  assert.equal(memory.swapUsage, 0);
  const swap = memoryUsage('MemTotal: 1000 kB\nMemAvailable: 250 kB\nSwapTotal: 200 kB\nSwapFree: 50 kB\n');
  assert.equal(swap.swapUsage, 75);
});

test('GPU converts MiB to bytes and leaves unavailable utilization missing', () => {
  const gpu = gpuUsage('NVIDIA GeForce RTX 5080, 98, 2048, 16384\n');
  assert.equal(gpu.usage, 98);
  assert.equal(gpu.vramUsed, 2048 * 1024 ** 2);
  assert.equal(gpu.vramUsage, 12.5);
  assert.equal(gpuUsage(''), null);
  assert.equal(gpuUsage('NVIDIA GPU, [N/A], 0, 16384').usage, null);
});

test('GPU reads encoder, decoder, clocks, power and temperature without turning missing values into zero', () => {
  const gpu = gpuUsage('NVIDIA GPU, 99, 4096, 16384, 7, 3, 2842, 14801, 295.3, 360, 60');
  assert.equal(gpu.encoderUsage, 7);
  assert.equal(gpu.decoderUsage, 3);
  assert.equal(gpu.frequencyMHz, 2842);
  assert.equal(gpu.memoryFrequencyMHz, 14801);
  assert.equal(gpu.powerWatts, 295.3);
  assert.equal(gpu.powerLimitWatts, 360);
  assert.equal(gpu.temperatureC, 60);
  const unavailable = gpuUsage('NVIDIA GPU, , [N/A], 16384, [N/A], , [Not Supported], [N/A], , [N/A], [N/A]');
  for (const key of ['usage', 'vramUsed', 'vramUsage', 'encoderUsage', 'decoderUsage', 'frequencyMHz', 'memoryFrequencyMHz', 'powerWatts', 'powerLimitWatts', 'temperatureC']) assert.equal(unavailable[key], null, key);
});
