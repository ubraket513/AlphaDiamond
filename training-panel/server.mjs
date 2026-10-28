import { spawn, execFileSync } from 'node:child_process';
import { createServer } from 'node:http';
import { open, readFile, readdir, readlink, mkdir, writeFile } from 'node:fs/promises';
import { basename, dirname, extname, join, resolve, sep } from 'node:path';
import { fileURLToPath, pathToFileURL } from 'node:url';

const repository = resolve(dirname(fileURLToPath(import.meta.url)), '..');
const clockTicks = Number(execFileSync('getconf', ['CLK_TCK'], { encoding: 'utf8' }).trim());
const httpError = (status, message) => Object.assign(new Error(message), { status });

async function optionalFile(path, json = false) {
  try {
    const content = await readFile(path, 'utf8');
    return json ? JSON.parse(content) : content;
  } catch (error) {
    if (error.code === 'ENOENT') return json ? null : '';
    throw error;
  }
}

async function optionalDirectory(path) {
  try { return await readdir(path); }
  catch (error) { if (error.code === 'ENOENT') return []; throw error; }
}

async function trainingProcess(pid, binary, bootTime) {
  try {
    const proc = `/proc/${pid}`;
    const argv = (await readFile(`${proc}/cmdline`, 'utf8')).split('\0').filter(Boolean);
    // Native binaries use argv[0]; executable scripts retain the interpreter first.
    const index = [0, 1].find(i => argv[i] &&
      (basename(argv[i]) === 'alphadiamond-train' || argv[i] === binary));
    if (index === undefined || !['train', 'resume'].includes(argv[index + 1])) return null;
    const args = argv.slice(index + 2);
    const runIndex = args.indexOf('--run-dir');
    if (runIndex < 0 || !args[runIndex + 1]) return null;
    const cwd = await readlink(`${proc}/cwd`);
    const stat = await readFile(`${proc}/stat`, 'utf8');
    const fields = stat.slice(stat.lastIndexOf(')') + 2).split(' ');
    if (fields[0] === 'Z') return null;
    return {
      pid: Number(pid), runDir: resolve(cwd, args[runIndex + 1]), command: argv,
      startedAt: new Date((bootTime + Number(fields[19]) / clockTicks) * 1000).toISOString(),
      pgid: Number(fields[2]), startTicks: fields[19],
    };
  } catch (error) {
    // Processes may exit during /proc enumeration, and other users may hide their proc files.
    if (['ENOENT', 'ESRCH', 'EACCES'].includes(error.code)) return null;
    throw error;
  }
}

async function discover(binary) {
  const stat = await readFile('/proc/stat', 'utf8');
  const bootTime = Number(stat.match(/^btime (\d+)$/m)[1]);
  const pids = (await readdir('/proc')).filter(name => /^\d+$/.test(name));
  return (await Promise.all(pids.map(pid => trainingProcess(pid, binary, bootTime))))
    .filter(Boolean).sort((a, b) => a.pid - b.pid);
}

async function nativeJob({ pgid, startTicks, ...job }) {
  const [state, config, log, numbers] = await Promise.all([
    optionalFile(join(job.runDir, 'state.json'), true),
    optionalFile(join(job.runDir, 'resolved-config.json'), true),
    optionalFile(join(job.runDir, 'console.log')),
    optionalDirectory(join(job.runDir, 'iterations')),
  ]);
  const iterations = await Promise.all(numbers.filter(name => /^\d+$/.test(name))
    .sort((a, b) => Number(a) - Number(b)).map(async number => {
      const directory = join(job.runDir, 'iterations', number);
      const files = (await optionalDirectory(directory)).filter(name => name.endsWith('.json')).sort();
      const entries = await Promise.all(files.map(async name => [name, await optionalFile(join(directory, name), true)]));
      return { iteration: Number(number), stages: Object.fromEntries(entries.filter(([, value]) => value !== null)) };
    }));
  const metrics = iterations.flatMap(({ iteration, stages }) =>
    (stages['train.json']?.training_metrics ?? []).map(metric => ({ ...metric, iteration })))
    .sort((a, b) => a.iteration - b.iteration || a.training_step - b.training_step);
  return { ...job, state, config, metrics, iterations, log };
}

async function readBody(request) {
  if (!request.headers['content-type']?.startsWith('application/json'))
    throw httpError(415, 'Use application/json');
  request.setEncoding('utf8');
  let body = '';
  for await (const chunk of request) {
    body += chunk;
    if (body.length > 1024 * 1024) throw httpError(413, 'Request is too large');
  }
  try { return JSON.parse(body); }
  catch { throw httpError(400, 'Invalid JSON'); }
}

