#!/usr/bin/env node
'use strict';
// Compare native fixtures/frames with the actual compiled WASM exports.
const fs = require('node:fs');
const path = require('node:path');
const assert = require('node:assert/strict');
(async () => {
  const [, , wasmPath, packPath, root] = process.argv;
  if (!root) throw Error('Usage: node tools/check_room_backends.js WASM PACK NATIVE_CAPTURE_DIRECTORY');
  const cases = JSON.parse(fs.readFileSync(path.join(root, 'scenarios.json'), 'utf8')).cases;
  const wasm = fs.readFileSync(wasmPath), pack = fs.readFileSync(packPath);
  let frames = 0, saves = 0;
  for (const scenario of cases) {
    let memory, game, saved = 0;
    let storage = fs.readFileSync(path.join(root, scenario.name + '-fixture.bin'));
    const { instance } = await WebAssembly.instantiate(wasm, { env: {
      save: (p, n) => {
        storage = Buffer.from(new Uint8Array(memory.buffer, p, n));
        assert.deepEqual(storage, fs.readFileSync(path.join(root, `${scenario.name}-save-${saved++}.bin`)));
        ++saves; return 1;
      },
      load: (p, max) => {
        assert.ok(storage.length <= max);
        new Uint8Array(memory.buffer, p, storage.length).set(storage);
        return storage.length;
      }
    }});
    game = instance.exports; memory = game.memory;
    new Uint8Array(memory.buffer, game.content_buffer(), pack.length).set(pack);
    assert.equal(game.start(pack.length), 1);
    for (const [i, key] of scenario.steps.entries()) {
      if (key < 0) game.advance(99); else game.input(key);
      game.draw_frame();
      const actual = Buffer.from(new Uint8Array(memory.buffer, game.framebuffer(), 320 * 240 * 2));
      assert.deepEqual(actual, fs.readFileSync(path.join(root, `${scenario.name}-frame-${i}.rgb`)), `${scenario.name} frame ${i}`);
      ++frames;
    }
    assert.equal(saved, scenario.saves);
    console.log(`${scenario.name}: ${scenario.steps.length} exact frames; ${saved} exact saves`);
  }
  console.log(`Total: ${frames} native/WASM RGB565 frame matches; ${saves} save matches. Storage adapter, not browser DOM or persistent localStorage.`);
})().catch(error => { console.error(error); process.exitCode = 1; });
