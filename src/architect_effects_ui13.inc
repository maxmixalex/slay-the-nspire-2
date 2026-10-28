// VisualOnly encounters have no canonical combat damage notices. Their
// original command still constructs the same NDamageNumVfx and NHitSparkVfx
// at each real hit cue, with requireInteractable:false.
#include "architect_attack_ui13.inc"
static void architect_scene_effects(Canvas&c){
 auto&a=c.a;const auto&v=a.room_presentation.architect;
 if(!v.bound||a.game.original_events.id!=OriginalEventId::TheArchitect)return;
 if(v.plan.architect_at){
  const unsigned attack=v.plan.architect_at-v.plan.windup*5u;
  const int age=int32_t(a.map_clock_ms-v.start-attack);
  if(age>=200&&age<2167){const unsigned frame=mn((age-200)/50,int(I13_ArchitectTrailCount)-1);
   c.ui(I13_ArchitectTrailOuter+frame,0,22,320,218);
   c.picture(a.content.ui_image(I13_ArchitectTrailInner+frame),0,22,320,218,255,presentation::Add);
  }
 }
 combat_hit_sparks(c,a.map_clock_ms);
 const auto target=architect_scene_pose(a.game.character,true);
 for(unsigned i=0;i<v.plan.hits;++i){const auto&hit=v.hits[i];
  const int ms=int32_t(a.map_clock_ms-v.start-v.plan.hit_at(i));if(ms<0)continue;
  architect_source_attack(c,hit.vfx,unsigned(ms),target.center_x,target.center_y);
  if(ms>=2000)continue;
  // NDamageNumVfx uses a cosmetic random velocity and size. The values
  // remain stable across repeat render, pause and independent room clocks.
  const unsigned variation=hit.seed&255u;
  const int vx=int((hit.seed>>8)%201u)-100,vy=-800+int((hit.seed>>16)%101u);
  const int cx=target.center_x+vx*ms*4/21000+int(variation%5u)-2;
  const int cy=target.center_y+(vy*ms+ms*ms)*4/21000;
  if(cy>250||cy<-50)continue;
  const unsigned u=unsigned(ms)*1024u/2000u,alpha=255u*(1024u-u*u/1024u)/1024u;
  const unsigned rest=ms<1200?unsigned(1200-ms)*1024u/1200u:0u;
  const int base=(1200+int(variation))*64*1024*4/(1000*21*16);
  const int scale=base*(1024+int(1536u*rest/1024u*rest/1024u))/1024;
  Canvas text=c;text.opacity=uint8_t(unsigned(c.opacity)*alpha/255u);
  const unsigned color=CombatCardMotion::cubic(unsigned(ms),500u)*255u/1024u;
  combat_damage_label(text,int(hit.amount),cx,cy,scale,blend(rgb(247,42,20),rgb(255,246,226),color));
 }
 if(v.plan.architect_at){const int age=int32_t(a.map_clock_ms-v.start-v.plan.architect_at);
  if(age>=0){const auto player=architect_scene_pose(a.game.character,false);
   Canvas burst=c;
   // Only the creature children move above NGameOverScreen's backstop.
   // Its CombatVfxContainer remains below that200ms fade, unlike the pen.
   if(a.game.phase==Phase::Complete)burst.opacity=uint8_t(div255(unsigned(burst.opacity)*(255u-a.game_over_presentation.black_alpha(a.map_clock_ms))));
   if(burst.opacity){
    architect_source_attack(burst,ArchitectPresentation::Lightning,unsigned(age),player.center_x,player.center_y);
    architect_source_attack(burst,ArchitectFireBurst,unsigned(age),player.body_left+player.body_width/2,player.body_top+player.body_height,(a.game.seed&1u)!=0);
   }
  }
 }
}
