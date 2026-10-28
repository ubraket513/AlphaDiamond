import assert from 'node:assert/strict';
import { once } from 'node:events';
import { spawn } from 'node:child_process';
import { request as httpRequest } from 'node:http';
import { mkdtemp, mkdir, writeFile, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import test, { describe } from 'node:test';

// Each test owns its trainer, directory and ephemeral HTTP port. No real run is stopped.
async function fixture(t) {
  const root = await mkdtemp(join(tmpdir(), 'training-panel-'));
  const binary = join(root, 'alphadiamond-train');
  await writeFile(binary, `#!/usr/bin/env node
import fs from 'node:fs';
import {spawn} from 'node:child_process';
const args=process.argv.slice(2);
const dir=args[args.indexOf('--run-dir')+1];
if(args.includes('--child')) {
 const child=spawn(process.execPath, ['-e','setInterval(()=>{},1000)'], {stdio:'ignore'});
 fs.writeFileSync(dir+'/child.pid', String(child.pid));
}
if(args.includes('--config')) {
 const config=JSON.parse(fs.readFileSync(args[args.indexOf('--config')+1], 'utf8'));
 if(config.invalid) { console.error('native config rejected'); process.exit(2); }
 fs.writeFileSync(dir+'/resolved-config.json', JSON.stringify(config));
}
fs.writeFileSync(dir+'/invocation.json', JSON.stringify(args));
console.log('native output'); console.error('native stderr');
process.on('SIGTERM',()=>{});
setInterval(()=>{}, 1000);
`, { mode: 0o755 });
  const children = [];
  t.after(async () => {
    for (const child of children) {
      if (child.exitCode === null && child.signalCode === null) {
        const exited = once(child, 'exit');
        child.kill('SIGKILL');
        await exited;
      }
    }
    await rm(root, { recursive: true, force: true });
  });
  const { createPanelServer } = await import('./server.mjs');
  const server = createPanelServer({ root, binary });
  server.listen(0, '127.0.0.1');
  await once(server, 'listening');
  t.after(() => new Promise(resolve => server.close(resolve)));
  const url = `http://127.0.0.1:${server.address().port}`;
  const request = async (path, body) => {
    const res = await fetch(url + path, body === undefined ? {} : {
      method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body),
    });
    return { status: res.status, data: await res.json() };
  };
  const runDir = join(root, 'runs/min/fixture');
  await mkdir(runDir, { recursive: true });
  return { root, binary, runDir, request, children, url };
}

