// Original potion_popup textures: a bottle-anchored Use/Discard menu, with a
// separate hover tip rather than replacing the entire combat scene.
static void potion_popup_screen(Canvas&c){
 auto&a=c.a;auto&s=a.game;if(a.potion_popup<0)return;
 if(a.potion_popup>=s.potion_slots||s.revision!=a.potion_revision||s.potions[a.potion_popup]!=a.popup_bottle){a.close_potion_popup();return;}
 const auto*d=potion_definition(a.popup_bottle);if(!d){a.close_potion_popup();return;}
 a.hit_count=0;c.action(0,0,320,240,UiAction::ClosePopupPotion);
 int slot=a.potion_popup,x=potion_popup_x(slot,s.potion_slots),cx=potion_slot_x(slot,s.potion_slots)+9;
 if(a.potion_targeting){
  for(int i=0;i<s.enemy_count;i++)if(s.enemies[i].hp){int ex=s.enemy_count==1?238:178+i*88;
   c.box(ex-35,68,70,67,a.target==i?Gold:Muted);c.hit(ex-36,48,73,97,HitKind::PotionTarget,i);
  }
  int ex=a.pointer.visible?a.pointer.x:(s.enemy_count==1?238:178+a.target*88),ey=a.pointer.visible?a.pointer.y:85;
  // Same original arrow head/segment assets, sampled along a small-screen arc.
  int dx=ex-cx,dy=ey-21;
  for(int i=1;i<9;i++){int px=cx+dx*i/9,py=21+dy*i/9-12*i*(9-i)/81;c.ui(U_TargetSegment,px-2,py-2,4,4);}
  c.ui(U_TargetHead,ex-6,ey-6,12,12);c.dim(70,211,180,23,185);c.centered("选择目标 · Esc 取消",70,215,180,10,Gold);
 }else{
  c.ui(U_PotionPopup,x,18,91,85);
  bool usable=false;for(int i=0;i<(s.enemy_count?s.enemy_count:1);i++)usable|=can_use_potion(s,slot,i);
  if(a.potion_menu==0&&usable)c.ui(U_PotionPopupTop,x+3,21,85,47,110);
  if(a.potion_menu==1)c.ui(U_PotionPopupBottom,x+3,60,85,34,125);
  c.centered(d->automatic?"自动触发":"使用",x+5,40,81,12,usable?White:Muted);
  c.centered("丢弃",x+5,72,81,12,rgb(245,116,105));
  c.action(x+6,32,79,30,UiAction::UsePopupPotion,usable);c.action(x+6,63,79,31,UiAction::DiscardPopupPotion);
  int tx=x>=142?6:x+96,tw=x>=142?x-12:314-tx;
  int lines=c.paragraph(d->description,tx+8,70,tw-16,0,0,10),visible=mn(10,lines),height=46+visible*13;
  c.rect(tx,34,tw,height,Panel);c.box(tx,34,tw,height,rgb(102,92,70));
  c.text(d->name,tx+8,40,12,Gold,tw-16);c.paragraph(d->description,tx+8,62,tw-16,visible,0,10,White);
  c.text("I 完整说明",tx+8,62+visible*13,10,Muted,tw-16);
  c.action(tx,34,tw,height,UiAction::DetailPopupPotion);
 }
 for(int i=0;i<s.potion_slots;i++){
  int px=potion_slot_x(i,s.potion_slots);if(s.potions[i]==Potion::Empty)continue;
  if(i==slot)c.box(px-1,0,20,22,Gold);
  c.hit(px-1,0,20,22,HitKind::PotionSlot,i);
 }
}
