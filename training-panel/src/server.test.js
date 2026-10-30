import { test } from 'node:test';
import assert from 'node:assert/strict';
import { mkdtemp, writeFile, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { spawn } from 'node:child_process';
import { once } from 'node:events';
import { createPanelServer } from '../server.mjs';

test('records a user stop with the terminated process identity', async (t) => {
  const directory = await mkdtemp(join(tmpdir(), 'alphadiamond-stop-'));
  const binary = join(directory, 'alphadiamond-train');
  await writeFile(binary, 'setInterval(() => {}, 1000);');
  const child = spawn(process.execPath, [binary, 'train', '--run-dir', directory]);
  const server = createPanelServer({ binary });
  t.after(async () => {
    if (child.exitCode === null && child.signalCode === null) child.kill();
    server.closeAllConnections();
    await new Promise(resolve => server.close(resolve));
    await rm(directory, { recursive: true, force: true });
  });
  await once(child, 'spawn');
  const processStat = await readFile(`/proc/${child.pid}/stat`, 'utf8');
  const startTicks = processStat.slice(processStat.lastIndexOf(')') + 2).split(' ')[19];
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  const exited = once(child, 'exit');
  const before = Date.now();
  const response = await fetch(`http://127.0.0.1:${server.address().port}/api/runs/${child.pid}/stop`, { method: 'POST' });
  assert.equal(response.status, 200);
  assert.deepEqual(await exited, [null, 'SIGKILL']);
  const stop = JSON.parse(await readFile(join(directory, 'user-stop.json'), 'utf8'));
  assert.equal(stop.pid, child.pid);
  assert.equal(stop.startTicks, startTicks);
  assert.equal(stop.source, 'training-panel');
  assert.ok(Date.parse(stop.requestedAt) >= before && Date.parse(stop.requestedAt) <= Date.now());
});

test('shows progress only for the current process, stage and iteration', async (t) => {
  const directory = await mkdtemp(join(tmpdir(), 'alphadiamond-progress-'));
  const binary = join(directory, 'alphadiamond-train');
  await writeFile(binary, 'setInterval(() => {}, 1000);');
  await writeFile(join(directory, 'state.json'), JSON.stringify({ stage: 'TRAIN', iteration: 2 }));
  const child = spawn(process.execPath, [binary, 'train', '--run-dir', directory]);
  const server = createPanelServer({ binary });
  t.after(async () => {
    child.kill();
    server.closeAllConnections();
    await new Promise(resolve => server.close(resolve));
    await rm(directory, { recursive: true, force: true });
  });
  await once(child, 'spawn');
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  const progress = { pid: child.pid, stage: 'TRAIN', iteration: 2, completed: 3, total: 8 };
  for (const [record, expected] of [
    [progress, progress],
    [{ ...progress, pid: child.pid + 1 }, null],
    [{ ...progress, stage: 'SELF_PLAY' }, null],
    [{ ...progress, iteration: 1 }, null],
  ]) {
    await writeFile(join(directory, 'progress.json'), JSON.stringify(record));
    const response = await fetch(`http://127.0.0.1:${server.address().port}/api/status`);
    assert.equal(response.status, 200);
    const { jobs, resources } = await response.json();
    assert.deepEqual(jobs.find(job => job.pid === child.pid).progress, expected);
    assert.ok(resources.timestamp > 0);
    assert.ok(resources.memory.total > 0 && resources.memory.used <= resources.memory.total);
    assert.ok(resources.processor.usage === null || (resources.processor.usage >= 0 && resources.processor.usage <= 100));
  }
});