describe('native CLI bridge', { concurrency: true }, () => {
test('discovers an independently launched resume and preserves ordered native metadata', async t => {
  const { root, binary, runDir, request, children } = await fixture(t);
  await writeFile(join(runDir, 'state.json'), JSON.stringify({ stage: 'SELF_PLAY', iteration: 11 }));
  await writeFile(join(runDir, 'resolved-config.json'), JSON.stringify({ run_seed: 7, custom: { setting: 42 }, training: { learning_rate: 0.01 } }));
  await writeFile(join(runDir, 'console.log'), 'durable console\n');
  for (const iteration of [10, 2, 11]) await mkdir(join(runDir, `iterations/${iteration}`), { recursive: true });
  await writeFile(join(runDir, 'iterations/10/train.json'), JSON.stringify({ training_metrics: [{ training_step: 12, total_loss: 0.5, samples_per_second: 99, optimizer_seconds: 0.2 }] }));
  await writeFile(join(runDir, 'iterations/2/train.json'), JSON.stringify({ training_metrics: [{ training_step: 3, total_loss: 1, policy_loss: 0.6, value_loss: 0.4 }] }));
  await writeFile(join(runDir, 'iterations/2/selfplay.metrics.json'), JSON.stringify({ games_per_second: 2, turn_orders: [1, 2] }));
  await writeFile(join(runDir, 'iterations/2/replay_update.json'), JSON.stringify({ retained: 200 }));
  const child = spawn(binary, ['resume', '--run-dir', 'runs/min/fixture'], { cwd: root, stdio: 'ignore' });
  children.push(child);
  await once(child, 'spawn');
  const { data } = await request('/api/status');
  const job = data.jobs.find(job => job.pid === child.pid);
  assert.ok(job, 'must find a process with no panel registration');
  assert.equal(job.runDir, runDir);
  assert.equal(job.state.stage, 'SELF_PLAY');
  assert.deepEqual(job.config.custom, { setting: 42 });
  assert.deepEqual(job.metrics.map(row => [row.iteration, row.training_step]), [[2, 3], [10, 12]]);
  assert.equal(job.metrics[1].optimizer_seconds, 0.2);
  assert.deepEqual(job.iterations.map(row => row.iteration), [2, 10, 11]);
  assert.equal(job.iterations[0].stages['selfplay.metrics.json'].games_per_second, 2);
  assert.equal(job.iterations[0].stages['replay_update.json'].retained, 200);
  assert.deepEqual(job.iterations[2].stages, {});
  assert.equal(job.log, 'durable console\n');
  assert.ok(job.command.includes('resume'));
  assert.ok(Math.abs(Date.parse(job.startedAt) - Date.now()) < 10000);
  assert.equal(job.config.training.learning_rate, 0.01);
  await writeFile(join(runDir, 'active-config.json'), JSON.stringify({ run_seed: 7, training: { learning_rate: 0.0002 } }));
  const resumed = (await request('/api/status')).data.jobs.find(job => job.pid === child.pid);
  assert.equal(resumed.config.training.learning_rate, 0.0002, 'a resumed run must expose its accepted active config');
});

test('launches detached native scratch/warm-start commands with persisted config and output', async t => {
  const { root, request } = await fixture(t);
  for (const warmStart of [undefined, join(root, 'model ; literal')]) {
    const runDir = join(root, 'runs/min', warmStart ? 'warm' : 'scratch');
    const config = { run_seed: 19, arbitrary_native_field: { value: true } };
    const result = await request('/api/runs', { runDir, config, warmStart });
    assert.equal(result.status, 201);
    const { pid } = result.data;
    t.after(() => { try { process.kill(pid, 'SIGKILL'); } catch {} });
    const args = JSON.parse(await readFile(join(runDir, 'invocation.json'), 'utf8'));
    assert.deepEqual(args, ['train', '--run-dir', runDir, '--config', join(runDir, 'panel-config.json'), ...(warmStart ? ['--warm-start', warmStart] : ['--scratch'])]);
    assert.deepEqual(JSON.parse(await readFile(join(runDir, 'panel-config.json'), 'utf8')), config);
    assert.match(await readFile(join(runDir, 'console.log'), 'utf8'), /native output\nnative stderr/);
    const stopped = await request(`/api/runs/${pid}/stop`, {});
    assert.deepEqual(stopped, { status: 200, data: { stopped: true } });
  }
});

test('stop immediately SIGKILLs a selected shared-group process without killing peers', async t => {
  const { root, binary, runDir, request, children } = await fixture(t);
  const child = spawn(binary, ['train', '--run-dir', runDir], { cwd: root, stdio: 'ignore' });
  const peer = spawn(process.execPath, ['-e', 'setInterval(()=>{},1000)'], { stdio: 'ignore' });
  children.push(child, peer);
  await Promise.all([once(child, 'spawn'), once(peer, 'spawn')]);
  // Allow the fixture to install its SIGTERM handler; Stop still must use SIGKILL.
  await delay(100);
  const exited = once(child, 'exit');
  const start = performance.now();
  const result = await request(`/api/runs/${child.pid}/stop`, {});
  const [, signal] = await exited;
  assert.deepEqual(result, { status: 200, data: { stopped: true } });
  assert.equal(signal, 'SIGKILL');
  assert.ok(performance.now() - start < 1000);
  assert.equal(peer.exitCode, null);
  assert.equal(process.kill(peer.pid, 0), true);
  assert.equal((await request(`/api/runs/${peer.pid}/stop`, {})).status, 404);
});

test('serves presets and static files, refuses replacement runs and reports native startup errors', async t => {
  const { root, runDir, request, url } = await fixture(t);
  await mkdir(join(root, 'configs/alphazero'), { recursive: true });
  await writeFile(join(root, 'configs/alphazero/min.json'), '{"run_seed":7}');
  const configs = await request('/api/configs');
  assert.deepEqual(configs.data.configs, [{ name: 'min.json', path: join(root, 'configs/alphazero/min.json'), config: { run_seed: 7 } }]);
  assert.equal((await request('/api/runs', { runDir, config: {} })).status, 409);
  const rejected = await request('/api/runs', { runDir: join(root, 'runs/min/invalid'), config: { invalid: true } });
  assert.equal(rejected.status, 400);
  assert.match(rejected.data.error, /native config rejected/);
  assert.equal((await request('/api/runs', { runDir: 'x', config: null })).status, 400);
  assert.equal((await request('/api/unknown')).status, 404);
  await mkdir(join(root, 'training-panel/dist/assets'), { recursive: true });
  await writeFile(join(root, 'training-panel/dist/index.html'), '<html>panel</html>');
  await writeFile(join(root, 'training-panel/dist/assets/app.js'), 'console.log("panel")');
  const index = await fetch(url);
  assert.match(index.headers.get('content-type'), /text\/html/);
  assert.equal(await index.text(), '<html>panel</html>');
  const script = await fetch(url + '/assets/app.js');
  assert.match(script.headers.get('content-type'), /javascript/);
  assert.equal(await script.text(), 'console.log("panel")');
  assert.equal((await fetch(url + '/%2e%2e%2fserver.mjs')).status, 404);
});

test('stops the selected independent process group including its worker', async t => {
  const { root, binary, runDir, request, children } = await fixture(t);
  const leader = spawn(binary, ['resume', '--run-dir', runDir, '--child'], { cwd: root, detached: true, stdio: 'ignore' });
  children.push(leader);
  await once(leader, 'spawn');
  let worker;
  for (let attempt = 0; attempt < 100; attempt++) {
    try { worker = Number(await readFile(join(runDir, 'child.pid'), 'utf8')); break; }
    catch (error) { if (error.code !== 'ENOENT') throw error; await delay(10); }
  }
  assert.ok(worker, 'fixture worker must have started');
  t.after(() => { try { process.kill(worker, 'SIGKILL'); } catch {} });
  const exited = once(leader, 'exit');
  assert.equal((await request(`/api/runs/${leader.pid}/stop`, {})).status, 200);
  assert.equal((await exited)[1], 'SIGKILL');
  let alive = true;
  for (let attempt = 0; attempt < 100 && alive; attempt++) {
    try {
      const stat = await readFile(`/proc/${worker}/stat`, 'utf8');
      alive = stat.slice(stat.lastIndexOf(')') + 2)[0] !== 'Z';
    } catch (error) { if (error.code !== 'ENOENT') throw error; alive = false; }
    if (alive) await delay(10);
  }
  assert.equal(alive, false, 'a group leader stop must kill its worker too');
});

test('preserves Unicode configuration across request chunks and returns actionable HTTP errors', async t => {
  const { root, request, url } = await fixture(t);
  const body = Buffer.from(JSON.stringify({ runDir: join(root, 'runs/min/unicode'), config: { note: '학습' } }));
  const split = body.indexOf(Buffer.from('학')) + 1;
  const result = new Promise((resolve, reject) => {
    const req = httpRequest(url + '/api/runs', { method: 'POST', headers: { 'Content-Type': 'application/json' } }, async response => {
      let content = '';
      for await (const chunk of response) content += chunk;
      resolve({ status: response.statusCode, data: JSON.parse(content) });
    });
    req.on('error', reject);
    req.write(body.subarray(0, split));
    setTimeout(() => req.end(body.subarray(split)), 30);
  });
  const created = await result;
  assert.equal(created.status, 201);
  t.after(() => { try { process.kill(created.data.pid, 'SIGKILL'); } catch {} });
  const config = JSON.parse(await readFile(join(created.data.runDir, 'panel-config.json'), 'utf8'));
  assert.equal(config.note, '학습');
  await request(`/api/runs/${created.data.pid}/stop`, {});
  const malformed = await fetch(url + '/api/runs', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{' });
  assert.equal(malformed.status, 400);
  assert.match((await malformed.json()).error, /JSON/);
  const crossOrigin = await fetch(url + '/api/runs/1/stop', { method: 'POST', headers: { Origin: 'https://example.com' } });
  assert.equal(crossOrigin.status, 403);
});
});
