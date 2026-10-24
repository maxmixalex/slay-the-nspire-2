'use strict';
const assert=require('node:assert/strict');
const createProfileStorage=require('../platform/web/profile_storage.js');
function fixture(){
 const values=new Map();let fail=()=>false;
 const storage={
  getItem(key){if(fail('get',key))throw Error('Denied read');return values.has(key)?values.get(key):null;},
  setItem(key,value){if(fail('set',key))throw Error('Denied write');values.set(key,String(value));},
  removeItem(key){if(fail('remove',key))throw Error('Denied delete');values.delete(key);}
 };
 return {values,storage,fail(fn){fail=fn;},profile:createProfileStorage(()=>storage)};
}
{
 const f=fixture(),p=f.profile;
 assert.equal(p.selected(),1);assert.equal(p.key('nSpire2-save-v15',1),'nSpire2-save-v15');
 assert.equal(p.key('nSpire2-progress-v1',3),'nSpire2-progress-v1-profile-3');
 assert.throws(()=>p.readRun(4));
 f.values.set('nSpire2-save-v3','legacy');
 assert.equal(p.readRun(1),'legacy');assert.equal(p.readRun(2),null);
 for(let slot=1;slot<=3;slot++){
  assert(p.writeRun(slot,'run'+slot));assert(p.writeProgress(slot,'history'+slot));
 }
 for(let slot=1;slot<=3;slot++){
  assert.equal(p.readRun(slot),'run'+slot);assert.equal(p.readProgress(slot),'history'+slot);
 }
 assert(p.select(3));assert.equal(p.selected(),3);
 f.values.set('nSpire2-preferences-v2','preferences');f.values.set('nSpire2-language','eng');
 assert(p.clearRun(1));assert.equal(p.readRun(1),null);assert.equal(p.readProgress(1),'history1');
 assert(p.deleteProfile(2));assert.equal(p.readRun(2),null);assert.equal(p.readProgress(2),null);
 assert.equal(p.readRun(3),'run3');assert.equal(p.readProgress(3),'history3');
 assert.equal(f.values.get('nSpire2-preferences-v2'),'preferences');assert.equal(f.values.get('nSpire2-language'),'eng');
 assert.equal(p.selected(),3);
}
{
 const f=fixture(),p=f.profile;p.writeRun(1,'original');p.writeProgress(1,'history');
 const before=new Map(f.values);
 f.fail((operation,key)=>operation==='set'&&key==='nSpire2-profile-delete-pending');
 assert.throws(()=>p.deleteProfile(1));assert.deepEqual(f.values,before);
 f.fail((operation,key)=>operation==='set'&&key==='nSpire2-selected-profile');
 assert.throws(()=>p.select(2));assert.equal(p.selected(),1);assert.deepEqual(f.values,before);
}
{
 const f=fixture(),p=f.profile;
 for(let version=3;version<=15;version++)f.values.set('nSpire2-save-v'+version,'run'+version);
 p.writeProgress(1,'history');p.writeRun(2,'other');const before=new Map(f.values);
 let failed=false;
 f.fail((operation,key)=>!failed&&operation==='remove'&&key==='nSpire2-save-v7'?(failed=true):false);
 assert.equal(p.deleteProfile(1),false);assert.deepEqual(f.values,before);
 assert.equal(p.readRun(1),'run15');assert.equal(p.readProgress(1),'history');assert.equal(p.readRun(2),'other');
}
{
 const f=fixture(),p=f.profile;p.writeRun(3,'run');p.writeProgress(3,'history');
 // Deletion has removed the run before storage starts denying every write.
 // Reads use the committed backup, including across a new page/host instance.
 let denied=false;
 f.fail((operation,key)=>{if(operation==='remove'&&key==='nSpire2-progress-v1-profile-3')denied=true;return denied&&operation!=='get';});
 assert.equal(p.deleteProfile(3),false);assert.equal(p.readRun(3),'run');assert.equal(p.readProgress(3),'history');
 assert(f.values.has('nSpire2-profile-delete-pending'));assert.equal(p.writeRun(3,'overwrite'),false);
 const next=createProfileStorage(()=>f.storage);assert.equal(next.recover(),false);assert.equal(next.readRun(3),'run');
 f.fail(()=>false);assert(next.recover());assert.equal(next.readRun(3),'run');assert.equal(next.readProgress(3),'history');
 assert.equal(f.values.has('nSpire2-profile-delete-pending'),false);
}
console.log('Web profile storage: slot isolation, legacy keys, failed selector writes and recoverable deletion passed');
