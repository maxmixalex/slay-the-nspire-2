// Independent source geometry and real Event -> Proceed -> Complete rendering.
// The shared fixture supplies previous room outcomes to the production world
// controller. It is not evidence of an ordinary three-act combat victory.
#define main screen_effects_previous_main
#include "screen_effects_ui12_regression.cpp"
#undef main
#undef CHECK
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Architect visual UI12 [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)

static const char* captures=nullptr;
static bool original_art=false;
static const char* names[]={"ironclad","silent","defect","regent","necrobinder"};
static constexpr int idle_clips[]={0,11,17,22,27};
static constexpr int attack_clips[]={1,12,18,23,28};
struct SourcePose {int x,y,scale,units;};
static SourcePose source_pose(unsigned character,bool enemy=false){
 // NCombatRoom: 1680x1260 viewport; +/-480,200 from its center. The
 // Necrobinder shifts left150 even though CombatEventVisuals has no Osty.
 // Registration values are the existing original skeleton bake contract.
 static constexpr int units[]={4483,3239,3618,4541,5120},dx[]={5,16,4,2,-2},dy[]={-19,-21,-27,-20,-6};
 if(enemy)return {1320*4/21,830*4/21,6261*4/21,6261};
 return {(360-(character==4?150:0)+dx[character])*4/21,(830+dy[character])*4/21,units[character]*4/21,units[character]};
}
static void geometry(){
 scenario="source VisualOnly formation and original sprite registration";
 for(unsigned n=0;n<5;++n)for(bool enemy:{false,true}){
  const auto expected=source_pose(n,enemy);const auto got=architect_scene_pose(Character(n),enemy);
  CHECK(got.sprite_x==expected.x&&got.sprite_y==expected.y&&got.scale==expected.scale);
  const auto anchor=enemy?creature_anchors(EnemyId::Architect):player_anchors(Character(n));
  CHECK(got.center_y==(830+anchor.center_y)*4/21&&got.body_top==(830+anchor.top)*4/21);
  CHECK(got.feet==830*4/21);
 }
}
static void displacement_rows(){
 scenario="source distortion reads untouched rows without a second framebuffer";
 std::vector<uint16_t> original(ScreenW*ScreenH),actual(ScreenW*ScreenH);
 for(unsigned i=0;i<original.size();++i)original[i]=uint16_t((i*1777u)^(i/ScreenW*3141u));
 actual=original;std::vector<uint16_t> field(64*100);
 for(int y=0;y<100;++y)for(int x=0;x<64;++x){const int dx=x&1?7:-7,dy=x&2?13:-13;
  field[y*64+x]=uint16_t((dx+16)<<11|(dy+32)<<5);}
 Canvas c{a};c.pixels=actual.data();
 architect_source_displacement(c,Picture{reinterpret_cast<const uint8_t*>(field.data()),nullptr,nullptr,64,100,0},-5,15);
 for(int y=0;y<ScreenH;++y)for(int x=0;x<ScreenW;++x){uint16_t expected=original[y*ScreenW+x];
  if(y>=22&&y<115&&x<59){const int source=x+5,dx=source&1?7:-7,dy=source&2?13:-13;
   const int xx=std::max(0,std::min(ScreenW-1,x+dx)),yy=std::max(22,std::min(ScreenH-1,y+dy));expected=original[yy*ScreenW+xx];}
  CHECK(actual[y*ScreenW+x]==expected);
 }
}
static void until(uint32_t time){CHECK(int32_t(time-a.map_clock_ms)>=0);advance_room(time-a.map_clock_ms);}
static void repeat_render(){
 const std::vector<uint16_t> first(a.pixels,a.pixels+ScreenW*ScreenH);a.render();
 CHECK(!std::memcmp(first.data(),a.pixels,sizeof a.pixels));equal_save();
}
static void picture_at(Canvas&canvas,int id,unsigned frame,SourcePose pose,unsigned mode=0){
 const auto clip=a.content.clip(id);CHECK(clip.count&&frame<clip.count);
 const auto image=a.content.image(int(clip.first)+int(frame));CHECK(image.pixels&&image.w&&image.h);
 const int scale=clip.sprite_units?int(int64_t(pose.scale)*clip.sprite_units/pose.units):pose.scale;
 canvas.picture(image,pose.x-clip.pivot_x*scale/1024,pose.y-clip.pivot_y*scale/1024,
                std::max(1,int(image.w)*scale/1024),std::max(1,int(image.h)*scale/1024),255,mode);
}
static unsigned sample(int id,unsigned elapsed,bool loop){
 const auto clip=a.content.clip(id);CHECK(clip.count&&clip.frame_ms);
 return loop?(elapsed/clip.frame_ms)%clip.count:std::min(elapsed/clip.frame_ms,unsigned(clip.count-1));
}
static void visible_original(int id,unsigned frame,SourcePose pose,unsigned minimum=20){
 if(!original_art)return;
 // Probe opaque source texels at literal source anchors in the actual App
 // frame. No actor-selection, actor-pose or game-over rendering helper is
 // used to manufacture the expected image.
 std::vector<uint16_t> expected(ScreenW*ScreenH,0);std::vector<uint8_t> alpha(ScreenW*ScreenH,0);
 Canvas probe{a};probe.pixels=expected.data();probe.coverage=alpha.data();
 const auto&event=a.room_presentation.architect;
 const unsigned reading=a.game.character!=Character::Silent?I13_ArchitectReading:event.fast?I13_ArchitectSilentFastReading:I13_ArchitectSilentReading;
 if(id>=49&&id<=51&&event.intro(a.map_clock_ms)&&a.content.ui_image(reading)>=0)
  probe.picture(a.content.ui_image(reading+(a.map_clock_ms-event.start)/50u),0,22,320,218);
 else picture_at(probe,id,frame,pose);
 // Original impact text/particles and the pen trail legitimately cover
 // source actor texels. Inspect the uncovered original actor, then check
 // the real command's separate cue/score/RNG contract below.
 std::vector<uint16_t> effects(ScreenW*ScreenH,0);std::vector<uint8_t> covered(ScreenW*ScreenH,0);
 Canvas effect{a};effect.pixels=effects.data();effect.coverage=covered.data();
 // WinRun reparents the player's dying creature above CombatVfxContainer.
 // Its opaque texels must match even where the lower FireBurst has ink;
 // excluding that lower layer would remove every Silent texel at age0.
 const bool terminal_player=id>=int(CharacterDeathAnimationFirst)&&id<int(CharacterDeathAnimationFirst+5)&&
  a.game.phase==Phase::Complete&&int32_t(a.map_clock_ms-a.game_over_presentation.opened)>=0;
 if(!terminal_player)architect_scene_effects(effect);
 unsigned ink=0,bad=0;for(int y=24;y<166;++y)for(int x=0;x<ScreenW;++x){const unsigned p=y*ScreenW+x;
  if(alpha[p]==255&&!covered[p]){++ink;bad+=a.pixels[p]!=expected[p];}}
 if(ink<minimum||bad)std::fprintf(stderr,"source sprite probe: clip=%d frame=%u anchor=%d,%d opaque=%u mismatch=%u\n",id,frame,pose.x,pose.y,ink,bad);
 CHECK(ink>=minimum&&!bad);CHECK(a.content.healthy());
}
static void visible_source_hit_pair(unsigned character,unsigned windup){
 if(!original_art)return;
 // Large source impacts can cover nearly every Architect texel. Compose
 // the independent source anchors and literal attack/hurt frames over the
 // original room, then add the retained effect layers. Compare the complete
 // composition at all opaque actor pixels instead of discarding covered ones.
 std::vector<uint16_t> expected(ScreenW*ScreenH,0);std::vector<uint8_t> alpha(ScreenW*ScreenH,0);
 Canvas scene{a};scene.pixels=expected.data();
 Canvas actors=scene;actors.coverage=alpha.data();
 const auto draw_pair=[&](Canvas&layer){
  picture_at(layer,attack_clips[character],sample(attack_clips[character],windup,false),source_pose(character));
  picture_at(layer,50,0,source_pose(character,true));
 };
 draw_pair(actors);
 // Coverage stores straight RGBA, not a composition over the existing
 // background. Repaint normally before a warp samples antialiased edges.
 scene.rect(0,0,ScreenW,ScreenH,Ink);
 scene.picture(a.content.world_image("events",original_event_key(OriginalEventId::TheArchitect)),0,22,320,218);
 draw_pair(scene);
 architect_scene_effects(scene);
 unsigned ink=0,bad=0;for(int y=24;y<166;++y)for(int x=0;x<ScreenW;++x){const unsigned p=y*ScreenW+x;
  if(alpha[p]==255){++ink;bad+=a.pixels[p]!=expected[p];}}
 if(ink<40||bad)std::fprintf(stderr,"source hit composition: character=%u windup=%u opaque=%u mismatch=%u\n",character,windup,ink,bad);
 CHECK(ink>=40&&!bad);CHECK(a.content.healthy());
}
static void death_scene(unsigned character,unsigned age){
 if(!original_art)return;
 // WinRun first opens/reparents the VisualOnly cast, then force-kills the
 // player. At exactly200ms FullBlackBackstop is opaque and UI is still
 // hidden. Compare the full actor band to exactly two original actors.
 // This catches a centered duplicate, an idle overlay and a retained Osty.
 CHECK(age==200&&a.game_over_presentation.black_alpha(a.map_clock_ms)==255);
 std::vector<uint16_t> expected(ScreenW*ScreenH,0);Canvas canvas{a};canvas.pixels=expected.data();
 const unsigned death_duration[]={2333,1867,2233,4167,2333},death_frames[]={71,57,68,126,71};
 const auto pose=source_pose(character);
 if(character==2||character==3){const int id=character==2?int(DefectDeathParticles):int(RegentDeathParticles);
  const auto clip=a.content.clip(id);CHECK(clip.count);const unsigned duration=character==2?2233u:5000u;
  picture_at(canvas,id,age*(clip.count-1)/duration,pose,presentation::Add);}
 picture_at(canvas,int(CharacterDeathAnimationFirst+character),age*(death_frames[character]-1)/death_duration[character],pose);
 // Architect's attack starts500/250ms before its FireBurst, then Wait(.5)
 // reaches WinRun. It is still within its original1500ms clip at +200.
 const unsigned enemy_age=(a.room_presentation.event.fast?750u:1000u)+age;
 picture_at(canvas,49,sample(49,enemy_age,false),source_pose(character,true));
 architect_scene_effects(canvas);
 unsigned bad=0,ink=0;for(int y=24;y<166;++y)for(int x=0;x<ScreenW;++x){const unsigned p=y*ScreenW+x;bad+=a.pixels[p]!=expected[p];ink+=expected[p]!=0;}
 if(bad)std::fprintf(stderr,"terminal two-actor band mismatch: character=%u age=%u pixels=%u\n",character,age,bad);
 CHECK(ink>100&&!bad);equal_save();
}
static void capture(const char*name){
 if(!captures||!original_art)return;
 const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
 CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[ScreenW*3];
 for(int y=0;y<ScreenH;++y){for(int x=0;x<ScreenW;++x){const uint16_t p=a.pixels[y*ScreenW+x];row[x*3]=uint8_t((p>>11)*255/31);row[x*3+1]=uint8_t(((p>>5)&63)*255/63);row[x*3+2]=uint8_t((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void write_file(const char*name,const uint8_t*bytes,size_t size){
 const auto path=std::string(captures)+"/"+name;FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);CHECK(std::fwrite(bytes,1,size,f)==size);CHECK(!std::fclose(f));
}
static void dialogue_text(){
 if(!original_art)return;
 char key[256]{},text[8192]{};CHECK(original_event_text_key(a.game,-1,true,key,sizeof key));
 // The source keeps this combat-event conversation in ancients.json,
 // unlike ordinary static event descriptions in events.json.
 CHECK(*a.content.world_string("ancients",key));
 a.original_event_text(-1,true,text,sizeof text);CHECK(*text);
}
static void read_dialogue_end(unsigned character){
 if(!original_art||!a.event_body_scroll.maximum)return;
 HitRegion body{};bool found=false;for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::EventScroll&&!a.hits[i].value&&a.hits[i].enabled){body=a.hits[i];found=true;break;}
 CHECK(found&&body.y>=166&&body.y+body.h<=201);
 a.pointer_move(body.x+body.w/2,body.y+body.h/2);a.pointer_button(true);a.pointer_button(false);a.pointer.visible=false;
 CHECK(a.event_body_focus);for(unsigned i=0;i<128&&a.event_body_scroll.position<a.event_body_scroll.maximum;++i){a.input(Key::Down);advance_room(33);}
 advance_room(250);CHECK(a.event_body_scroll.position==a.event_body_scroll.maximum);equal_save();
 char name[96];std::snprintf(name,sizeof name,"%s-dialogue-tail",names[character]);capture(name);
 a.input(Key::Confirm);CHECK(!a.event_body_focus&&a.game.phase==Phase::Event);equal_save();
}
static void export_entry(){
 if(!captures)return;
 scenario="production first Architect entry save and separate partial-history profile";
 CHECK(a.game.character==Character::Ironclad&&a.game.phase==Phase::Event&&a.game.original_events.values[0]==0);
 CHECK(validate(a.game)&&save(a.game,one));CHECK(load(direct,one.bytes,one.size)&&validate(direct)&&save(direct,two));
 CHECK(one.size==two.size&&!std::memcmp(one.bytes,two.bytes,one.size));write_file("architect-entry-ironclad.sav.tns",one.bytes,one.size);
 clear_ui_progress(a.progress);a.begin_progress(true);a.sync_progress();
 static uint8_t encoded[UiProgressCapacity],again[UiProgressCapacity];static UiProgress decoded;
 const size_t size=encode_ui_progress(a.progress,encoded,sizeof encoded);CHECK(size&&decode_ui_progress(encoded,size,decoded));
 const size_t copied=encode_ui_progress(decoded,again,sizeof again);CHECK(copied==size&&!std::memcmp(encoded,again,size));
 CHECK(decoded.current.partial&&!decoded.characters[0].wins&&!decoded.tutorial_flags);
 write_file("architect-entry-ironclad.progress.tns",encoded,size);equal_save();
 std::printf("Architect native-generated entry fixture exported: save=%zu progress=%zu revision=%u; prior room outcomes supplied, no console or ordinary victory claim\n",one.size,size,a.game.revision);std::fflush(stdout);
}
static void run_character(unsigned character,bool fast){
 scenario="real final-room source entry, animation lifecycle and canonical equivalence";
 architect_room(Character(character),fast,0,0);const uint32_t entered=a.map_clock_ms;a.render();
 const unsigned intro=character==1?(fast?3750u:3950u):2250u;
 CHECK(a.screen_motion.sequence_end(entered)==entered+intro);
 CHECK(a.room_presentation.architect.bound&&a.room_presentation.architect.opening&&a.room_presentation.architect.start==entered);
 CHECK(a.room_presentation.architect.plan.hits==(character==1?4u:0u));
 if(!character&&!fast)export_entry();
 static char stage[96];std::snprintf(stage,sizeof stage,"%s %s source actors and dialogue",names[character],fast?"Fast":"Normal");scenario=stage;
 dialogue_text();
 const auto pose=source_pose(character),enemy=source_pose(character,true);
 if(original_art){CHECK(a.content.world_clip("Architect","attack")==49&&a.content.world_clip("Architect","hurt")==50&&a.content.world_clip("Architect","idle_loop")==51);
  const auto f=architect_scene_frame(a);CHECK(f.player==idle_clips[character]&&f.architect==51&&f.player_loop&&f.architect_loop);
  visible_original(51,sample(51,f.architect_ms,true),enemy);visible_original(idle_clips[character],sample(idle_clips[character],entered,true),pose);}
 a.input(Key::Confirm);equal_save();CHECK(a.game.phase==Phase::Event);
 if(character==1&&original_art){
  until(entered+750);auto f=architect_scene_frame(a);CHECK(f.player==12&&!f.player_loop&&f.player_ms==0&&f.architect==51);
  until(entered+(fast?800u:850u));f=architect_scene_frame(a);CHECK(f.architect==50&&!f.architect_loop&&!f.architect_ms);
  visible_original(50,0,enemy);
 }
 until(entered+intro-1);CHECK(a.screen_motion.sequence_active(a.map_clock_ms));a.input(Key::Confirm);equal_save();
 until(entered+intro);CHECK(!a.screen_motion.sequence_active(a.map_clock_ms));
 for(unsigned i=0;i<200&&!a.room_presentation.event.option_ready(0,a.map_clock_ms);++i)advance_room(33);
 CHECK(a.room_presentation.event.option_ready(0,a.map_clock_ms));a.render();equal_save();repeat_render();
 if(original_art){const auto f=architect_scene_frame(a);CHECK(f.player==idle_clips[character]&&f.architect==51&&f.player_loop&&f.architect_loop);
  visible_original(51,sample(51,f.architect_ms,true),enemy);visible_original(idle_clips[character],sample(idle_clips[character],a.map_clock_ms,true),pose);}
 char name[128];if(!fast){std::snprintf(name,sizeof name,"%s-idle-dialogue",names[character]);capture(name);}
 if(!fast)read_dialogue_end(character);
 if(character==4&&original_art){
  // A prior combat pet value cannot add an entity to VisualOnly.Allies.
  const auto saved_hp=a.game.expansion.osty_hp,saved_max=a.game.expansion.osty_max_hp;
  const std::vector<uint16_t> before(a.pixels,a.pixels+ScreenW*ScreenH);
  a.game.expansion.osty_hp=a.game.expansion.osty_max_hp=77;a.render();CHECK(!std::memcmp(before.data(),a.pixels,sizeof a.pixels));
  a.game.expansion.osty_hp=saved_hp;a.game.expansion.osty_max_hp=saved_max;equal_save();
 }
 uint32_t began=0;for(unsigned page=0;page<5&&a.game.phase==Phase::Event;++page){
  for(unsigned i=0;i<200&&!a.room_presentation.event.option_ready(0,a.map_clock_ms);++i)advance_room(33);
  CHECK(a.room_presentation.event.option_ready(0,a.map_clock_ms));began=a.map_clock_ms;
  dialogue_text();
  direct=a.game;CHECK(original_event_choose(direct,0));a.cursor=0;a.input(Key::Confirm);a.render();equal_save();
 }
 CHECK(a.game.phase==Phase::Complete);
 const unsigned hits=character==1||character==4?4u:5u,windup=fast?50u:100u,step=fast?150u:200u;
 const unsigned last=windup+(hits-1)*step,architect_start=last+2000u,terminal=architect_start+(fast?750u:1000u);
 CHECK(a.game_over_presentation.opened==began+terminal);
 const auto&effects=a.room_presentation.architect;
 CHECK(effects.bound&&!effects.opening&&effects.start==began&&effects.plan.hits==hits);
 unsigned score_parts=0,seen_vfx=0;
 for(unsigned i=0;i<hits;++i){CHECK(effects.hits[i].amount>0);score_parts+=effects.hits[i].amount;
  CHECK(!(seen_vfx&(1u<<effects.hits[i].vfx)));seen_vfx|=1u<<effects.hits[i].vfx;}
 CHECK(score_parts>=effects.score);if(effects.score>=hits)CHECK(score_parts==effects.score);
 static constexpr unsigned source_lists[]={
  (1u<<ArchitectPresentation::Blunt)|(1u<<ArchitectPresentation::HeavyBlunt)|(1u<<ArchitectPresentation::Slash)|(1u<<ArchitectPresentation::BloodyImpact)|(1u<<ArchitectPresentation::RockShatter),
  (1u<<ArchitectPresentation::DaggerSpray)|(1u<<ArchitectPresentation::FlyingSlash)|(1u<<ArchitectPresentation::DramaticStab)|(1u<<ArchitectPresentation::DaggerThrow),
  (1u<<ArchitectPresentation::Lightning)|(1u<<ArchitectPresentation::Blunt)|(1u<<ArchitectPresentation::Scratch)|(1u<<ArchitectPresentation::Slash)|(1u<<ArchitectPresentation::HeavyBlunt),
  (1u<<ArchitectPresentation::StarryImpact)|(1u<<ArchitectPresentation::Blunt)|(1u<<ArchitectPresentation::Slash)|(1u<<ArchitectPresentation::HeavyBlunt)|(1u<<ArchitectPresentation::Lightning),
  (1u<<ArchitectPresentation::Thrash)|(1u<<ArchitectPresentation::HeavyBlunt)|(1u<<ArchitectPresentation::Slash)|(1u<<ArchitectPresentation::BloodyImpact)};
 CHECK(seen_vfx==source_lists[character]);
 if(original_art)for(unsigned i=0;i<hits;++i){
  until(began+windup+i*step);const auto f=architect_scene_frame(a);
  CHECK(f.player==attack_clips[character]&&!f.player_loop&&f.player_ms==windup);
  CHECK(f.architect==50&&!f.architect_loop&&f.architect_ms==0);
  visible_source_hit_pair(character,windup);equal_save();
  if(!character&&!fast&&i==1)capture("ironclad-repeat-hit");
 }
 until(began+last+733);if(original_art){const auto f=architect_scene_frame(a);CHECK(f.architect==51&&f.architect_loop);}
 until(began+architect_start);if(original_art){const auto f=architect_scene_frame(a);CHECK(f.architect==49&&!f.architect_loop&&!f.architect_ms);visible_original(49,0,enemy);}
 until(began+architect_start+300);if(!character&&!fast)capture("ironclad-architect-attack");
 until(began+terminal-1);CHECK(!a.game_over_presentation.initial_age(a.map_clock_ms)&&!a.game_over_presentation.black_alpha(a.map_clock_ms));equal_save();
 until(began+terminal);CHECK(a.game_over_presentation.initial_age(a.map_clock_ms)==0);visible_original(int(CharacterDeathAnimationFirst+character),0,pose);equal_save();
 if(!character&&!fast)capture("ironclad-death-origin");
 until(began+terminal+200);
 if(!fast){std::snprintf(name,sizeof name,"%s-death-200ms",names[character]);capture(name);}
 death_scene(character,200);repeat_render();
 until(began+architect_start+1500);if(original_art){const auto f=architect_scene_frame(a);CHECK(f.architect==51&&f.architect_loop);}
 const unsigned tail[]={2333,1867,2233,4167,2333};until(began+terminal+tail[character]+1);equal_save();
 // The Silent's1867ms die clip finishes before the independent1950ms
 // terminal UI intro. Completing an actor clip cannot enable Continue.
 if(tail[character]+1<1950)until(began+terminal+1950);
 CHECK(a.game_over_button_ready());a.input(Key::Confirm);a.render();CHECK(a.game_over_summary);equal_save();
 if(!character&&!fast){
  CHECK(save(a.game,architect_saved));const auto callbacks=a.callbacks;a.callbacks.load=load_architect;a.return_to_title();
  // Terminal checkpoints are deliberately excluded from ordinary Continue.
  // Older hosts can still persist them; exercise that production codec
  // reload directly, as the existing terminal compatibility gate does.
  CHECK(!a.has_save&&!a.load_saved_run());CHECK(load(a.game,architect_saved.bytes,architect_saved.size));
  a.title=false;a.reset_presentation();a.render();equal_save();const auto frame=architect_scene_frame(a);
  CHECK(!a.room_presentation.architect.bound);
  if(original_art)CHECK(frame.player==0&&frame.architect==51&&frame.player_loop&&frame.architect_loop);
  CHECK(!a.screen_motion.sequence_active(a.map_clock_ms)&&a.game_over_presentation.opened==a.map_clock_ms);
  visible_original(int(CharacterDeathAnimationFirst),0,pose);capture("ironclad-complete-reload");a.callbacks=callbacks;
 }
}
int main(int argc,char**argv){
 CHECK(argc>=1&&argc<=4);captures=argc>=3?argv[2]:nullptr;geometry();displacement_rows();
 const char*locale=argc==4?argv[3]:"eng";CHECK(!std::strcmp(locale,"eng")||!std::strcmp(locale,"zhs"));
 std::vector<uint8_t> common,language;SplitResourceSource split;
 if(argc>=2){common=read(std::string(argv[1])+"/nSpire2.common.tns");language=read(std::string(argv[1])+"/nSpire2."+locale+".tns");
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),!std::strcmp(locale,"zhs")?ResourceLanguage::Chinese:ResourceLanguage::English));
  CHECK(a.init(split.source(),{}));original_art=true;}
 for(unsigned fast=0;fast<2;++fast)for(unsigned character=0;character<5;++character)run_character(character,fast);
 std::printf("Architect visual UI12: %u checks passed; five characters x Normal/Fast, production event entry, canonical saves%s\n",checks,original_art?", original actor texels and terminal two-actor composition":" (asset-independent)");
}
