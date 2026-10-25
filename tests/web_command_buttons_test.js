'use strict';
const assert=require('node:assert/strict');
const bind=require('../platform/web/command_buttons.js');
class Button{
 constructor(code){this.dataset={key:String(code)};this.handlers=new Map();this.captured=null;this.disabled=false;}
 addEventListener(type,callback){if(!this.handlers.has(type))this.handlers.set(type,[]);this.handlers.get(type).push(callback);}
 getBoundingClientRect(){return {left:10,right:90,top:10,bottom:40};}
 setPointerCapture(id){this.captured=id;}
 hasPointerCapture(id){return this.captured===id;}
 releasePointerCapture(id){assert.equal(this.captured,id);this.captured=null;this.fire('lostpointercapture',{pointerId:id});}
 fire(type,values={}){const event={button:0,pointerId:1,clientX:20,clientY:20,detail:1,repeat:false,
  preventDefault(){this.prevented=true;},stopPropagation(){this.stopped=true;},...values};
  for(const callback of this.handlers.get(type)||[])callback(event);return event;}
}
(async()=>{
 const end=new Button(6),confirm=new Button(4),calls=[];
 const control=bind([end,confirm],{press:key=>calls.push(['down',key]),release:key=>calls.push(['up',key]),cancel:()=>calls.push(['cancel']),focus:()=>calls.push(['focus'])});
 const take=()=>calls.splice(0);
 end.fire('pointerdown');assert.deepEqual(take(),[['down',6]]);assert.equal(end.captured,1);
 end.fire('pointerup');assert.deepEqual(take(),[['up',6],['focus']]);assert.equal(end.captured,null);
 end.fire('click');assert.deepEqual(take(),[]); // No duplicate canonical input.
 end.fire('pointerdown');end.fire('pointermove',{clientX:100});end.fire('pointerup');end.fire('click');
 assert.deepEqual(take(),[['down',6],['cancel']]);
 end.fire('pointerdown');end.fire('pointercancel');end.fire('pointerup');assert.deepEqual(take(),[['down',6],['cancel']]);
 end.fire('pointerdown');end.fire('pointerup',{pointerId:2});assert.deepEqual(take(),[['down',6]]);control.cancel();assert.deepEqual(take(),[['cancel']]);
 end.fire('pointerdown');end.fire('pointerup',{button:2});assert.deepEqual(take(),[['down',6]]);control.cancel();assert.deepEqual(take(),[['cancel']]);
 end.fire('pointerdown');confirm.fire('pointerdown',{pointerId:2});confirm.fire('pointerup',{pointerId:2});
 assert.deepEqual(take(),[['down',6],['cancel'],['down',4],['up',4],['focus']]);
 const key=end.fire('keydown',{key:' '});assert(key.prevented&&key.stopped);end.fire('keydown',{key:' ',repeat:true});assert.deepEqual(take(),[['down',6]]);
 end.fire('keyup',{key:' '});end.fire('click',{detail:0});assert.deepEqual(take(),[['up',6],['focus']]);
 await new Promise(resolve=>setTimeout(resolve,1));
 end.fire('click',{detail:0});assert.deepEqual(take(),[['down',6],['up',6],['focus']]); // A zero-duration activation.
 end.fire('keydown',{key:'Enter'});end.fire('blur');end.fire('keyup',{key:'Enter'});assert.deepEqual(take(),[['down',6],['cancel']]);
 end.disabled=true;end.fire('pointerdown');end.fire('click',{detail:0});assert.deepEqual(take(),[]);end.disabled=false;
 end.fire('pointerdown');control.cancel();control.cancel();assert.deepEqual(take(),[['down',6],['cancel']]);
 console.log('Web command buttons: held pointer/keyboard lifecycle, cancelled owners, blur, disabled controls and duplicate-click suppression passed');
})().catch(error=>{console.error(error);process.exitCode=1;});
