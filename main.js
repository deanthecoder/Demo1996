const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', { alpha: false });
context.imageSmoothingEnabled = false;
const status = document.querySelector('#status');

const WIDTH = 320, HEIGHT = 200;
const image = context.createImageData(WIDTH, HEIGHT);
const pixels = new Uint8Array(WIDTH * HEIGHT);
let palette = new Uint8Array(768);
let assets = {};

async function loadBlock(name) {
  const response = await fetch(`assets/${name}`);
  if (!response.ok) throw new Error(`Could not load ${name}`);
  const data = new Uint8Array(await response.arrayBuffer());
  const view = new DataView(data.buffer, data.byteOffset, data.byteLength);
  const width = view.getUint16(0, true), height = view.getUint16(2, true);
  const size = width * height;
  if (data.length !== 9 + 768 + size) throw new Error(`${name} has an unexpected length`);
  return { width, height, palette: data.slice(9, 777), pixels: data.slice(777) };
}

function clear(value = 0) { pixels.fill(value); }
function copyBlock(block, x = 0, y = 0) {
  for (let row = 0; row < block.height; row++) {
    if (row + y < 0 || row + y >= HEIGHT) continue;
    const start = Math.max(0, -x), end = Math.min(block.width, WIDTH - x);
    if (end > start) pixels.set(block.pixels.subarray(row * block.width + start, row * block.width + end), (row + y) * WIDTH + x + start);
  }
}
function put(x, y, color) { if (x >= 0 && x < WIDTH && y >= 0 && y < HEIGHT) pixels[(y | 0) * WIDTH + (x | 0)] = color; }
function line(x0, y0, x1, y1, color, dotted = false) {
  x0 = Math.round(x0); y0 = Math.round(y0); x1 = Math.round(x1); y1 = Math.round(y1);
  let dx = Math.abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -Math.abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy, step = 0;
  while (true) {
    if (!dotted || step % 7 < 2) put(x0, y0, color);
    if (x0 === x1 && y0 === y1) break;
    const twice = 2 * err;
    if (twice >= dy) { err += dy; x0 += sx; }
    if (twice <= dx) { err += dx; y0 += sy; }
    step++;
  }
}
function present(alpha = 1) {
  const data = image.data;
  for (let i = 0; i < pixels.length; i++) {
    const paletteAt = pixels[i] * 3, out = i * 4;
    data[out] = Math.round(palette[paletteAt] * 255 / 63 * alpha);
    data[out + 1] = Math.round(palette[paletteAt + 1] * 255 / 63 * alpha);
    data[out + 2] = Math.round(palette[paletteAt + 2] * 255 / 63 * alpha);
    data[out + 3] = 255;
  }
  context.putImageData(image, 0, 0);
}

function setPalette(block) { palette = block.palette; }
function rotate(point, rx, ry, rz) {
  let [x, y, z] = point;
  let c = Math.cos(rx), s = Math.sin(rx); [y, z] = [y * c - z * s, y * s + z * c];
  c = Math.cos(ry); s = Math.sin(ry); [x, z] = [x * c + z * s, x * s - z * c];
  c = Math.cos(rz); s = Math.sin(rz); [x, y] = [x * c - y * s, x * s + y * c];
  const scale = 140 / (z + 270);
  return [160 + x * scale, 100 - y * scale];
}
const cubePoints = [[-50,50,-50],[50,50,-50],[50,-50,-50],[-50,-50,-50],[-50,50,50],[50,50,50],[50,-50,50],[-50,-50,50]];
const cubeEdges = [[0,1],[1,2],[2,3],[3,0],[4,5],[5,6],[6,7],[7,4],[0,4],[1,5],[2,6],[3,7]];

function drawCube(t) {
  const back = assets['DATA.003'];
  setPalette(back); copyBlock(back);
  const phase = Math.min(t / 14, 1);
  const rx = t * 0.036, ry = t * 0.049, rz = t * 0.021;
  const points = cubePoints.map(point => rotate(point.map(value => value * phase), rx, ry, rz));
  for (const [a, b] of cubeEdges) line(points[a][0], points[a][1], points[b][0], points[b][1], 250, t < 5.2);
}

function drawScroller(t) {
  const scroll = assets['DATA.005'];
  setPalette(scroll); clear(0);
  const sourceX = Math.floor(t * 144) - WIDTH;
  for (let x = 0; x < WIDTH; x++) {
    const sx = sourceX + x;
    if (sx < 0 || sx >= scroll.height) continue;
    const top = Math.round((HEIGHT - scroll.width) / 2 - 15 * Math.sin((sourceX / 2 + x) * Math.PI / 180));
    for (let y = 0; y < scroll.width; y++) put(x, top + y, scroll.pixels[sx * scroll.width + y]);
  }
}

