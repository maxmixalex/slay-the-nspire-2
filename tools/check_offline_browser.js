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
const [html,output,modulePath,browserPath]=process.argv.slice(2);
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
  await page.waitForFunction(()=>document.querySelector('#loading')?.hidden||document.querySelector('#error')?.textContent,{},{timeout:60000});
  assert.equal(await page.locator('#error').innerText(),'');
  assert.equal(await page.locator('#language').inputValue(),language);
  assert.equal(await page.locator('html').getAttribute('lang'),language==='eng'?'en':'zh-CN');
  assert.equal(await page.locator('#language').isEnabled(),true);
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
 const saved=()=>page.evaluate(()=>localStorage.getItem('nSpire2-save-v15'));
 const begin=Date.now();
 await page.goto(pathToFileURL(path.resolve(html)).href,{waitUntil:'load',timeout:120000});
 await ready('eng');report.first_ready_wall_ms=Date.now()-begin;
 assert.equal(await saved(),null);record('fresh English startup, localized DOM and ready Canvas');
 await capture('browser-title-eng');
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
 async function switchLanguage(language){
  const loaded=page.waitForEvent('load',{timeout:60000});
  await page.locator('#language').selectOption(language);await loaded;await ready(language);
  assert.equal(await saved(),firstSave);
  assert.equal(await page.evaluate(()=>localStorage.getItem('nSpire2-language')),language);
 }
 await switchLanguage('zhs');await capture('browser-continue-zhs');
 await confirm();await press('s');assert.equal(await saved(),firstSave);await press('o');
 await capture('browser-run-zhs');record('Chinese language selection, confirmation, Continue and S/O share the original save');
 await page.reload({waitUntil:'load'});await ready('zhs');assert.equal(await saved(),firstSave);
 await capture('browser-reloaded-zhs');record('reload remembers Chinese and the same save');
 acceptDialog=false;await page.locator('#language').selectOption('eng');await painted();
 assert.equal(await page.locator('#language').inputValue(),'zhs');assert.equal(await saved(),firstSave);
 record('cancelling the language confirmation preserves the selected language and save');
 acceptDialog=true;await switchLanguage('eng');
 // Exercise the existing visible measurement control while the card grid is
 // on screen. The page report keeps CPU/Canvas submission timing distinct
 // from compositor FPS and physical-device performance.
 await page.locator('button[data-key="17"]').click();await painted();await capture('browser-compendium-eng');
 await page.locator('#diagnostics summary').click();await page.locator('#measure').click();
 await page.waitForFunction(()=>document.querySelector('#performance-report').textContent.trim().startsWith('{'),{},{timeout:20000});
 report.performance=JSON.parse(await page.locator('#performance-report').innerText());
 assert.equal(report.performance.finished_reason,'complete');
 assert.ok(report.performance.scenes.some(scene=>scene.scene==='collection'&&scene.render.count>0));
 record('visible 10-second performance control completes while rendering the card collection');
 assert.deepEqual(report.errors,[]);assert.deepEqual(report.blocked_requests,[]);
 record('no page errors or external resource requests');
 report.status='passed';report.ended_at=new Date().toISOString();writeReport();
 await browser.close();
})().catch(async error=>{
 report.status='failed';report.error=error.stack||String(error);report.ended_at=new Date().toISOString();writeReport();
 if(browser)await browser.close().catch(()=>{});console.error(error);process.exitCode=1;
});