async function launch(root, binary, body) {
  const { runDir: input, config, warmStart } = body ?? {};
  if (typeof input !== 'string' || !input.trim() || !config || typeof config !== 'object' || Array.isArray(config) ||
      (warmStart !== undefined && (typeof warmStart !== 'string' || !warmStart.trim())))
    throw httpError(400, 'Provide runDir, a config object, and an optional warmStart path');
  const runDir = resolve(root, input);
  await mkdir(dirname(runDir), { recursive: true });
  try { await mkdir(runDir); }
  catch (error) { if (error.code === 'EEXIST') throw httpError(409, 'Run directory already exists'); throw error; }
  const configPath = join(runDir, 'panel-config.json');
  await writeFile(configPath, JSON.stringify(config, null, 2) + '\n', { flag: 'wx' });
  const output = await open(join(runDir, 'console.log'), 'a');
  let child;
  try {
    child = spawn(binary, ['train', '--run-dir', runDir, '--config', configPath,
      ...(warmStart ? ['--warm-start', resolve(root, warmStart)] : ['--scratch'])], {
      cwd: root, detached: true, stdio: ['ignore', output.fd, output.fd],
    });
    // Observe one short startup window so native argument/config errors reach the form.
    // The CLI owns validation; later progress and output remain in its native files.
    await new Promise((accept, reject) => {
      const timer = setTimeout(accept, 500);
      child.once('error', error => { clearTimeout(timer); reject(httpError(500, `Cannot start trainer: ${error.message}`)); });
      child.once('exit', (code, signal) => {
        clearTimeout(timer);
        if (code === 0) accept();
        else reject(httpError(400, `Trainer exited during startup (${signal ?? code})`));
      });
    }).catch(async error => {
      const log = await optionalFile(join(runDir, 'console.log'));
      if (log.trim()) error.message += `: ${log.trim()}`;
      throw error;
    });
    child.unref();
    return { pid: child.pid, runDir };
  } finally { await output.close(); }
}

export function createPanelServer({ root = repository, binary = process.env.ALPHADIAMOND_TRAIN_BIN ?? join(root, 'build/native-training-release/native/alphadiamond-train') } = {}) {
  binary = resolve(root, binary);
  const dist = join(root, 'training-panel/dist');
  return createServer(async (request, response) => {
    const send = (status, data) => {
      response.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
      response.end(JSON.stringify(data));
    };
    try {
      const pathname = new URL(request.url, 'http://127.0.0.1').pathname;
      if (request.method === 'POST' && request.headers.origin &&
          request.headers.origin !== `http://${request.headers.host}`)
        throw httpError(403, 'Cross-origin requests are not allowed');
      if (request.method === 'GET' && pathname === '/api/status') {
        const processes = await discover(binary);
        send(200, { jobs: await Promise.all(processes.map(nativeJob)) });
      } else if (request.method === 'GET' && pathname === '/api/configs') {
        const directory = join(root, 'configs/alphazero');
        const names = (await readdir(directory)).filter(name => name.endsWith('.json')).sort();
        send(200, { configs: await Promise.all(names.map(async name => ({ name,
          path: join(directory, name), config: await optionalFile(join(directory, name), true) }))) });
      } else if (request.method === 'POST' && pathname === '/api/runs') {
        send(201, await launch(root, binary, await readBody(request)));
      } else if (request.method === 'POST' && /^\/api\/runs\/\d+\/stop$/.test(pathname)) {
        const pid = Number(pathname.split('/')[3]);
        const selected = (await discover(binary)).find(job => job.pid === pid);
        if (!selected) throw httpError(404, 'Active training process not found');
        // Re-read identity before signalling; a supervisor's shared group must survive.
        const current = await trainingProcess(pid, binary, 0);
        if (!current || current.startTicks !== selected.startTicks) throw httpError(404, 'Training process exited');
        try { process.kill(current.pgid === pid ? -pid : pid, 'SIGKILL'); }
        catch (error) { if (error.code === 'ESRCH') throw httpError(404, 'Training process exited'); throw error; }
        send(200, { stopped: true });
      } else if (request.method === 'GET' && !pathname.startsWith('/api/')) {
        const file = resolve(dist, '.' + decodeURIComponent(pathname === '/' ? '/index.html' : pathname));
        if (!file.startsWith(dist + sep)) throw httpError(404, 'Not found');
        let content;
        try { content = await readFile(file); }
        catch (error) { if (error.code === 'ENOENT' || error.code === 'EISDIR') throw httpError(404, 'Not found'); throw error; }
        const types = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css', '.svg': 'image/svg+xml', '.png': 'image/png' };
        response.writeHead(200, { 'Content-Type': types[extname(file)] ?? 'application/octet-stream' });
        response.end(content);
      } else throw httpError(404, 'Not found');
    } catch (error) { send(error.status ?? 500, { error: error.message }); }
  });
}

if (process.argv[1] && import.meta.url === pathToFileURL(resolve(process.argv[1])).href) {
  const port = Number(process.env.PORT ?? 5178);
  createPanelServer().listen(port, '127.0.0.1', () => console.log(`Training panel: http://127.0.0.1:${port}`));
}
