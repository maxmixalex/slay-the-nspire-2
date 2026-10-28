// Semantic toolbar controls retain real press/release lifetimes. A short
// browser-generated click must not bypass the game's optional hold gate.
function bindCommandButtons(buttons,actions){
 let active=null,keyboardClick=false;
 function clear(){
  const previous=active;active=null;
  if(previous&&previous.pointer!==undefined&&previous.node.hasPointerCapture(previous.pointer))previous.node.releasePointerCapture(previous.pointer);
  return previous;
 }
 function cancel(){if(clear())actions.cancel();}
 function inside(node,event){const r=node.getBoundingClientRect();return event.clientX>=r.left&&event.clientX<r.right&&event.clientY>=r.top&&event.clientY<r.bottom;}
 function begin(node,extra){cancel();active={node,code:Number(node.dataset.key),...extra};actions.press(active.code);}
 for(const node of buttons){
  node.addEventListener('pointerdown',event=>{
   if(event.button!==0||node.disabled)return;event.preventDefault();
   begin(node,{pointer:event.pointerId});node.setPointerCapture(event.pointerId);
  });
  node.addEventListener('pointermove',event=>{if(active&&active.node===node&&active.pointer===event.pointerId&&!inside(node,event))cancel();});
  node.addEventListener('pointerleave',event=>{if(active&&active.node===node&&active.pointer===event.pointerId)cancel();});
  node.addEventListener('pointerup',event=>{
   if(event.button!==0||!active||active.node!==node||active.pointer!==event.pointerId)return;event.preventDefault();
   if(!inside(node,event)){cancel();return;}const previous=clear();actions.release(previous.code);actions.focus();
  });
  for(const type of ['pointercancel','lostpointercapture'])node.addEventListener(type,event=>{
   if(active&&active.node===node&&active.pointer===event.pointerId)cancel();
  });
  node.addEventListener('keydown',event=>{
   if(event.key!==' '&&event.key!=='Enter')return;event.preventDefault();event.stopPropagation();
   if(!event.repeat&&!node.disabled)begin(node,{keyboard:event.key});
  });
  node.addEventListener('keyup',event=>{
   if(event.key!==' '&&event.key!=='Enter')return;event.preventDefault();event.stopPropagation();
   if(!active||active.node!==node||active.keyboard!==event.key)return;
   const previous=clear();keyboardClick=true;setTimeout(()=>{keyboardClick=false;},0);
   actions.release(previous.code);actions.focus();
  });
  node.addEventListener('blur',()=>{if(active&&active.node===node&&active.keyboard!==undefined)cancel();});
  node.addEventListener('click',event=>{
   event.preventDefault();
   // Pointer clicks were already handled at pointerup. Assistive/programmatic
   // activation is a zero-duration tap and therefore never completes a hold.
   if(event.detail!==0||keyboardClick||node.disabled)return;
   cancel();const code=Number(node.dataset.key);actions.press(code);actions.release(code);actions.focus();
  });
 }
 return {cancel};
}
if(typeof module!=='undefined'&&module.exports)module.exports=bindCommandButtons;
