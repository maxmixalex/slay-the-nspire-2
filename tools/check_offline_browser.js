#!/usr/bin/env node
'use strict';

// Private, self-contained release acceptance through the actual browser DOM.
// Usage: node tools/check_offline_browser.js HTML OUTPUT PLAYWRIGHT_MODULE CHROMIUM
// The last two arguments select an installed Playwright module and executable.
// No user browser profile, external service or application test hook is used.
const fs=require('node:fs');
const path=require('node:path');
const assert=require('node:assert/strict');
const {pathToFileURL}=require('node:url');
const [html,output,modulePath,browserPath,debugHtml]=process.argv.slice(2);
if(!html||!output||!modulePath||!browserPath){
 console.error('usage: check_offline_browser.js HTML OUTPUT PLAYWRIGHT_MODULE CHROMIUM');
 process.exit(2);
}
const root=path.resolve(__dirname,'..'),directory=path.resolve(output);
if(directory===root||directory.startsWith(root+path.sep))throw Error('Keep private game captures outside the source repository');
fs.mkdirSync(directory,{recursive:true});
const {chromium}=require(modulePath);
const report={schema:1,html:path.resolve(html),started_at:new Date().toISOString(),status:'running',
 checks:[],captures:[],dialogs:[],errors:[],blocked_requests:[],scope:'Isolated desktop Chromium DOM and Canvas; not physical-calculator performance'};
