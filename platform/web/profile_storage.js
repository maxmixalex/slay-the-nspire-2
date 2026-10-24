// Slot one retains the keys used by every previous offline player. Original
// PrefsSave is profile-specific; SettingsSave (including language) is global.
function createProfileStorage(access) {
 const selector='nSpire2-selected-profile',journalKey='nSpire2-profile-delete-pending';
 let pending=null;
 function valid(slot){if(!Number.isInteger(slot)||slot<1||slot>3)throw Error('Invalid save profile');return slot;}
 function key(base,slot){valid(slot);return base+(slot===1?'':'-profile-'+slot);}
 function keys(slot,history){
  const list=[];for(let version=3;version<=16;version++)list.push(key('nSpire2-save-v'+version,slot));
  if(history)list.push(key('nSpire2-progress-v1',slot),key('nSpire2-preferences-v1',slot),key('nSpire2-preferences-v2',slot));return list;
 }
 function readJournal(text){
  if(text.length>8*1024*1024)throw Error('Invalid profile transaction');
  const value=JSON.parse(text);valid(value.slot);
  if((value.version!==1&&value.version!==2)||!Array.isArray(value.entries)||value.entries.length>17)throw Error('Invalid profile transaction');
  const allowed=new Set(keys(value.slot,true)),seen=new Set();
  for(const entry of value.entries){
   if(!Array.isArray(entry)||entry.length!==2||!allowed.has(entry[0])||seen.has(entry[0])||
      (typeof entry[1]!=='string'&&!(value.version===2&&entry[1]===null)))throw Error('Invalid profile transaction');
   seen.add(entry[0]);
  }
  return value;
 }
 function restore(){
  try{const storage=access();for(const [name,value]of pending.entries){if(value===null)storage.removeItem(name);else storage.setItem(name,value);}
   storage.removeItem(journalKey);pending=null;return true;
  }catch(error){return false;}
 }
 function recover(){
  if(!pending){const text=access().getItem(journalKey);if(text)pending=readJournal(text);}
  return !pending||restore();
 }
 function read(name){
  // If storage became unavailable during rollback, the committed backup is
  // still readable in this session and recoverable on the next launch.
  if(pending)for(const [saved,value]of pending.entries)if(saved===name)return value;
  return access().getItem(name);
 }
 function json(text){try{return JSON.parse(text);}catch(error){return null;}}
 function validFlags(flags){return Number.isInteger(flags)&&flags>=0&&flags<=0xffffffff&&(flags&~0x73f)===0&&(flags>>>8)<=4;}
 function erase(slot,history){
  if(!recover())return false;
  const storage=access(),entries=[];
  for(const name of keys(slot,history)){const value=storage.getItem(name);if(value!==null)entries.push([name,value]);}
  if(!entries.length)return true;
  const transaction={version:1,slot,entries};
  // A failed quota/permission check happens before any original is removed.
  storage.setItem(journalKey,JSON.stringify(transaction));pending=transaction;
  try{for(const [name]of entries)storage.removeItem(name);storage.removeItem(journalKey);pending=null;return true;}
  catch(error){restore();return false;}
 }
 return {
  key,recover,
  selected(){const value=Number(access().getItem(selector));return Number.isInteger(value)&&value>=1&&value<=3?value:1;},
  select(slot){valid(slot);if(!recover())return false;access().setItem(selector,String(slot));return true;},
  readProgress(slot){return read(key('nSpire2-progress-v1',slot));},
  readPreferences(slot){
   const saved=json(read(key('nSpire2-preferences-v2',slot)));
   if(saved&&saved.version===2&&validFlags(saved.flags))return saved.flags;
   const old=json(read(key('nSpire2-preferences-v1',slot)));
   return 4|(2<<8)|(old&&old.version===1&&old.fastMode===true?1:0);
  },
  readRun(slot,maxVersion=16){for(let version=16;version>=3;version--){const value=read(key('nSpire2-save-v'+version,slot));
   if(value){if(version>maxVersion)throw Error('This run requires a newer player');return value;}}return null;},
  writeRun(slot,value,version=15,maxVersion=16){
   if((version!==15&&version!==16)||version>maxVersion)throw Error('Unsupported save format');
   if(!recover())return false;
   const storage=access(),target=key('nSpire2-save-v'+version,slot),entries=[];
   // An older WASM must not replace an unreadable Custom/Daily run with a new
   // Standard run. Updating the HTML does not upgrade its embedded codec.
   if(maxVersion<16&&storage.getItem(key('nSpire2-save-v16',slot)))return false;
   for(const name of keys(slot,false)){const old=storage.getItem(name);if(old!==null||name===target)entries.push([name,old]);}
   if(entries.length===1&&entries[0][1]===value)return true;
   const transaction={version:2,slot,entries};
   storage.setItem(journalKey,JSON.stringify(transaction));pending=transaction;
   try{storage.setItem(target,value);for(const [name]of entries)if(name!==target)storage.removeItem(name);
    storage.removeItem(journalKey);pending=null;return true;
   }catch(error){restore();return false;}
  },
  writeProgress(slot,value){if(!recover())return false;access().setItem(key('nSpire2-progress-v1',slot),value);return true;},
  writePreferences(slot,flags){
   if(!validFlags(flags))throw Error('Invalid preferences');if(!recover())return false;
   const storage=access();storage.setItem(key('nSpire2-preferences-v2',slot),JSON.stringify({version:2,flags}));
   storage.setItem(key('nSpire2-preferences-v1',slot),JSON.stringify({version:1,fastMode:(flags&1)!==0}));return true;
  },
  clearRun(slot,maxVersion=16){if(maxVersion<16&&read(key('nSpire2-save-v16',slot)))return false;return erase(slot,false);},
  deleteProfile(slot){return erase(slot,true);}
 };
}
if(typeof module!=='undefined'&&module.exports)module.exports=createProfileStorage;
