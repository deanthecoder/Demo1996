// Regression checks for direct-file startup and the visible cube animation.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { createServer } from 'node:http';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { runInNewContext } from 'node:vm';

const root = new URL('../', import.meta.url);
const bundle = {};
runInNewContext(await readFile(new URL('assets-data.js', root), 'utf8'), bundle);
for (const [name, encoded] of Object.entries(bundle.DEMO1996_DATA))
  assert.deepEqual(Buffer.from(encoded, 'base64'), await readFile(new URL(`assets/${name}`, root)), `${name}: bundled bytes should match the original runtime asset`);
const profile = await mkdtemp(join(tmpdir(), 'demo1996-check-'));
const delay = milliseconds => new Promise(resolve => setTimeout(resolve, milliseconds));
const server = createServer(async (request, response) => {
  try {
    const path = new URL(request.url, 'http://localhost').pathname;
    const file = new URL(`.${path === '/' ? '/index.html' : path}`, root);
    const content = await readFile(file);
    const type = file.pathname.endsWith('.js') ? 'text/javascript' :
      file.pathname.endsWith('.css') ? 'text/css' : 'text/html';
    response.writeHead(200, { 'Content-Type': type });
    response.end(content);
  } catch { response.writeHead(404); response.end(); }
});
await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
const chrome = spawn('/Applications/Google Chrome.app/Contents/MacOS/Google Chrome', [
  '--headless=new', '--disable-gpu', '--no-first-run', `--user-data-dir=${profile}`,
  '--remote-debugging-port=0', '--window-size=1280,900', 'about:blank',
], { stdio: 'ignore' });

let socket;
try {
  let port;
  for (let attempt = 0; attempt < 100; attempt++) {
    try { port = Number((await readFile(join(profile, 'DevToolsActivePort'), 'utf8')).split('\n')[0]); break; }
    catch { await delay(100); }
  }
  assert.ok(port, 'Chrome should start its debugging endpoint');
  const targets = await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
  socket = new WebSocket(targets.find(target => target.type === 'page').webSocketDebuggerUrl);
  await new Promise(resolve => socket.addEventListener('open', resolve, { once: true }));
  let id = 0;
  const pending = new Map(), exceptions = [];
  socket.addEventListener('message', event => {
    const message = JSON.parse(event.data);
    if (message.method === 'Runtime.exceptionThrown') exceptions.push(message.params.exceptionDetails.text);
    if (pending.has(message.id)) { pending.get(message.id)(message); pending.delete(message.id); }
  });
  const command = (method, params = {}) => new Promise((resolve, reject) => {
    const requestId = ++id;
    const timeout = setTimeout(() => { pending.delete(requestId); reject(new Error(`${method} timed out`)); }, 10000);
    pending.set(requestId, message => { clearTimeout(timeout); message.error ? reject(message.error) : resolve(message.result); });
    socket.send(JSON.stringify({ id: requestId, method, params }));
  });
  const evaluate = async expression => {
    const answer = await command('Runtime.evaluate', { expression, returnByValue: true });
    if (answer.exceptionDetails) throw new Error(JSON.stringify(answer.exceptionDetails));
    return answer.result.value;
  };
  await command('Page.enable');
  await command('Runtime.enable');
  for (const url of [new URL('index.html', root).href, `http://127.0.0.1:${server.address().port}/`]) {
    await command('Page.navigate', { url });
    let state;
    for (let attempt = 0; attempt < 50; attempt++) {
      state = await evaluate(`(() => {
        const c = document.querySelector('#screen');
        if (!c) return null;
        const data = c.getContext('2d').getImageData(0, 0, 320, 200).data;
        let nonblack = 0;
        for (let i = 0; i < data.length; i += 4) if (data[i] || data[i + 1] || data[i + 2]) nonblack++;
        return { status: document.querySelector('#status').textContent, nonblack, width: c.getBoundingClientRect().width };
      })()`);
      if (state?.nonblack) break;
      await delay(100);
    }
    assert.ok(state?.nonblack > 1000, `${url}: the title should render`);
    assert.ok(state.width <= 960, 'The display should respect its size cap');
    await evaluate(`document.querySelector('#skip').click(); document.querySelector('#skip').click(); document.querySelector('#skip').click()`);
    await delay(1500);
    await evaluate(`window.__beforeCube = document.querySelector('#screen').getContext('2d').getImageData(0, 0, 320, 200).data`);
    await delay(500);
    const changed = await evaluate(`(() => {
      const next = document.querySelector('#screen').getContext('2d').getImageData(0, 0, 320, 200).data;
      let changed = 0;
      for (let i = 0; i < next.length; i += 4)
        if (next[i] !== __beforeCube[i] || next[i + 1] !== __beforeCube[i + 1] || next[i + 2] !== __beforeCube[i + 2]) changed++;
      return changed;
    })()`);
    assert.ok(changed > 100, `Cube should rotate visibly in half a second; changed ${changed} pixels`);
    console.log(`${url.startsWith('file:') ? 'file' : 'HTTP'}: title rendered, width ${state.width}px, cube changed ${changed} pixels in 0.5s`);
    for (const name of ['Sine scroller', 'Plasma']) {
      await evaluate(`document.querySelector('#skip').click()`);
      assert.equal(await evaluate(`document.querySelector('#status').textContent`), name);
    }
  }
  assert.deepEqual(exceptions, [], 'No browser script exceptions');
  console.log('Chrome regression checks passed.');
} finally {
  socket?.close();
  chrome.kill();
  await new Promise(resolve => chrome.exitCode !== null ? resolve() : chrome.once('exit', resolve));
  server.close();
  await rm(profile, { recursive: true, force: true });
}
