(() => {
const canvas = document.querySelector('#screen');
const context = canvas.getContext('2d', { alpha: false });
context.imageSmoothingEnabled = false;
const status = document.querySelector('#status');

const WIDTH = 320, HEIGHT = 200;
const image = context.createImageData(WIDTH, HEIGHT);
const pixels = new Uint8Array(WIDTH * HEIGHT);
let palette = new Uint8Array(768);
let assets = {};

function loadBlock(name) {
  const encoded = globalThis.DEMO1996_DATA?.[name];
  if (!encoded) throw new Error(`Could not load ${name}`);
  const data = Uint8Array.from(atob(encoded), character => character.charCodeAt(0));
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
function line(x0, y0, x1, y1, color) {
  x0 = Math.round(x0); y0 = Math.round(y0); x1 = Math.round(x1); y1 = Math.round(y1);
  let dx = Math.abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -Math.abs(y1 - y0), sy = y0 < y1 ? 1 : -1, err = dx + dy;
  while (true) {
    put(x0, y0, color);
    if (x0 === x1 && y0 === y1) break;
    const twice = 2 * err;
    if (twice >= dy) { err += dy; x0 += sx; }
    if (twice <= dx) { err += dx; y0 += sy; }
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
function rotate(point, rx, ry, rz, scale = 1) {
  let [x, y, z] = point;
  // ASM3D.C uses 14-bit products, Y/X/Z rotation, and orthographic translation.
  const multiply = (value, angle, sine = false) =>
    Math.floor(value * Math.trunc((sine ? Math.sin(angle) : Math.cos(angle)) * 16384) / 16384);
  [x, z] = [multiply(x, ry) + multiply(z, ry, true), multiply(x, -ry, true) + multiply(z, ry)];
  if (rx) [y, z] = [multiply(y, rx) - multiply(z, rx, true), multiply(y, rx, true) + multiply(z, rx)];
  if (rz) [x, y] = [multiply(x, rz) - multiply(y, rz, true), multiply(x, rz, true) + multiply(y, rz)];
  return [160 + Math.trunc(x * scale), 100 - Math.trunc(y * scale)];
}
const cubePoints = [[-50,50,-50],[50,50,-50],[50,-50,-50],[-50,-50,-50],[-50,50,50],[50,50,50],[50,-50,50],[-50,-50,50]];
const cubeEdges = [[0,1],[1,2],[2,3],[3,0],[4,5],[5,6],[6,7],[7,4],[0,4],[1,5],[2,6],[3,7]];
const cubeFaces = [[0,1,2,3],[1,5,6,2],[5,4,7,6],[4,0,3,7],[0,4,5,1],[2,6,7,3]];
const radians = degrees => (degrees % 360) * Math.PI / 180;
const VGA_HZ = 70;

function dotLine(a, b) {
  for (let dot = 0; dot < 7; dot++)
    put(Math.trunc(a[0] + (b[0] - a[0]) * dot / 6), Math.trunc(a[1] + (b[1] - a[1]) * dot / 6), 250);
}

function texturedFace(points) {
  // Translate GLIB.C's 7-bit edge tables and affine scanline mapper.
  const left = [], right = [];
  const leftUV = [[126 * 128,0,-127 * 128,0],[127 * 128,127 * 128,0,-127 * 128],
    [0,127 * 128,127 * 128,0],[0,0,0,127 * 128]];
  const rightUV = [[0,0,127 * 128,0],[127 * 128,0,0,127 * 128],
    [127 * 128,127 * 128,-127 * 128,0],[0,127 * 128,0,-127 * 128]];
  for (let edge = 0; edge < 4; edge++) {
    const a = points[edge], b = points[(edge + 1) % 4];
    const descending = b[1] < a[1], from = descending ? b : a, to = descending ? a : b;
    const table = descending ? left : right, uv = (descending ? leftUV : rightUV)[edge];
    const height = to[1] - from[1] + 1;
    let x = from[0] * 128, u = uv[0], v = uv[1];
    const dx = Math.trunc((to[0] - from[0]) * 128 / height), du = Math.trunc(uv[2] / height), dv = Math.trunc(uv[3] / height);
    for (let y = 0; y <= height; y++) {
      table[from[1] + y] = [(x & 65535) >> 7, (u & 65535) >> 7, (v & 65535) >> 7];
      x += dx; u += du; v += dv;
    }
  }
  const minY = Math.min(...points.map(point => point[1])), maxY = Math.max(...points.map(point => point[1]));
  if (maxY - minY < 5) return;
  const texture = assets['DATA.004'].pixels;
  for (let y = Math.max(0, minY); y <= Math.min(HEIGHT - 1, maxY); y++) {
    if (!left[y] || !right[y]) continue;
    const [x1, u1, v1] = left[y], [x2, u2, v2] = right[y];
    const width = Math.max(1, x2 - x1);
    let u = u1 * 128, v = v1 * 128;
    const du = Math.trunc((u2 - u1) * 128 / width), dv = Math.trunc((v2 - v1) * 128 / width);
    for (let x = x1; x < x1 + width; x++) {
      const address = ((u & 0xff00) + ((v & 65535) >> 7)) & 65535;
      put(x, y, texture[address] ?? 0);
      u += du; v += dv;
    }
  }
}

function drawCube(t) {
  const back = assets['DATA.003'];
  setPalette(back); copyBlock(back);
  const frame = Math.floor(t * VGA_HZ);
  let points, dots = false;
  if (frame < 720) {
    dots = true;
    const step = frame + 1;
    points = cubePoints.map(point => rotate(point, radians(step), radians(step), radians(step * 2), Math.min(frame / 500, 1)));
    status.textContent = '3D — seven dots per edge, zooming in';
  } else if (frame < 940) {
    const progress = Math.min((frame - 720) / 150, 1);
    const starts = [[0,0],[319,0],[319,0],[319,199],[319,199],[0,199],[0,199],[0,0]];
    const ends = [[110,150],[110,50],[110,150],[210,150],[210,150],[210,50],[110,50],[210,50]];
    const corners = [[110,50],[210,50],[210,150],[110,150]];
    for (let edge = 0; edge < 4; edge++) dotLine(corners[edge], corners[(edge + 1) % 4]);
    const moving = starts.map((start, i) => start.map((coordinate, axis) => Math.trunc(coordinate + (ends[i][axis] - coordinate) * progress)));
    for (let edge = 0; edge < 4; edge++) line(...moving[edge * 2], ...moving[edge * 2 + 1], 250);
    status.textContent = '3D — lines moving in from the screen corners';
    return;
  } else if (frame < 1600) {
    const step = frame - 940 + 1;
    const scale = step <= 360 ? 1 : (661 - step) / 300;
    points = cubePoints.map(point => rotate(point, radians(step), radians(step), radians(step * 2), scale));
    status.textContent = '3D — wireframe rotation and shrink';
  } else if (frame < 1670) {
    status.textContent = '3D — texture mapping next';
    return;
  } else {
    const step = frame - 1670 + 1;
    const scale = step <= 490 ? (step + 9) / 500 : 1;
    points = cubePoints.map(point => rotate(point, radians(step * 2), radians(step * 3), 0, scale));
    for (const face of cubeFaces) {
      const p = face.map(index => points[index]);
      const cross = (p[1][0] - p[0][0]) * (p[2][1] - p[1][1]) - (p[1][1] - p[0][1]) * (p[2][0] - p[1][0]);
      if (cross >= 0) texturedFace(p);
    }
    if (step > 720) palette = back.palette.map(value => Math.max(0, value - (step - 720)));
    status.textContent = '3D — original skull texture';
    return;
  }
  for (const [a, b] of cubeEdges) {
    if (dots) dotLine(points[a], points[b]);
    else line(...points[a], ...points[b], 250);
  }
}

function drawScroller(t) {
  const scroll = assets['DATA.005'];
  setPalette(scroll); clear(0);
  const sourceX = Math.floor(t * VGA_HZ) * 2 - WIDTH;
  for (let x = 0; x < WIDTH - 1; x++) {
    const sx = sourceX + x;
    if (sx < 0 || sx >= scroll.height) continue;
    const top = Math.trunc((190 - scroll.width) / 2 - 15 * Math.sin((sourceX / 2 + x) * Math.PI / 180));
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
const plasmaColors = plasmaPalette();
function drawPlasma(t) {
  palette = plasmaColors;
  const frame = Math.min(899, Math.floor(t * VGA_HZ));
  const offsetA = Math.trunc(48 + Math.cos(frame / 37) * 47 + 256 * Math.trunc(48 + 47 * Math.sin(frame / 31)));
  const difference = Math.trunc(48 + Math.sin(frame / 24) * 47 + 256 * Math.trunc(48 + 47 * Math.cos(frame / 19)) - offsetA);
  // LODSB advances SI before the second lookup. Each sample becomes a 2×2 block.
  for (let y = 0; y < HEIGHT / 2; y++) for (let x = 0; x < WIDTH / 2; x++) {
    const source = offsetA + x + y * 256;
    const color = (plasmaData[source] + plasmaData[source + difference + 1]) & 255;
    const target = y * 2 * WIDTH + x * 2;
    pixels[target] = pixels[target + 1] = pixels[target + WIDTH] = pixels[target + WIDTH + 1] = color;
  }
}

function drawMelt(t) {
  drawPlasma(899 / VGA_HZ);
  // WipeScreen copies forward into overlapping VGA rows, repeating row 1.
  const row = pixels.slice(WIDTH, WIDTH * 2);
  for (let y = 2; y < HEIGHT; y++) pixels.set(row, y * WIDTH);
  const count = Math.min(200, Math.floor(t * VGA_HZ));
  for (let step = 0; step < count; step++) {
    const y = step < 100 ? step * 2 : (step - 100) * 2 + 1;
    pixels.fill(0, y * WIDTH, (y + 1) * WIDTH);
  }
}

const scenes = [
  { name: 'Edis Enterprises', duration: 2.2, draw: (t) => drawIntro('DATA.000', t) },
  { name: 'Presents', duration: 2.2, draw: (t) => drawIntro('DATA.001', t) },
  { name: 'The Stack', duration: 3.4, draw: (t) => drawIntro('DATA.002', t) },
  { name: '3D cube sequence', duration: 2454 / VGA_HZ, draw: drawCube },
  { name: 'Sine scroller', duration: 956 / VGA_HZ, draw: drawScroller },
  { name: 'Plasma', duration: 900 / VGA_HZ, draw: drawPlasma },
  { name: 'Melt and alternating scanline wipe', duration: 200 / VGA_HZ, draw: drawMelt },
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

function init() {
  const names = ['DATA.000','DATA.001','DATA.002','DATA.003','DATA.004','DATA.005'];
  const blocks = names.map(loadBlock);
  assets = Object.fromEntries(blocks.map((block, index) => [names[index], block]));
  addEventListener('resize', fit);
  document.querySelector('#crt-aspect').addEventListener('change', fit);
  document.querySelector('#restart').addEventListener('click', restart);
  document.querySelector('#skip').addEventListener('click', nextScene);
  canvas.addEventListener('pointerup', () => { canvas.focus(); nextScene(); });
  addEventListener('keydown', event => { if (event.key === ' ' || event.key === 'Enter') { event.preventDefault(); nextScene(); } });
  fit(); restart(); requestAnimationFrame(frame);
}
try { init(); }
catch (error) { console.error(error); status.textContent = `Could not start: ${error.message}`; }
})();
