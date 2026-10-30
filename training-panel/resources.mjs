import { readFile } from 'node:fs/promises';
import { execFile } from 'node:child_process';
import { promisify } from 'node:util';
import { cpus } from 'node:os';

const exec = promisify(execFile);
const processorName = cpus()[0]?.model ?? 'Processor';
let previousCpu = '';
let memoryHardware;
const number = value => value?.trim() && Number.isFinite(Number(value)) ? Number(value) : null;

export function cpuCounters(text) {
  const values = text.split('\n')[0].trim().split(/\s+/).slice(1, 9).map(Number);
  return { total: values.reduce((sum, value) => sum + value, 0), idle: values[3] + values[4] };
}

export function cpuUsage(current, previous) {
  if (!previous || current.total <= previous.total) return null;
  return Math.max(0, Math.min(100, 100 * (1 - (current.idle - previous.idle) / (current.total - previous.total))));
}

export function logicalCpuUsage(text, previous, info) {
  const before = Object.fromEntries([...previous.matchAll(/^(cpu\d+)\s+(.+)$/gm)].map(([, id, values]) => [id, cpuCounters(`${id} ${values}`)]));
  const frequencies = Object.fromEntries(info.trim().split(/\n\s*\n/).map(block => [
    block.match(/^processor\s*:\s*(\d+)/m)?.[1], number(block.match(/^cpu MHz\s*:\s*([\d.]+)/m)?.[1]),
  ]));
  return [...text.matchAll(/^cpu(\d+)\s+(.+)$/gm)].map(([, id, values]) => ({
    id: Number(id), usage: cpuUsage(cpuCounters(`cpu${id} ${values}`), before[`cpu${id}`]),
    frequencyMHz: frequencies[id] ?? null,
  }));
}

export function memoryUsage(text) {
  const fields = Object.fromEntries([...text.matchAll(/^(\w+):\s+(\d+) kB/gm)].map(([, key, value]) => [key, Number(value) * 1024]));
  const used = fields.MemTotal - fields.MemAvailable;
  const swapUsed = fields.SwapTotal - fields.SwapFree;
  return { used, total: fields.MemTotal, usage: used / fields.MemTotal * 100,
    swapUsed, swapTotal: fields.SwapTotal, swapUsage: fields.SwapTotal ? swapUsed / fields.SwapTotal * 100 : 0 };
}

export function memoryProperties(text) {
  const devices = new Map();
  for (const [, id, key, value] of text.matchAll(/^MEMORY_DEVICE_(\d+)_(\w+)=(.*)$/gm)) {
    if (!devices.has(id)) devices.set(id, {});
    devices.get(id)[key] = value;
  }
  if (!devices.size) return null;
  const populated = [...devices.values()].filter(device => Number(device.SIZE) > 0);
  const values = key => [...new Set(populated.map(device => device[key]).filter(value => value && value !== 'Unknown'))].join(' / ') || null;
  const speed = values('CONFIGURED_SPEED_MTS');
  return { slotsUsed: populated.length, slotsTotal: number(text.match(/^MEMORY_ARRAY_NUM_DEVICES=(\d+)/m)?.[1]),
    speed: speed ? `${speed} MT/s` : null, formFactor: values('FORM_FACTOR'), type: values('TYPE'), typeDetail: values('TYPE_DETAIL') };
}

export function gpuUsage(text) {
  if (!text.trim()) return null;
  const [name, ...fields] = text.trim().split('\n')[0].split(',').map(value => value.trim());
  const [usage, used, total, encoderUsage, decoderUsage, frequencyMHz, memoryFrequencyMHz, powerWatts, powerLimitWatts, temperatureC] = fields.map(number);
  const vramUsed = used == null ? null : used * 1024 ** 2, vramTotal = total == null ? null : total * 1024 ** 2;
  return { name, usage, vramUsed, vramTotal,
    vramUsage: vramTotal > 0 && vramUsed != null ? vramUsed / vramTotal * 100 : null,
    encoderUsage: encoderUsage ?? null, decoderUsage: decoderUsage ?? null,
    frequencyMHz: frequencyMHz ?? null, memoryFrequencyMHz: memoryFrequencyMHz ?? null,
    powerWatts: powerWatts ?? null, powerLimitWatts: powerLimitWatts ?? null, temperatureC: temperatureC ?? null };
}

export async function resourceSnapshot() {
  memoryHardware ??= exec('udevadm', ['info', '--query=property', '--path=/sys/devices/virtual/dmi/id'],
    { timeout: 1200, maxBuffer: 65536 }).then(result => memoryProperties(result.stdout)).catch(() => null);
  const [cpu, cpuInfo, memory, properties, gpu] = await Promise.all([
    readFile('/proc/stat', 'utf8'), readFile('/proc/cpuinfo', 'utf8'), readFile('/proc/meminfo', 'utf8'), memoryHardware,
    exec('nvidia-smi', ['--query-gpu=name,utilization.gpu,memory.used,memory.total,utilization.encoder,utilization.decoder,clocks.gr,clocks.mem,power.draw,power.limit,temperature.gpu', '--format=csv,noheader,nounits'],
      { timeout: 1200, maxBuffer: 16384 }).then(result => result.stdout).catch(() => ''),
  ]);
  const current = cpuCounters(cpu);
  const usage = cpuUsage(current, previousCpu ? cpuCounters(previousCpu) : null);
  const cores = logicalCpuUsage(cpu, previousCpu, cpuInfo);
  previousCpu = cpu;
  return { timestamp: Date.now(), processor: { name: processorName, usage, cores },
    memory: { ...memoryUsage(memory), properties }, gpu: gpuUsage(gpu) };
}
