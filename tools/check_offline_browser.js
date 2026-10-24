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
 async function press(key){await canvas.focus();await page.keyboard.press(key);await painted();}
 async function confirm(){await page.locator('button[data-key="4"]').click();await painted();}
 const saved=(slot=1)=>page.evaluate(id=>localStorage.getItem('nSpire2-save-v15'+(id===1?'':'-profile-'+id)),slot);
 const preference=(slot=1)=>page.evaluate(id=>JSON.parse(localStorage.getItem('nSpire2-preferences-v2'+(id===1?'':'-profile-'+id))),slot);
 async function point(x,y){const box=await canvas.boundingBox();assert(box);await page.mouse.click(box.x+x*box.width/320,box.y+y*box.height/240);await painted();}
 async function openSettings(hasRun){for(let i=0;i<(hasRun?2:1);++i)await press('ArrowDown');await confirm();}
 async function toggleFast(){await press('ArrowDown');await press('ArrowDown');await confirm();}
 async function returnToTitle(){await press('Escape');for(let i=0;i<4;++i)await press('ArrowDown');await confirm();}
 async function selectProfile(slot){await point(20,10);await point(72+(slot-1)*88,120);await page.waitForFunction(id=>localStorage.getItem('nSpire2-selected-profile')===String(id),slot);await painted();}
 const begin=Date.now();
 await page.goto(pathToFileURL(path.resolve(html)).href,{waitUntil:'load',timeout:120000});
 await ready('eng');report.first_ready_wall_ms=Date.now()-begin;
 assert.equal(await saved(),null);record('fresh English title without a startup language chooser or external language control');
 assert.equal(await page.evaluate(()=>typeof globalThis.nSpire2Debug),'undefined');
 record('ordinary HTML does not expose the developer command API');
 await capture('browser-title-eng');
 await openSettings(false);await toggleFast();
 assert.equal((await preference()).flags&1,1);
 await capture('browser-settings-persisted-eng');
 await page.reload({waitUntil:'load'});await ready('eng');
 await openSettings(false);await toggleFast();
 assert.equal((await preference()).flags&1,0);
 await press('Escape');await press('ArrowUp');
 record('in-game Fast Mode survives reload and the real toggle reads its restored value');
 await confirm();await capture('browser-character-eng');
 await confirm();await press('s');
 await page.waitForFunction(()=>!!localStorage.getItem('nSpire2-save-v15'));
 const firstSave=await saved(),bytes=Buffer.from(firstSave,'base64');
 assert.ok(bytes.length>40);report.saved_bytes=bytes.length;
 fs.writeFileSync(path.join(directory,'browser-test-save-v15.bin'),bytes);
 record('DOM Confirm starts a run; S stores a nonempty v15 browser save');
 await capture('browser-run-eng');
 await press('o');assert.equal(await saved(),firstSave);await capture('browser-loaded-eng');
 record('O load returns through the real keyboard handler without changing stored save bytes');
 await page.reload({waitUntil:'load'});await ready('eng');assert.equal(await saved(),firstSave);
 await capture('browser-continue-eng');await confirm();await press('s');assert.equal(await saved(),firstSave);
 record('reload offers Continue and resumed run saves the same canonical bytes');
 await returnToTitle();await openSettings(true);
 async function switchLanguage(language){
  const origin=await page.evaluate(()=>performance.timeOrigin);
  await point(241,73);await point(241,language==='eng'?98:119);
  await page.waitForFunction(lang=>document.documentElement.lang===(lang==='eng'?'en':'zh-CN')&&localStorage.getItem('nSpire2-language')===lang,language,{timeout:60000});
  await ready(language);assert.equal(await page.evaluate(()=>performance.timeOrigin),origin);
  assert.equal(await saved(),firstSave);
 }
 await switchLanguage('zhs');await capture('browser-settings-zhs');
 await press('Escape');await confirm();await press('s');assert.equal(await saved(),firstSave);await press('o');
 await capture('browser-run-zhs');record('language changes inside the title settings, without reload or changing the saved run');
 await page.reload({waitUntil:'load'});await ready('zhs');assert.equal(await saved(),firstSave);
 await capture('browser-reloaded-zhs');record('reload remembers Chinese and the same save');
 await openSettings(true);await switchLanguage('eng');await press('Escape');
 assert.deepEqual(report.dialogs,[]);record('original language dropdown changes directly without a browser confirmation dialog');
 await selectProfile(2);assert.equal(await saved(2),null);assert.equal(await saved(),firstSave);
 await capture('browser-profile2-title');await openSettings(false);await toggleFast();
 assert.equal((await preference(2)).flags&1,1);assert.equal((await preference()).flags&1,0);
 await press('Escape');await press('ArrowUp');await confirm();await press('ArrowRight');await confirm();await press('s');
 const secondSave=await saved(2);assert(secondSave&&secondSave!==firstSave);assert.equal(await saved(),firstSave);
 await capture('browser-profile2-run');await returnToTitle();
 await selectProfile(1);await openSettings(true);await toggleFast();assert.equal((await preference()).flags&1,1);
 await confirm();assert.equal((await preference()).flags&1,0);await press('Escape');
 // The original profile picker is a real modal. Cancel leaves the other
 // profile's run/history/preferences intact; active deletion resets only it.
 await selectProfile(2);await page.reload({waitUntil:'load'});await ready('eng');
 assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-selected-profile')),'2');
 await openSettings(true);await toggleFast();assert.equal((await preference(2)).flags&1,0);
 await confirm();assert.equal((await preference(2)).flags&1,1);await press('Escape');
 await point(20,10);await capture('browser-profiles');await point(72,192);await capture('browser-profile-delete-confirm');await press('Escape');
 assert.equal(await saved(),firstSave);assert.equal(await saved(2),secondSave);
 await point(160,192);await press('ArrowRight');await confirm();
 assert.equal(await saved(2),null);assert.equal(await preference(2),null);assert.equal(await saved(),firstSave);
 assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-progress-v1-profile-2')),null);
 assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-language')),'eng');
 await capture('browser-profile2-deleted');await point(72,120);
 record('three-profile controls preserve legacy slot one, isolate runs and preferences, persist selection, cancel deletion and reset an active profile');
 // Measure through the visible control with the actual library active.
 await press('c');await capture('browser-compendium-eng');await confirm();await capture('browser-card-library-eng');
 await page.locator('#diagnostics summary').click();await page.locator('#measure').click();
 await page.waitForFunction(()=>document.querySelector('#performance-report').textContent.trim().startsWith('{'),{},{timeout:20000});
 report.performance=JSON.parse(await page.locator('#performance-report').innerText());
 assert.equal(report.performance.finished_reason,'complete');
 assert.ok(report.performance.scenes.some(scene=>scene.scene==='collection'&&scene.render.count>0));
 record('visible 10-second performance control renders the actual card library');
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
  assert.equal(await debugPage.evaluate(()=>localStorage.getItem('nSpire2-save-v15')),null);
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