let browser;
function record(name){report.checks.push(name);console.log(name);}
function writeReport(){fs.writeFileSync(path.join(directory,'browser-acceptance.json'),JSON.stringify(report,null,2)+'\n');}
writeReport();
(async()=>{
 browser=await chromium.launch({executablePath:path.resolve(browserPath),headless:true,args:['--disable-gpu','--disable-dev-shm-usage']});
 report.browser=browser.version();
 const context=await browser.newContext({viewport:{width:1000,height:1100},locale:'en-US',serviceWorkers:'block'});
 await context.route('**/*',route=>{
  const url=route.request().url();
  if(url.startsWith('file:'))return route.continue();
  report.blocked_requests.push(url);return route.abort();
 });
 const page=await context.newPage();
 page.on('pageerror',error=>report.errors.push(error.message));
 page.on('console',message=>{if(message.type()==='error')report.errors.push(message.text());});
 let acceptDialog=true;
 page.on('dialog',async dialog=>{
  report.dialogs.push({type:dialog.type(),message:dialog.message(),accepted:acceptDialog});
  if(acceptDialog)await dialog.accept();else await dialog.dismiss();
 });
 const canvas=page.locator('#screen');
 async function painted(){await page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(()=>resolve()))));}
 async function ready(language){
  await page.waitForFunction(()=>!document.querySelector('#measure')?.disabled||document.querySelector('#error')?.textContent,{},{timeout:60000});
  assert.equal(await page.locator('#error').innerText(),'');
  assert.equal(await page.locator('#language').count(),0);
  assert.equal(await page.locator('html').getAttribute('lang'),language==='eng'?'en':'zh-CN');
  assert.equal(await canvas.isVisible(),true);
  assert.equal(await page.locator('#loading').isVisible(),false);
  await painted();
 }
 async function capture(name){
  await painted();
  const url=await canvas.evaluate(node=>node.toDataURL('image/png'));
  fs.writeFileSync(path.join(directory,name+'.png'),Buffer.from(url.split(',')[1],'base64'));
  report.captures.push(name+'.png');
 }
 async function canvasPixels(x,y,width,height){
  await painted();return canvas.evaluate((node,box)=>Array.from(node.getContext('2d').getImageData(...box).data),[x,y,width,height]);
 }
 function pixelDifference(before,after){
  assert.equal(before.length,after.length);let difference=0;
  for(let i=0;i<before.length;++i)if(i%4!==3)difference+=Math.abs(before[i]-after[i]);
  return difference/(before.length*3/4);
 }
 function runPrefix(value){
  assert.ok(value);const data=Buffer.from(value,'base64');assert.equal(data.subarray(0,4).toString(),'NSV1');
  assert.equal(data.readUInt32LE(4),17);assert.equal(data.readUInt32LE(8),0x41cef1ea);
  // The original fixed v17 prefix: 13 scalar fields, two power-order
  // entries, two RNG fields, seed, revision, then the deck count.
  return {phase:data.readUInt32LE(12),turn:data.readUInt32LE(56),deck_count:data.readUInt32LE(88),bytes:data.length};
 }
 async function press(key){await canvas.focus();await page.keyboard.press(key);await painted();}
 async function confirm(){await page.locator('button[data-key="4"]').click();await painted();}
 const saved=(slot=1)=>page.evaluate(id=>localStorage.getItem('nSpire2-save-v17'+(id===1?'':'-profile-'+id)),slot);
 const preference=(slot=1)=>page.evaluate(id=>JSON.parse(localStorage.getItem('nSpire2-preferences-v5'+(id===1?'':'-profile-'+id))),slot);
 const progress=(slot=1)=>page.evaluate(id=>localStorage.getItem('nSpire2-progress-v1'+(id===1?'':'-profile-'+id)),slot);
 const tutorial={accept:1,combat:2,map:4,enabled:0x2000,initialized:0x4000,fresh:0x6000};
 async function tutorialRecord(slot){
  const value=await progress(slot);if(!value)return null;
  const data=Buffer.from(value,'base64');assert.match(data.subarray(0,4).toString(),/^NSH[89]$/);assert.equal(data.readUInt32LE(4),data.length);
  const metadata=data[3]===57?536:0;const flags=data.readUInt16LE(data.length-2-metadata);assert.equal(flags&0x8000,0);assert.ok(flags&tutorial.initialized);
  return {flags,next_serial:data.readUInt32LE(8),history_count:data[12],bytes:data.length};
 }
 async function waitTutorial(slot,mask,value,timeout=10000){
  await page.waitForFunction(({slot,mask,value})=>{
   const encoded=localStorage.getItem('nSpire2-progress-v1'+(slot===1?'':'-profile-'+slot));if(!encoded)return false;
   const data=atob(encoded);if(!/^NSH[89]$/.test(data.slice(0,4))||data.length<10)return false;
   const metadata=data[3]==='9'?536:0;if(data.length<10+metadata)return false;
   const at=data.length-2-metadata,flags=data.charCodeAt(at)|(data.charCodeAt(at+1)<<8);return (flags&mask)===value;
  },{slot,mask,value},{timeout});
  return tutorialRecord(slot);
 }
 async function point(x,y){const box=await canvas.boundingBox();assert(box);await page.mouse.click(box.x+x*box.width/320,box.y+y*box.height/240);await painted();}
 async function move(x,y){const box=await canvas.boundingBox();assert(box);await page.mouse.move(box.x+x*box.width/320,box.y+y*box.height/240);await painted();}
 async function titleRow(row,hasRun){const count=hasRun?5:4;await point(125,Math.round((699+(450-count*50)/2+row*50+25)*4/21));}
 async function openSettings(hasRun){await titleRow(hasRun?2:1,hasRun);}
 // Pinned Settings source:1014px control origin,320px width,64px row
 // height,211px first row and82px pitch. These are ordinary canvas clicks;
 // no application state or hit-region test API supplies the coordinates.
 const settingsProject=value=>Math.floor((value*4+10)/21);
 const settingsControlX=settingsProject(1014)+Math.floor(settingsProject(320)/2);
 const settingsRowCenter=row=>settingsProject(211+82*row)+Math.floor(settingsProject(64)/2);
 const settingsLanguageCenter=item=>Math.floor((settingsProject(275+44*item)+settingsProject(275+44*(item+1)))/2);
 async function toggleFast(){await point(settingsControlX,settingsRowCenter(2));}
 async function toggleTimer(){await point(settingsControlX,settingsRowCenter(4));}
 async function returnToTitle(){await press('Escape');await point(160,156);}
 async function saveAndResume(){await returnToTitle();await titleRow(0,true);}
 async function selectProfile(slot){await point(20,10);await point(72+(slot-1)*88,120);await page.waitForFunction(id=>localStorage.getItem('nSpire2-selected-profile')===String(id),slot);await painted();}
 async function declineTutorials(slot,name){
  assert.equal(await saved(slot),null);assert.equal((await tutorialRecord(slot)).flags,tutorial.fresh);
  await capture(name+'-ask');await point(127,155);
  assert.equal((await waitTutorial(slot,0xffff,tutorial.initialized|tutorial.accept)).flags,0x4001);
  assert.equal(await saved(slot),null); // The modal callback starts, but does not save, the run.
 }
 const begin=Date.now();
 await page.goto(pathToFileURL(path.resolve(html)).href,{waitUntil:'load',timeout:120000});
 await ready('eng');report.first_ready_wall_ms=Date.now()-begin;
 assert.equal(await saved(),null);record('fresh English title without a startup language chooser or external language control');
 assert.equal(await page.evaluate(()=>typeof globalThis.nSpire2Debug),'undefined');
 record('ordinary HTML does not expose the developer command API');
 await capture('browser-title-eng');
 await openSettings(false);await toggleFast();await toggleTimer();
 assert.equal((await preference()).version,5);assert.equal((await preference()).flags&65,65);
 await capture('browser-settings-persisted-eng');
 await page.reload({waitUntil:'load'});await ready('eng');
 await openSettings(false);await toggleFast();
 assert.equal((await preference()).flags&65,64);
 await toggleTimer();assert.equal((await preference()).flags&64,0);
 await toggleTimer();assert.equal((await preference()).flags&64,64);
 await press('Escape');
 record('in-game Fast Mode survives reload and the real toggle reads its restored value');
 record('Show Run Timer uses the real General settings checkbox and survives reload in the v5 profile preference record');
 await titleRow(0,false);await capture('browser-singleplayer-eng');await confirm();await capture('browser-character-eng');
 assert.equal((await tutorialRecord(1)).flags,tutorial.fresh);
 await confirm();await declineTutorials(1,'browser-profile1-tutorial');await saveAndResume();
 await page.waitForFunction(()=>!!localStorage.getItem('nSpire2-save-v17'));
 const firstSave=await saved(),bytes=Buffer.from(firstSave,'base64');
 assert.ok(bytes.length>40);report.saved_bytes=bytes.length;
 fs.writeFileSync(path.join(directory,'browser-test-save-v17.bin'),bytes);
 assert.equal(bytes.readUInt32LE(4),17);
 record('original Singleplayer to character flow starts a run; pause Save and Quit writes a v17 save and Continue resumes');
 record('the tutorial question appears on Embark; No persists only in this profile and continues the original run flow');
 await capture('browser-run-eng');
 await press('o');assert.equal(await saved(),firstSave);await capture('browser-loaded-eng');
 record('O load returns through the real keyboard handler without changing stored save bytes');
 await page.reload({waitUntil:'load'});await ready('eng');assert.equal(await saved(),firstSave);
 await capture('browser-continue-eng');await titleRow(0,true);await saveAndResume();assert.equal(await saved(),firstSave);
 record('reload offers Continue and resumed run saves the same canonical bytes');
 await returnToTitle();await openSettings(true);
 async function switchLanguage(language){
  const origin=await page.evaluate(()=>performance.timeOrigin);
  await point(settingsControlX,settingsRowCenter(0));await point(settingsControlX,settingsLanguageCenter(language==='eng'?0:1));
  await page.waitForFunction(lang=>document.documentElement.lang===(lang==='eng'?'en':'zh-CN')&&localStorage.getItem('nSpire2-language')===lang,language,{timeout:60000});
  await ready(language);assert.equal(await page.evaluate(()=>performance.timeOrigin),origin);
  assert.equal(await saved(),firstSave);
 }
 await switchLanguage('zhs');await capture('browser-settings-zhs');
 await press('Escape');await titleRow(0,true);await saveAndResume();assert.equal(await saved(),firstSave);await press('o');
 await capture('browser-run-zhs');record('language changes inside the title settings, without reload or changing the saved run');
 await page.reload({waitUntil:'load'});await ready('zhs');assert.equal(await saved(),firstSave);
 await capture('browser-reloaded-zhs');record('reload remembers Chinese and the same save');
 await openSettings(true);await switchLanguage('eng');await press('Escape');
 assert.deepEqual(report.dialogs,[]);record('original language dropdown changes directly without a browser confirmation dialog');
 await selectProfile(2);assert.equal(await saved(2),null);assert.equal(await saved(),firstSave);
 await capture('browser-profile2-title');await openSettings(false);await toggleFast();
 assert.equal((await preference(2)).flags&65,1);assert.equal((await preference()).flags&65,64);
 await press('Escape');await titleRow(0,false);await confirm();await press('ArrowRight');await confirm();
 await declineTutorials(2,'browser-profile2-tutorial');await saveAndResume();
 const secondSave=await saved(2);assert(secondSave&&secondSave!==firstSave);assert.equal(await saved(),firstSave);
 await capture('browser-profile2-run');await returnToTitle();
 await selectProfile(1);await openSettings(true);await toggleFast();assert.equal((await preference()).flags&1,1);
 await toggleFast();assert.equal((await preference()).flags&1,0);await press('Escape');
 // The original profile picker is a real modal. Cancel leaves the other
 // profile's run/history/preferences intact; active deletion resets only it.
 await selectProfile(2);await page.reload({waitUntil:'load'});await ready('eng');
 assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-selected-profile')),'2');
 await openSettings(true);await toggleFast();assert.equal((await preference(2)).flags&1,0);
 await toggleFast();assert.equal((await preference(2)).flags&1,1);await press('Escape');
 await point(20,10);await capture('browser-profiles');await point(72,199);await capture('browser-profile-delete-confirm');await point(118,155);
 assert.equal(await saved(),firstSave);assert.equal(await saved(2),secondSave);
 await point(160,199);await point(192,155);
 assert.equal(await saved(2),null);assert.equal(await preference(2),null);assert.equal(await saved(),firstSave);
 // An empty active profile may immediately persist the new source-default
 // tutorial preference. That fresh NSH8 record must contain no old run or
 // history identity; deleting it again is not part of the host contract.
 const deletedProgress=await tutorialRecord(2);
 if(deletedProgress){assert.equal(deletedProgress.flags,tutorial.fresh);assert.equal(deletedProgress.next_serial,1);assert.equal(deletedProgress.history_count,0);}
 assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-language')),'eng');
 await capture('browser-profile2-deleted');await point(72,120);
 record('three-profile controls preserve legacy slot one, isolate runs and preferences, persist selection, cancel deletion and reset an active profile');
 // A separate empty profile exercises the enabled path through only real
 // controls. No run seed, modal state, or completion bit is injected.
 await selectProfile(3);assert.equal(await saved(3),null);
 await titleRow(0,false);await confirm();await point(116,214); // Original Ironclad portrait.
 assert.equal((await tutorialRecord(3)).flags,tutorial.fresh);
 await confirm();await capture('browser-profile3-tutorial-ask');await point(192,155);
 await waitTutorial(3,tutorial.fresh|tutorial.accept,tutorial.fresh|tutorial.accept);
 assert.equal(await saved(3),null);assert.equal((await tutorialRecord(1)).flags,0x4001);
 // Neow's first randomly offered relic can open an actual card/bundle or
 // reward choice. Select and the visible confirmation control resolve those
 // bounded prompts; a map completion bit is observed only after the real
 // MapSelect modal has opened. The limit makes a stuck/incorrect flow fail.
 let neowSteps=0;
 for(;neowSteps<30&&!((await tutorialRecord(3)).flags&tutorial.map);++neowSteps){
  await confirm();await page.waitForTimeout(900);
  if((await tutorialRecord(3)).flags&tutorial.map)break;
  await page.locator('button[data-key="6"]').click();await painted();await page.waitForTimeout(250);
 }
 assert.ok((await tutorialRecord(3)).flags&tutorial.map,'real Neow choices must reach the first map tutorial');
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);
 await capture('browser-profile3-map-tutorial');await confirm();
 await press('ArrowRight');await confirm();await page.waitForTimeout(5500);
 await capture('browser-profile3-combat-tutorial-page1');
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);
 // Other shortcuts and the visible End turn button are blocked by the
 // lesson. The saved first-turn value below verifies no turn slipped through.
 for(const key of ['Escape','m','p','o'])await press(key);
 await page.locator('button[data-key="6"]').click();await painted();
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);assert.equal(await saved(3),null);
 await press('ArrowRight');await page.waitForTimeout(650);await capture('browser-profile3-combat-tutorial-page2');
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);
 await point(20,120);await page.waitForTimeout(650);await capture('browser-profile3-combat-tutorial-back');
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);
 await point(300,120);await page.waitForTimeout(650);
 await point(300,120);await page.waitForTimeout(650);await capture('browser-profile3-combat-tutorial-page3');
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,0);
 await point(300,120);const learned=await waitTutorial(3,tutorial.combat,tutorial.combat);
 report.tutorial={profile:3,neow_control_steps:neowSteps,flags_after_lesson:learned.flags};
 await capture('browser-profile3-combat-tutorial-completed');await saveAndResume();
 const tutorialSave=await saved(3),tutorialBytes=Buffer.from(tutorialSave,'base64');
 assert.equal(tutorialBytes.readUInt32LE(4),17);assert.equal(tutorialBytes.readUInt32LE(12),0); // Phase::Combat.
 assert.equal(tutorialBytes.readUInt32LE(56),1); // The unchanged v17 prefix's turn field.
 await page.reload({waitUntil:'load'});await ready('eng');assert.equal(await saved(3),tutorialSave);
 assert.equal((await tutorialRecord(3)).flags&tutorial.combat,tutorial.combat);
 await titleRow(0,true);await page.waitForTimeout(1200);await capture('browser-profile3-combat-resumed');
 await returnToTitle();await selectProfile(1);assert.equal(await saved(),firstSave);
 assert.equal((await tutorialRecord(1)).flags,0x4001);assert.equal((await tutorialRecord(3)).flags&tutorial.combat,tutorial.combat);
 record('a separate fresh profile accepts tutorials, reaches the real first battle, navigates all three pages and persists completion only on the final Next release');
 record('the combat tutorial blocks gameplay shortcuts and End turn; reload resumes turn one without repeating the completed lesson or changing another profile');
 // The deleted second profile supplies an ordinary, deterministic route to
 // the native simple selector. No state, save or completion bit is injected.
 await selectProfile(2);assert.equal(await saved(2),null);
 await titleRow(0,false);await press('ArrowRight');await press('ArrowRight');await confirm();
 await press('m');await press('ArrowDown');await confirm(); // Sealed Deck, modifier row one.
 await capture('browser-sealed-custom-mode');await page.locator('button[data-key="6"]').click();await painted();
 await declineTutorials(2,'browser-sealed-tutorial');await saveAndResume();
 const sealedBaseline=await saved(2),sealedBefore=runPrefix(sealedBaseline);
 assert.equal(sealedBefore.phase,10);assert.equal(sealedBefore.deck_count,0); // Actual Neow Event before its modifier option.
 await page.waitForTimeout(1000);await confirm();await page.waitForTimeout(650);
 await capture('browser-sealed-thirty-card-grid');
 // NCardGrid wraps Right within its four-card row. Move down only after
 // wrapping to that row's first column, so these are nine distinct cards.
 for(let i=0;i<9;++i){await confirm();await press('ArrowRight');if(i%4===3)await press('ArrowDown');}
 await page.waitForTimeout(650);await capture('browser-sealed-nine-selected');
 await press('Escape');assert.equal(await saved(2),sealedBaseline);
 await confirm();await page.waitForTimeout(650);await capture('browser-sealed-ten-manual-confirm');
 // Toggling the tenth card back off must leave a usable selector. An E
 // release with nine cards cannot complete it; reselect before confirming.
 await confirm();await page.waitForTimeout(650);await press('e');
 await capture('browser-sealed-deselected-tenth');await confirm();await page.waitForTimeout(650);
 await canvas.focus();await page.keyboard.down('e');await painted();await capture('browser-sealed-confirm-held');
 assert.equal(await saved(2),sealedBaseline);await page.keyboard.up('e');await painted();await page.waitForTimeout(650);
 await capture('browser-sealed-completed');await returnToTitle();
 const sealedSave=await saved(2),sealedAfter=runPrefix(sealedSave);
 assert.notEqual(sealedSave,sealedBaseline);assert.equal(sealedAfter.phase,10);assert.equal(sealedAfter.deck_count,10);
 report.selector={profile:2,kind:'Sealed Deck',before:sealedBefore,after:sealedAfter,
  canonical_snapshot_scope:'Save and Quit before opening the selector and after manual confirmation; intermediate UI purity is covered by the matching native suite'};
 assert.equal(await saved(),firstSave);assert.equal(await saved(3),tutorialSave);await selectProfile(1);
 record('ordinary Custom Sealed Deck opens the thirty-card grid, retains manual selection through cancellation and deselection, and commits exactly ten cards through the real E release');
 // NInspectRelicScreen is reachable from the ordinary Compendium. Compare
 // the opaque popup and its source icon while letting its 250ms motion end.
 await press('c');await capture('browser-compendium-eng');await press('ArrowRight');await confirm();await page.waitForTimeout(650);
 await capture('browser-relic-collection');await confirm();await page.waitForTimeout(650);
 await capture('browser-relic-inspector-first');const firstRelic=await canvasPixels(142,87,37,37);
 await point(160,100);await page.waitForTimeout(300);
 // The game draws its own cursor into this icon rectangle after the click.
 // Move within the paper without clicking before comparing the icon itself.
 await move(160,195);await capture('browser-relic-inspector-popup-retained');
 assert.deepEqual(await canvasPixels(142,87,37,37),firstRelic,'clicking the popup must not close the relic inspector');
 await press('ArrowRight');await page.waitForTimeout(350);const nextRelic=await canvasPixels(142,87,37,37);
 assert.ok(pixelDifference(firstRelic,nextRelic)>.5,'the real inspector arrow must advance to another relic');await capture('browser-relic-inspector-next');
 await press('ArrowLeft');await page.waitForTimeout(350);
 assert.ok(pixelDifference(firstRelic,await canvasPixels(142,87,37,37))<2,'the previous arrow returns to the same original relic');
 await point(12,120);await page.waitForTimeout(300);await capture('browser-relic-inspector-backstop-closed');
 await confirm();await page.waitForTimeout(350);await press('Escape');await page.waitForTimeout(300);await press('Escape');
 await press('ArrowRight');await confirm();await press('ArrowRight');await press('ArrowLeft');await page.waitForTimeout(650);
 const potionGrid=await canvasPixels(78,44,165,165);await confirm();await page.waitForTimeout(350);
 assert.ok(pixelDifference(potionGrid,await canvasPixels(78,44,165,165))<6,'Potion Lab confirmation keeps its collection and hover instead of opening an inspector');
 await capture('browser-potion-lab-focus-only');await press('Escape');await press('ArrowLeft');await press('ArrowLeft');
 assert.equal(await saved(),firstSave);assert.equal(await saved(2),sealedSave);assert.equal(await saved(3),tutorialSave);
 record('ordinary relic collection opens the source inspector with clamped model navigation and popup/backstop dismissal; Potion Lab confirmation remains focus-only without mutating any saved run');
 // Measure through the visible control with the actual library active.
 await confirm();await capture('browser-card-library-eng');
 await page.locator('#diagnostics summary').click();await page.locator('#measure').click();
 await page.waitForFunction(()=>document.querySelector('#performance-report').textContent.trim().startsWith('{'),{},{timeout:20000});
 report.performance=JSON.parse(await page.locator('#performance-report').innerText());
 assert.equal(report.performance.finished_reason,'complete');
 assert.ok(report.performance.scenes.some(scene=>scene.scene==='collection'&&scene.render.count>0));
 record('visible 10-second performance control renders the actual card library');
 // Return to the actual first battle created above. Preference writes and
 // run transitions use only visible controls; localStorage is read solely
 // to verify what the ordinary host persisted.
 await page.reload({waitUntil:'load'});await ready('eng');await selectProfile(3);
 await openSettings(true);await point(settingsControlX,settingsRowCenter(6));
 assert.equal((await preference(3)).version,5);assert.equal((await preference(3)).flags&128,128);
 assert.equal((await preference(1)).flags&128,0);assert.equal((await preference(2))?.flags&128,0);
 await capture('browser-long-press-setting');await page.reload({waitUntil:'load'});await ready('eng');
 assert.equal((await preference(3)).flags&128,128);await titleRow(0,true);await page.waitForTimeout(2500);
 const endToolbar=page.locator('button[data-key="6"]');
 await endToolbar.click();await painted();
 // The source short-release path may show its still-playable-cards lesson;
 // completing that lesson must itself leave the canonical first turn alone.
 await waitTutorial(3,8,8);await capture('browser-long-press-short-release-tutorial');await confirm();await page.waitForTimeout(400);
 await press('e');await move(288,196);await page.mouse.down();await page.waitForTimeout(150);
 await capture('browser-long-press-pointer-progress');await move(240,160);await page.waitForTimeout(650);
 await page.mouse.up();await painted();await returnToTitle();
 assert.equal(await saved(3),tutorialSave);
 record('Long Press Confirmations persists only in its selected profile; short toolbar and keyboard taps plus moving out of the canvas End turn control leave the real first-turn save unchanged');
 await titleRow(0,true);await page.waitForTimeout(2500);await canvas.focus();
 await page.keyboard.down('e');await page.waitForTimeout(1300);await page.keyboard.down('e');
 await page.waitForTimeout(1700);await page.keyboard.up('e');await painted();
 await capture('browser-long-press-keyboard-completed');await returnToTitle();
 const keyboardHoldSave=await saved(3),keyboardHold=runPrefix(keyboardHoldSave);
 assert.equal(keyboardHold.phase,0);assert.equal(keyboardHold.turn,2);
 record('holding the real mapped E key beyond half a second, including an OS-repeat event, ends exactly one turn before release');
 await titleRow(0,true);await page.waitForTimeout(2500);
 const beforeToolbarTutorial=(await tutorialRecord(3)).flags;
 await endToolbar.scrollIntoViewIfNeeded();const toolbarBox=await endToolbar.boundingBox();assert(toolbarBox);
 await page.mouse.move(toolbarBox.x+toolbarBox.width/2,toolbarBox.y+toolbarBox.height/2);await page.mouse.down();
 await page.waitForTimeout(3000);await page.mouse.up();await painted();await page.waitForTimeout(1600);
 // The starter deck can reach its first shuffle on turn three. That source
 // modal pauses animation until its real confirmation button is released.
 if(!(beforeToolbarTutorial&64)&&((await tutorialRecord(3)).flags&64)){
  await capture('browser-long-press-shuffle-tutorial');await confirm();await page.waitForTimeout(1500);
 }
 await capture('browser-long-press-toolbar-completed');await returnToTitle();
 const toolbarHoldSave=await saved(3),toolbarHold=runPrefix(toolbarHoldSave);
 assert.equal(toolbarHold.phase,0);assert.equal(toolbarHold.turn,3);
 assert.equal(await saved(1),firstSave);assert.equal(await saved(2),sealedSave);
 await page.reload({waitUntil:'load'});await ready('eng');
 assert.equal(await saved(3),toolbarHoldSave);assert.equal((await preference(3)).flags&128,128);
 report.long_press={profile:3,before:runPrefix(tutorialSave),after_keyboard:keyboardHold,after_toolbar:toolbarHold,
  input_scope:'Real canvas pointer cancellation, physical E down/repeat/up and HTML toolbar pointer down/up; no application test hook or save injection'};
 record('holding the ordinary HTML End turn button ends exactly one further turn; reload preserves the v5 setting and v17 run while the other two profile saves remain exact');
 if(debugHtml){
  // A separate origin context prevents developer mutations from altering the
  // ordinary acceptance save. Only the shipped, flag-gated console is used.
  const debugContext=await browser.newContext({viewport:{width:1000,height:1100},locale:'en-US',serviceWorkers:'block'});
  await debugContext.route('**/*',route=>{const url=route.request().url();if(url.startsWith('file:'))return route.continue();report.blocked_requests.push(url);return route.abort();});
  const debugPage=await debugContext.newPage();debugPage.on('pageerror',error=>report.errors.push(error.message));
  await debugPage.goto(pathToFileURL(path.resolve(debugHtml)).href,{waitUntil:'load',timeout:120000});
  await debugPage.waitForFunction(()=>typeof globalThis.nSpire2Debug==='function'&&!document.querySelector('#measure').disabled,{},{timeout:60000});
  assert.equal(await debugPage.locator('#error').innerText(),'');
  const catalog=await debugPage.evaluate(()=>({encounters:nSpire2DebugList('encounters'),events:nSpire2DebugList('events')}));
  assert.ok(catalog.encounters.length>20&&catalog.events.length>20);
  assert.equal(await debugPage.evaluate(()=>nSpire2Debug('new regent 12345').ok),true);
  assert.equal(await debugPage.evaluate(()=>nSpire2Debug('room monster').ok),true);
  await debugPage.waitForTimeout(1500);
  await debugPage.locator('#screen').focus();await debugPage.keyboard.press('F1');await debugPage.keyboard.press('i');
  for(let i=0;i<16;++i)await debugPage.keyboard.press('Backspace');
  await debugPage.keyboard.type('kill all');await debugPage.keyboard.press('Enter');
  await debugPage.waitForTimeout(1500);
  const debugShot=await debugPage.locator('#screen').evaluate(node=>node.toDataURL('image/png'));
  fs.writeFileSync(path.join(directory,'browser-debug-kill-menu.png'),Buffer.from(debugShot.split(',')[1],'base64'));
  report.captures.push('browser-debug-kill-menu.png');
  assert.equal(await debugPage.evaluate(()=>nSpire2Debug('kill all').ok),false); // already resolved by real text entry
  for(const command of ['room shop','room rest','room treasure','room map','act 3','room event '+catalog.events[0]]){
   const result=await debugPage.evaluate(value=>nSpire2Debug(value),command);assert.equal(result.ok,true,command+': '+result.message);
  }
  assert.equal(await debugPage.evaluate(()=>nSpire2Debug('room event NOT_AN_EVENT').ok),false);
  assert.equal(await debugPage.evaluate(()=>localStorage.getItem('nSpire2-save-v17')),null);
  record('separate debug HTML exposes catalogs, accepts real F1 text entry, room/act commands and rejects invalid events without autosaving');
  await debugContext.close();
 }
 assert.deepEqual(report.errors,[]);assert.deepEqual(report.blocked_requests,[]);
 record('no page errors or external resource requests');
 report.status='passed';report.ended_at=new Date().toISOString();writeReport();
 await browser.close();
})().catch(async error=>{
 report.status='failed';report.error=error.stack||String(error);report.ended_at=new Date().toISOString();writeReport();
 if(browser)await browser.close().catch(()=>{});console.error(error);process.exitCode=1;
});
