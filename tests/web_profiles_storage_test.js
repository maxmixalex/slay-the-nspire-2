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
  assert(p.writePreferences(slot,516+slot));
 }
 for(let slot=1;slot<=3;slot++){
  assert.equal(p.readRun(slot),'run'+slot);assert.equal(p.readProgress(slot),'history'+slot);
  assert.equal(p.readPreferences(slot),516+slot);
 }
 assert(p.select(3));assert.equal(p.selected(),3);
 const preferences=f.values.get('nSpire2-preferences-v2');f.values.set('nSpire2-language','eng');
 assert(p.clearRun(1));assert.equal(p.readRun(1),null);assert.equal(p.readProgress(1),'history1');
 assert(p.deleteProfile(2));assert.equal(p.readRun(2),null);assert.equal(p.readProgress(2),null);
 assert.equal(p.readPreferences(2),516);assert.equal(p.readPreferences(1),517);assert.equal(p.readPreferences(3),519);
 assert(!f.values.has('nSpire2-preferences-v1-profile-2'));assert(!f.values.has('nSpire2-preferences-v2-profile-2'));
 assert.equal(p.readRun(3),'run3');assert.equal(p.readProgress(3),'history3');
 assert.equal(f.values.get('nSpire2-preferences-v2'),preferences);assert.equal(f.values.get('nSpire2-language'),'eng');
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
 p.writeProgress(1,'history');p.writeRun(2,'other');p.writePreferences(1,549);const before=new Map(f.values);
 let failed=false;
 f.fail((operation,key)=>!failed&&operation==='remove'&&key==='nSpire2-save-v7'?(failed=true):false);
 assert.equal(p.deleteProfile(1),false);assert.deepEqual(f.values,before);
 assert.equal(p.readRun(1),'run15');assert.equal(p.readProgress(1),'history');assert.equal(p.readRun(2),'other');
 assert.equal(p.readPreferences(1),549);
}
{
 const f=fixture(),p=f.profile;p.writeRun(3,'run');p.writeProgress(3,'history');p.writePreferences(3,1052);
 // Deletion has removed the run before storage starts denying every write.
 // Reads use the committed backup, including across a new page/host instance.
 let denied=false;
 f.fail((operation,key)=>{if(operation==='remove'&&key==='nSpire2-progress-v1-profile-3')denied=true;return denied&&operation!=='get';});
 assert.equal(p.deleteProfile(3),false);assert.equal(p.readRun(3),'run');assert.equal(p.readProgress(3),'history');
 assert(f.values.has('nSpire2-profile-delete-pending'));assert.equal(p.writeRun(3,'overwrite'),false);
 const next=createProfileStorage(()=>f.storage);assert.equal(next.recover(),false);assert.equal(next.readRun(3),'run');
 f.fail(()=>false);assert(next.recover());assert.equal(next.readRun(3),'run');assert.equal(next.readProgress(3),'history');
 assert.equal(next.readPreferences(3),1052);
 assert.equal(f.values.has('nSpire2-profile-delete-pending'),false);
}
{
 const f=fixture(),p=f.profile;
 f.values.set('nSpire2-preferences-v1',JSON.stringify({version:1,fastMode:true}));
 assert.equal(p.readPreferences(1),517);assert.equal(p.readPreferences(2),516);
 f.values.set('nSpire2-preferences-v2',JSON.stringify({version:2,flags:0xffffffff}));
 assert.equal(p.readPreferences(1),517);assert.throws(()=>p.writePreferences(1,0xffffffff));
 f.values.set('nSpire2-preferences-v2','broken');assert.equal(p.readPreferences(1),517);
 p.writeRun(2,'run');p.writeProgress(2,'history');p.writePreferences(2,549);const before=new Map(f.values);
 let failed=false;f.fail((op,key)=>!failed&&op==='remove'&&key==='nSpire2-preferences-v2-profile-2'?(failed=true):false);
 assert.equal(p.deleteProfile(2),false);assert.deepEqual(f.values,before);assert.equal(p.readPreferences(2),549);
}
{
 const f=fixture(),p=f.profile,record=new Uint8Array(108);record.set([78,83,80,70,3,4,2,0]);
 // The browser preserves the opaque portable record; C++ validates actual
 // command conflicts. This nondefault code must survive every host operation.
 record[8+2*2]=89;record[8+2*25+2*2]=80;
 assert(p.writePreferences(1,516,record));assert.deepEqual(p.readPreferencesRecord(1),record);
 const second=record.slice();second[12]=84;assert(p.writePreferences(2,525,second));second[5]=13;
 assert.deepEqual(p.readPreferencesRecord(2),second);assert.equal(p.readPreferencesRecord(3),null);
 assert(p.writePreferences(1,517));record[5]=5;assert.deepEqual(p.readPreferencesRecord(1),record); // Older flags API retains bindings.
 const before=new Map(f.values);let failed=false;
 f.fail((op,key)=>!failed&&op==='set'&&key==='nSpire2-preferences-v3'?(failed=true):false);
 assert.equal(p.writePreferences(1,520,second),false);assert.deepEqual(f.values,before);assert.deepEqual(p.readPreferencesRecord(1),record);
 f.fail(()=>false);assert(p.clearRun(1));assert.deepEqual(p.readPreferencesRecord(1),record);
 assert(p.deleteProfile(2));assert.equal(p.readPreferencesRecord(2),null);assert.deepEqual(p.readPreferencesRecord(1),record);
 const bad=record.slice();bad[4]=4;assert.throws(()=>p.writePreferences(3,516,bad));assert.equal(p.readPreferencesRecord(3),null);
 f.values.set('nSpire2-preferences-v3-profile-3','{"version":3,"bytes":[78,83,80,70]}');assert.equal(p.readPreferencesRecord(3),null);
}
console.log('Web profiles: isolated runs/history/two-bank preferences, legacy migration, failed writes and recoverable deletion passed');