function plasmaPalette() {
  const result = new Uint8Array(768);
  let red = 0, green = 0;
  for (let x = 0; x < 64; x++) result.set([red++, green, 0], x * 3);
  for (let x = 64; x < 128; x++) result.set([--red, green, 0], x * 3);
  for (let x = 128; x < 192; x++) result.set([red++, green++, 0], x * 3);
  for (let x = 192; x < 256; x++) result.set([--red, --green, 0], x * 3);
  return result;
}
const plasmaData = new Uint8Array(256 * 256);
for (let x = 0; x < 256; x++) for (let y = 0; y < 256; y++)
  plasmaData[x + 256 * y] = 83 * (3 + Math.cos(0.09 * Math.hypot(x - 128, y - 128)) + Math.cos(x / 21) + Math.cos(y / 26));
function drawPlasma(t) {
  palette = plasmaPalette();
  const frame = t * 60;
  const offsetA = 48 + Math.cos(frame / 37) * 47 + 256 * Math.floor(48 + 47 * Math.sin(frame / 31));
  const offsetB = 48 + Math.sin(frame / 24) * 47 + 256 * Math.floor(48 + 47 * Math.cos(frame / 19)) - offsetA;
  for (let y = 0; y < HEIGHT; y++) for (let x = 0; x < WIDTH; x++) {
    const source = ((offsetA + x + y * 256) & 0xffff);
    pixels[y * WIDTH + x] = (plasmaData[source] + plasmaData[(source + offsetB) & 0xffff]) & 255;
  }
}

const scenes = [
  { name: 'Edis Enterprises', duration: 2.2, draw: (t) => drawIntro('DATA.000', t) },
  { name: 'Presents', duration: 2.2, draw: (t) => drawIntro('DATA.001', t) },
  { name: 'The Stack', duration: 3.4, draw: (t) => drawIntro('DATA.002', t) },
  { name: '3D dot and wireframe cube', duration: 8, draw: drawCube },
  { name: 'Sine scroller', duration: 13, draw: drawScroller },
  { name: 'Plasma', duration: 8, draw: drawPlasma },
];
function drawIntro(name, t) {
  const block = assets[name]; setPalette(block); clear(0); copyBlock(block, (WIDTH - block.width) >> 1, (HEIGHT - block.height) >> 1);
}

let scene = 0, sceneStart = 0;
function drawScene(elapsed) {
  const current = scenes[scene];
  current.draw(elapsed);
  // The original fades are retained, but the title begins visibly rather than
  // leaving a black canvas while a browser is starting its animation clock.
  let fade = 1;
  if (scene < 3) fade = Math.min(1, 0.3 + elapsed / 0.3, (current.duration - elapsed) / 0.25);
  present(Math.max(0, fade));
}
function restart() { scene = 0; sceneStart = performance.now(); status.textContent = scenes[0].name; drawScene(0); }
function nextScene() { scene = (scene + 1) % scenes.length; sceneStart = performance.now(); status.textContent = scenes[scene].name; drawScene(0); }
function frame(now) {
  if (!sceneStart) sceneStart = now;
  let elapsed = (now - sceneStart) / 1000, current = scenes[scene];
  if (elapsed >= current.duration) { nextScene(); elapsed = 0; current = scenes[scene]; }
  drawScene(elapsed);
  requestAnimationFrame(frame);
}

function fit() {
  const wrap = document.querySelector('.screen-wrap'), toolbar = document.querySelector('.toolbar');
  const aspect = document.querySelector('#crt-aspect').checked ? 4 / 3 : 8 / 5;
  const height = Math.max(180, innerHeight - wrap.getBoundingClientRect().top - toolbar.offsetHeight - 24);
  const width = Math.floor(Math.min(960, wrap.clientWidth, height * aspect));
  canvas.style.width = `${width}px`; canvas.style.height = `${Math.floor(width / aspect)}px`;
}

async function init() {
  const names = ['DATA.000','DATA.001','DATA.002','DATA.003','DATA.004','DATA.005'];
  const blocks = await Promise.all(names.map(loadBlock));
  assets = Object.fromEntries(blocks.map((block, index) => [names[index], block]));
  addEventListener('resize', fit);
  document.querySelector('#crt-aspect').addEventListener('change', fit);
  document.querySelector('#restart').addEventListener('click', restart);
  document.querySelector('#skip').addEventListener('click', nextScene);
  canvas.addEventListener('pointerup', () => { canvas.focus(); nextScene(); });
  addEventListener('keydown', event => { if (event.key === ' ' || event.key === 'Enter') { event.preventDefault(); nextScene(); } });
  fit(); restart(); requestAnimationFrame(frame);
}
init().catch(error => { console.error(error); status.textContent = `Could not start: ${error.message}`; });
