// Check source-derived geometry and compare plasma with its DOS instruction loop.
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { runInNewContext } from 'node:vm';

const root = new URL('../', import.meta.url);
const element = { addEventListener() {}, getBoundingClientRect: () => ({ top: 66 }), offsetHeight: 50, checked: true, clientWidth: 960, style: {} };
const canvas = { ...element, getContext: () => ({ createImageData: () => ({ data: new Uint8ClampedArray(320 * 200 * 4) }), putImageData() {} }) };
const sandbox = { document: { querySelector: name => name === '#screen' ? canvas : element },
  console, Uint8Array, Uint8ClampedArray, DataView, atob, innerHeight: 900,
  performance: { now: () => 1 }, requestAnimationFrame() {}, addEventListener() {} };
runInNewContext(await readFile(new URL('assets-data.js', root), 'utf8'), sandbox);
const source = await readFile(new URL('main.js', root), 'utf8');
runInNewContext(source.replace('try { init(); }',
  'globalThis.renderer = { drawCube, drawPlasma, drawMelt, rotate, pixels, plasmaData, assets: () => assets }; try { init(); }'), sandbox);
const renderer = sandbox.renderer;
assert.deepEqual(Array.from(renderer.rotate([-50,50,-50], 0, 0, 0)), [110,50], 'Projection must match ASM3D Translate, without perspective');
const drawCubeFrame = frame => { renderer.drawCube((frame + 0.01) / 70); return renderer.pixels.slice(); };
let result = drawCubeFrame(0);
assert.equal(result[100 * 320 + 160], 250, 'Dots start together at the center');
result = drawCubeFrame(795);
assert.equal(result[75 * 320 + 55], 250, 'The incoming line endpoint is halfway from (0,0) to (110,150)');
assert.match(element.textContent, /lines moving in/);
result = drawCubeFrame(900);
for (const [x,y] of [[110,50],[210,50],[110,150],[210,150],[110,100]])
  assert.equal(result[y * 320 + x], 250, `Incoming lines should form the original square at ${x},${y}`);
drawCubeFrame(1100);
assert.match(element.textContent, /wireframe/);
result = drawCubeFrame(2300);
assert.match(element.textContent, /skull texture/);
const background = renderer.assets()['DATA.003'].pixels;
assert.ok(result.reduce((count, pixel, index) => count + (pixel !== background[index]), 0) > 2000, 'Textured faces should cover a visible part of the cube');

for (const frame of [0,133,456,899]) {
  renderer.drawPlasma((frame + 0.01) / 70);
  const expected = new Uint8Array(320 * 200);
  const pl1 = Math.trunc(48 + Math.cos(frame / 37) * 47 + 256 * Math.trunc(48 + 47 * Math.sin(frame / 31)));
  const bx = Math.trunc(48 + Math.sin(frame / 24) * 47 + 256 * Math.trunc(48 + 47 * Math.cos(frame / 19)) - pl1);
  let si = pl1, di = 0;
  for (let row = 0; row < 100; row++) {
    for (let column = 0; column < 160; column++) {
      const first = renderer.plasmaData[si++]; // LODSB increments SI.
      const second = renderer.plasmaData[si + bx];
      assert.notEqual(first, undefined, 'First sample must stay inside the table');
      assert.notEqual(second, undefined, 'Second sample must stay inside the table');
      const al = (first + second) & 255;
      expected[di] = expected[di + 1] = expected[di + 320] = expected[di + 321] = al;
      di += 2;
    }
    si += 96;
    di += 320;
  }
  assert.deepEqual(Array.from(renderer.pixels), Array.from(expected), `Plasma frame ${frame} should match the assembly sampling loop`);
}
renderer.drawMelt(200 / 70);
assert.ok(renderer.pixels.every(pixel => pixel === 0), 'The final alternating scanline wipe should finish black');
console.log('Render checks passed: original cube stages, orthographic projection, skull texture, four DOS plasma frames, and final wipe.');
