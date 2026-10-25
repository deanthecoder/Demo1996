[![Follow @deanthecoder](https://img.shields.io/twitter/url/https/twitter.com/deanthecoder.svg?style=social&label=Follow%20%40deanthecoder)](https://x.com/deanthecoder) [![GitHub stars](https://img.shields.io/github/stars/deanthecoder/Demo1996?style=social&label=Star)](https://github.com/deanthecoder/Demo1996/stargazers)

# Demo1996

**[Run Demo1996](https://deanthecoder.github.io/Demo1996/)**

An MS-DOS graphics demo I wrote in C in **1996**, restored to run in a modern browser.

The original program presents three title cards, a cube sequence, a sine-wave scroller and a plasma effect. The cube grows from seven dots per edge; lines move in from the screen corners to form a square, followed by wireframe rotation and shrink, and a cube with the original skull texture. The browser edition keeps the original packed graphic data and 320 × 200 indexed-color rendering, while making the sequence easier to replay or skip.

| Control | Action |
| --- | --- |
| Space / Enter / tap screen | Skip to the next scene |
| Restart demo | Return to the first title card |
| Skip scene | Advance to the next part |

## Preservation

The original `DEMO.EXE`, source, Borland project, packed `DATA.000`–`DATA.005` graphics and PCX working art are in [`original/`](original/). The original code was saved on 5 May 1996; its graphics data and working artwork are dated April 1996. The shared graphics and 3D library source found alongside the project is preserved in [`original/shared/`](original/shared/).

`main.js` reads copies of the original packed data directly. It decodes the blocks using the on-disk layout written by the original `BlockSave` function, renders the title cards and the original scroller artwork, and recreates the cube and plasma effects in a 320 × 200 canvas. The original archive remains untouched.

The renderer now follows the source's cube stages, fixed-point Y/X/Z rotation, orthographic projection, and affine texture scanlines. The plasma follows the assembly loop's 160 × 100 samples doubled to 2 × 2 pixels, including its second-sample offset and final melt/scanline wipe.

This is a source-based recreation, with exact original image bytes, rather than a verified pixel-perfect conversion. Intro fades and timing remain adapted for the browser. A nominal 70 Hz clock supplies the animation steps; some original DOS loops run as fast as the machine allows, so their wall-clock timing depends on the original PC. The texture mapper's edge pixels and the transitions have not yet been compared frame by frame with `DEMO.EXE`.

The original program refers to a MOD song in `ref`, but no music file or playback code survived with the project. This restoration is therefore silent.

## Run locally

Open `index.html` directly in Chrome or Safari, or run `./run.sh` for a local server. The browser loads a generated JavaScript bundle of the original block bytes, so local files work without a server or special browser settings.

To rebuild that data bundle after changing a runtime asset, run `node tools/build-assets.mjs`. The preserved files under `original/` are independent of the runtime bundle.

## License

MIT. See [LICENSE](LICENSE).
