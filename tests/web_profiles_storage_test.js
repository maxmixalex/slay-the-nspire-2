'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
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
 assert(p.writePreferences(1,516,record));record[4]=5;assert.deepEqual(p.readPreferencesRecord(1),record);
 const second=record.slice();second[12]=84;assert(p.writePreferences(2,525,second));second[5]=13;
 assert.deepEqual(p.readPreferencesRecord(2),second);assert.equal(p.readPreferencesRecord(3),null);
 assert(p.writePreferences(1,517));record[5]=5;assert.deepEqual(p.readPreferencesRecord(1),record); // Older flags API retains bindings.
 const before=new Map(f.values);let failed=false;
 f.fail((op,key)=>!failed&&op==='set'&&key==='nSpire2-preferences-v3'?(failed=true):false);
 assert.equal(p.writePreferences(1,520,second),false);assert.deepEqual(f.values,before);assert.deepEqual(p.readPreferencesRecord(1),record);
 f.fail(()=>false);assert(p.clearRun(1));assert.deepEqual(p.readPreferencesRecord(1),record);
 assert(p.deleteProfile(2));assert.equal(p.readPreferencesRecord(2),null);assert.deepEqual(p.readPreferencesRecord(1),record);
 const bad=record.slice();bad[4]=6;assert.throws(()=>p.writePreferences(3,516,bad));assert.equal(p.readPreferencesRecord(3),null);
 f.values.set('nSpire2-preferences-v3-profile-3','{"version":3,"bytes":[78,83,80,70]}');assert.equal(p.readPreferencesRecord(3),null);
}
{
 const f=fixture(),p=f.profile,record=new Uint8Array(108);record.set([78,83,80,70,4,68,2,0]);record[12]=89;
 assert(p.writePreferences(2,580,record));record[4]=5;assert.equal(p.readPreferences(2),580);assert.deepEqual(p.readPreferencesRecord(2),record);
 assert.equal(p.readPreferences(1),516);assert.equal(JSON.parse(f.values.get('nSpire2-preferences-v2-profile-2')).flags,516);
 const old=JSON.parse(f.values.get('nSpire2-preferences-v3-profile-2')).bytes;
 assert.equal(old[4],3);assert.equal(old[5],4);assert.equal(old[12],89);
 const before=new Map(f.values);let failed=false;
 f.fail((op,key)=>!failed&&op==='set'&&key==='nSpire2-preferences-v4-profile-2'?(failed=true):false);
 assert.equal(p.writePreferences(2,516,record),false);assert.deepEqual(f.values,before);assert.equal(p.readPreferences(2),580);
 f.fail(()=>false);assert(p.deleteProfile(2));assert.equal(p.readPreferences(2),516);assert.equal(p.readPreferencesRecord(2),null);
 assert(!f.values.has('nSpire2-preferences-v4-profile-2'));
 assert(!f.values.has('nSpire2-preferences-v5-profile-2'));
}
{
 // Long-press confirmations occupy only v5's formerly reserved bit7.
 // Earlier records retain their exact meaning and compatible key maps.
 const f=fixture(),p=f.profile,record=new Uint8Array(108);record.set([78,83,80,70,5,196,2,0]);record[12]=89;
 assert(p.writePreferences(2,708,record));assert.equal(p.readPreferences(2),708);assert.deepEqual(p.readPreferencesRecord(2),record);
 assert.equal(p.readPreferences(1),516);assert.equal(p.readPreferences(3),516);
 const four=JSON.parse(f.values.get('nSpire2-preferences-v4-profile-2'));
 assert.equal(four.version,4);assert.equal(four.flags,580);assert.equal(four.bytes[4],4);assert.equal(four.bytes[5],68);assert.equal(four.bytes[12],89);
 const three=JSON.parse(f.values.get('nSpire2-preferences-v3-profile-2'));
 assert.equal(three.bytes[4],3);assert.equal(three.bytes[5],4);assert.equal(three.bytes[12],89);
 assert.equal(JSON.parse(f.values.get('nSpire2-preferences-v2-profile-2')).flags,516);
 for(const version of [3,4]){
  const bad=record.slice();bad[4]=version;
  assert.throws(()=>p.writePreferences(1,644,bad));assert.equal(p.readPreferencesRecord(1),null);
  f.values.set('nSpire2-preferences-v'+version,JSON.stringify({version,bytes:Array.from(bad)}));
  assert.equal(p.readPreferencesRecord(1),null);assert.equal(p.readPreferences(1),516);
 }
 const before=new Map(f.values);let failed=false;
 f.fail((op,key)=>!failed&&op==='set'&&key==='nSpire2-preferences-v5-profile-2'?(failed=true):false);
 assert.equal(p.writePreferences(2,516),false);assert.deepEqual(f.values,before);assert.equal(p.readPreferences(2),708);
 let denied=false;f.fail((op,key)=>{if(op==='remove'&&key==='nSpire2-preferences-v5-profile-2')denied=true;return denied&&op!=='get';});
 assert.equal(p.deleteProfile(2),false);assert.equal(p.readPreferences(2),708);
 const next=createProfileStorage(()=>f.storage);assert.equal(next.recover(),false);assert.deepEqual(next.readPreferencesRecord(2),record);
 f.fail(()=>false);assert(next.recover());assert.deepEqual(f.values,before);
 assert(next.clearRun(2));assert.deepEqual(next.readPreferencesRecord(2),record);
 assert(next.deleteProfile(2));assert.equal(next.readPreferences(2),516);assert.equal(next.readPreferencesRecord(2),null);
 for(let version=1;version<=5;++version)assert(!f.values.has('nSpire2-preferences-v'+version+'-profile-2'));
}
{
 const f=fixture(),p=f.profile;
 assert(p.writeRun(1,'historical-standard',15,15));
 assert(p.writeRun(1,'historical-custom',16,16));
 assert(!f.values.has('nSpire2-save-v15'));assert.equal(p.readRun(1,16),'historical-custom');
 assert.throws(()=>p.readRun(1,15));assert.equal(p.writeRun(1,'old-overwrite',15,15),false);assert.equal(p.clearRun(1,15),false);
 const before=new Map(f.values);let failed=false;
 f.fail((op,key)=>!failed&&op==='remove'&&key==='nSpire2-save-v16'?(failed=true):false);
 assert.equal(p.writeRun(1,'power-order-v17',17,17),false);assert.deepEqual(f.values,before);
 f.fail(()=>false);assert(p.writeRun(1,'power-order-v17',17,17));
 assert.equal(p.readRun(1,17),'power-order-v17');assert(!f.values.has('nSpire2-save-v16'));
 assert.throws(()=>p.readRun(1,16));assert.throws(()=>p.writeRun(1,'bad-codec',17,16));
 assert.equal(p.writeRun(1,'older-custom',16,16),false);assert.equal(p.clearRun(1,16),false);
 assert.equal(p.readRun(1,17),'power-order-v17');assert(p.deleteProfile(1));assert.equal(p.readRun(1),null);
}
for(const maximumVersion of [8,9]){
 // This host deliberately treats progress as opaque bytes. Its ordinary
 // test stresses storage length and transaction preservation; passing the
 // ftue_progress_regression --export directory repeats these checks with
 // valid, round-tripped NSH7/8/9 payloads emitted by the canonical C++ codec.
 const directory=process.argv[2],f=fixture(),p=f.profile;
 function progress(name,version,flags,length=40){
  let bytes;
  if(directory)bytes=fs.readFileSync(path.join(directory,name));
  else{
   bytes=Buffer.alloc(length);for(let i=8;i<length;i++)bytes[i]=(i*43+17)&255;
   bytes.write('NSH'+version,0,'ascii');bytes.writeUInt32LE(length,4);
   if(version>=8)bytes.writeUInt16LE(flags,length-2-(version===9?536:0));
  }
  assert.equal(bytes.toString('ascii',0,4),'NSH'+version);assert.equal(bytes.readUInt32LE(4),bytes.length);
  if(version>=8)assert.equal(bytes.readUInt16LE(bytes.length-2-(version===9?536:0)),flags);
  return bytes.toString('base64');
 }
 const old=progress('legacy-profile.nsh',7,0),declined=progress('declined-profile.nsh',8,0x4001);
 const maximumBytes=maximumVersion===9?295885:295349;
 const enabled=progress('enabled-profile.nsh',8,0x6000);
 const full=progress(maximumVersion===9?'full-profile-v9.nsh':'full-profile.nsh',maximumVersion,0x7fff,maximumBytes);
 assert.equal(Buffer.from(full,'base64').length,maximumBytes);
 const histories=[old,declined,full];
 for(let slot=1;slot<=3;slot++){
  assert(p.writeRun(slot,'canonical-v17-'+slot,17,17));assert(p.writeProgress(slot,histories[slot-1]));
 }
 for(let slot=1;slot<=3;slot++)assert.equal(p.readProgress(slot),histories[slot-1]);
 assert(p.select(2));assert.equal(p.selected(),2);assert.equal(p.readProgress(1),old);
 // Run-only deletion and every preference fallback must preserve the FTUE
 // preference/completions in the independent progress record byte-for-byte.
 assert(p.clearRun(2));assert.equal(p.readProgress(2),declined);
 f.values.set('nSpire2-preferences-v1-profile-2',JSON.stringify({version:1,fastMode:true}));
 f.values.set('nSpire2-preferences-v2-profile-2','broken');
 assert.equal(p.readPreferences(2),517);assert.equal(p.readProgress(2),declined);
 assert(p.writePreferences(2,580));assert.equal(p.readProgress(2),declined);
 let before=new Map(f.values),failed=false;
 f.fail((op,key)=>!failed&&op==='set'&&key==='nSpire2-preferences-v4-profile-2'?(failed=true):false);
 assert.equal(p.writePreferences(2,516),false);assert.deepEqual(f.values,before);
 f.fail((op,key)=>op==='set'&&key==='nSpire2-progress-v1-profile-2');
 assert.throws(()=>p.writeProgress(2,enabled));assert.equal(p.readProgress(2),declined);assert.deepEqual(f.values,before);
 f.fail(()=>false);assert(p.writeProgress(2,enabled));assert.equal(p.readProgress(2),enabled);
 // A denied deletion after the run has moved must retain even a maximum
 // NSH8/9 profile across a new host instance and restore it on recovery.
 before=new Map(f.values);let denied=false;
 f.fail((op,key)=>{if(op==='remove'&&key==='nSpire2-progress-v1-profile-3')denied=true;return denied&&op!=='get';});
 assert.equal(p.deleteProfile(3),false);assert.equal(p.readProgress(3),full);assert.equal(p.readRun(3),'canonical-v17-3');
 const next=createProfileStorage(()=>f.storage);assert.equal(next.recover(),false);assert.equal(next.readProgress(3),full);
 assert.equal(next.writeProgress(3,enabled),false);f.fail(()=>false);assert(next.recover());assert.deepEqual(f.values,before);
 assert(next.deleteProfile(3));assert.equal(next.readProgress(3),null);assert.equal(next.readRun(3),null);
 assert.equal(next.readProgress(1),old);assert.equal(next.readProgress(2),enabled);assert.equal(next.selected(),2);
 console.log(directory?`Web profiles: valid canonical legacy and maximum NSH${maximumVersion} export fixtures passed`:`Web profiles: maximum-size opaque NSH${maximumVersion} storage/rollback fixture passed`);
}
console.log('Web profiles: isolated runs/history/two-bank preferences, legacy migration, failed writes and recoverable deletion passed');
