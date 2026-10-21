#!/usr/bin/env node
'use strict';
// Actual WASM render diagnostics on a named host, not browser or device FPS.
// Replay native fixtures using normal controller inputs without mutating assets.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const {performance} = require('node:perf_hooks');
const assert = require('node:assert/strict');

function distribution(values) {
  if (!values.length) return {count:0};
  const sorted=[...values].sort((a,b)=>a-b), sum=values.reduce((a,b)=>a+b,0);
  return {count:values.length,total_ms:sum,mean_ms:sum/values.length,
    median_ms:sorted[Math.floor(sorted.length*.5)],p95_ms:sorted[Math.min(sorted.length-1,Math.ceil(sorted.length*.95)-1)],max_ms:sorted.at(-1),
    over_50ms:values.filter(x=>x>50).length,over_100ms:values.filter(x=>x>100).length,over_250ms:values.filter(x=>x>250).length};
}
(async()=>{
 const [,,wasmPath,packPath,fixtureRoot,outPath,filter=''] = process.argv;
 if(!outPath)throw Error('Usage: profile_web_runtime.js WASM PACK FIXTURES OUTPUT_JSON [SCENARIO_SUBSTRING]');
 const wasm=fs.readFileSync(wasmPath),pack=fs.readFileSync(packPath);
 const cases=JSON.parse(fs.readFileSync(path.join(fixtureRoot,'scenarios.json'),'utf8')).cases.filter(x=>!filter||x.name.includes(filter));
 assert.ok(cases.length,'No matching scenarios');
 const report={schema:2,measurement:'Node WebAssembly execution, excluding browser canvas/compositor and device CPU',
  preparation_policy:'Finish bounded combat preparation before the next fixture input; report its work separately from visible animation windows. No wall-clock frame sleeps are performed.',
  node:process.version,cpu:os.cpus()[0]?.model,pack_bytes:pack.length,wasm_bytes:wasm.length,scenarios:[]};
 for(const scenario of cases){
  let memory,game,reads=0,readBytes=0,readMs=0;
  let storage=fs.readFileSync(path.join(fixtureRoot,scenario.name+'-fixture.bin'));
  const begin=performance.now();
  const {instance}=await WebAssembly.instantiate(wasm,{env:{
   resource_read:(off,p,n)=>{const start=performance.now();assert.ok(off>=0&&n>=0&&off<=pack.length&&n<=pack.length-off);new Uint8Array(memory.buffer,p,n).set(pack.subarray(off,off+n));reads++;readBytes+=n;readMs+=performance.now()-start;return 1;},
   save:(p,n)=>{storage=Buffer.from(new Uint8Array(memory.buffer,p,n));return 1;},
   load:(p,max)=>{assert.ok(storage.length<=max);new Uint8Array(memory.buffer,p,storage.length).set(storage);return storage.length;}
  }});
  game=instance.exports;memory=game.memory;
  const scene=()=>game.performance_scene?(['title','character','collection','map','combat','room','overlay','other','combat_preparation'][game.performance_scene()]||'unknown'):'unavailable_in_baseline';
  const instantiateMs=performance.now()-begin,start=performance.now();assert.equal(game.start(pack.length),1);
  const result={name:scenario.name,instantiate_ms:instantiateMs,startup_ms:performance.now()-start,startup_read_calls:reads,startup_read_bytes:readBytes,steps:[],preparations:[]};
  function finishPreparation(afterStep){
   if(scene()!=='combat_preparation')return;
   const row={after_step:afterStep,updates:0,read_calls:0,read_bytes:0,resource_copy_ms:0,
    progress_read_calls:0,progress_read_bytes:0,first_visible:null},updateTimes=[],progressTimes=[];
   while(scene()==='combat_preparation'){
    assert.ok(row.updates<8192,'Combat preparation did not finish');
    let calls=reads,bytes=readBytes,io=readMs,started=performance.now();game.advance(33);
    updateTimes.push(performance.now()-started);row.updates++;
    row.read_calls+=reads-calls;row.read_bytes+=readBytes-bytes;row.resource_copy_ms+=readMs-io;
    const preparing=scene()==='combat_preparation';
    calls=reads;bytes=readBytes;io=readMs;started=performance.now();game.draw_frame();
    const elapsed=performance.now()-started;
    if(preparing){progressTimes.push(elapsed);row.progress_read_calls+=reads-calls;row.progress_read_bytes+=readBytes-bytes;row.resource_copy_ms+=readMs-io;}
    else row.first_visible={scene:scene(),render_ms:elapsed,read_calls:reads-calls,read_bytes:readBytes-bytes,resource_copy_ms:readMs-io};
   }
   row.update=distribution(updateTimes);row.progress_render=distribution(progressTimes);
   row.scheduled_ms_at_33ms=row.updates*33;
   row.scheduled_ms_scope='Simulation update schedule only; this process does not sleep and this value is not device elapsed time.';
   result.preparations.push(row);
  }
  for(const [index,step] of scenario.steps.entries()){
   const calls=reads,bytes=readBytes,io=readMs,beforeScene=scene(),input=performance.now();
   if(typeof step==='number'){if(step<0)game.advance(99);else game.input(step);}
   else if(Object.hasOwn(step,'key'))game.input(step.key);
   else if(Object.hasOwn(step,'advance'))game.advance(step.advance);
   else if(Object.hasOwn(step,'scroll'))game.pointer_scroll(step.scroll);
   else if(Object.hasOwn(step,'move'))game.pointer_move(...step.move);
   else if(Object.hasOwn(step,'click')){game.pointer_move(...step.click);game.pointer_button(1);game.pointer_button(0);}
   else if(Object.hasOwn(step,'pointer')){game.pointer_move(step.pointer[0],step.pointer[1]);game.pointer_button(step.pointer[2]?1:0);}
   else throw Error('Unknown action');
   const inputMs=performance.now()-input,render=performance.now();game.draw_frame();
   result.steps.push({index,step,scene_before:beforeScene,scene_after:scene(),input_ms:inputMs,render_ms:performance.now()-render,read_calls:reads-calls,read_bytes:readBytes-bytes,resource_copy_ms:readMs-io});
   finishPreparation(index);
  }
  assert.notEqual(scene(),'combat_preparation','Preparation must be excluded from visible animation windows');
  const cold=[],warm=[],sceneBefore=scene(),sceneCounts={};let before=reads,byteStart=readBytes;
  // Two equal six-second animation windows show recurrent cache pressure.
  for(let frame=0;frame<360;frame++){const t=performance.now();game.advance(33);game.draw_frame();(frame<180?cold:warm).push(performance.now()-t);const current=scene();sceneCounts[current]=(sceneCounts[current]||0)+1;}
  result.sustained={scene_before:sceneBefore,scene_after:scene(),scene_counts:sceneCounts,first_180:distribution(cold),next_180:distribution(warm),read_calls:reads-before,read_bytes:readBytes-byteStart};
  result.input=distribution(result.steps.map(x=>x.input_ms));result.render=distribution(result.steps.map(x=>x.render_ms));
  report.scenarios.push(result);fs.writeFileSync(outPath,JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify({scenario:scenario.name,startup_ms:result.startup_ms,render:result.render,sustained:result.sustained}));
 }
})().catch(error=>{console.error(error);process.exitCode=1;});
