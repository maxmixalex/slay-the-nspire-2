// Display/observer correctness only. Firebird owns performance acceptance.
#include "../src/app.cpp"
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
using namespace spire;
static App a;
static State direct;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"combat secondary: %d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void canonical_equal(const State&left,const State&right){
 // Effect's byte after its uint8_t kind is ABI padding. Independent core
 // calls can leave different stack padding under -O0; it is not gameplay.
 // Check every other State byte, including inactive command slots and RNG.
 const auto*l=reinterpret_cast<const uint8_t*>(&left),*r=reinterpret_cast<const uint8_t*>(&right);
 bool equal=true;for(unsigned i=0;i<sizeof(State);++i)if(l[i]!=r[i]){
  const size_t offset=i-size_t(offsetof(State,effects));
  if(offset>=sizeof(left.effects)||offset%sizeof(Effect)!=1u)equal=false;
 }
 CHECK(equal);
 // Some synthetic observer fixtures intentionally omit their full world
 // context. When serializable, also require complete canonical equality.
 static SaveData one{},two{};const bool lsave=save(left,one),rsave=save(right,two);CHECK(lsave==rsave);
 if(lsave)CHECK(one.size==two.size&&!std::memcmp(one.bytes,two.bytes,one.size));
}
struct Notices {
 CoreOrbNotice values[32]{};unsigned count=0;
 static void receive(void*opaque,const State&,const CoreOrbNotice&notice){auto&v=*static_cast<Notices*>(opaque);CHECK(v.count<32);v.values[v.count++]=notice;}
};
static void fresh(){
 new_run(a.game,719,false,Character::Defect);auto&s=a.game;s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=1;
 s.relics=s.relic_count=s.relic_flash=0;s.energy=20;s.hp=s.max_hp=75;s.deck_count=1;s.deck[0]={DefectDefend,0};
 CHECK(encounter_setup(s,EncounterId::NibbitsNormal));for(auto&e:s.enemies)e.hp=e.max_hp=1000;
 s.expansion.orb_slots=3;s.expansion.orb_count=0;s.hand.count=s.draw.count=s.discard.count=s.exhaust.count=s.card_count=0;
 a.title=a.paused=a.help=a.library=a.character_select=a.settings_open=false;a.reset_presentation();
}
static void observers(){
 for(unsigned kind=0;kind<unsigned(DOrb::Count);++kind){
  fresh();auto&s=a.game;s.expansion.orb_count=1;s.defect.orb_kind[0]=uint8_t(kind);s.expansion.dark[0]=6;direct=s;
  Notices notices;CoreObserver binding;binding.context=&notices;binding.on_orb=Notices::receive;
  const auto old=set_core_observer(binding);queue_extension(s,XOp::DEvoke,0,2);resolve_pending(s);set_core_observer(old);
  queue_extension(direct,XOp::DEvoke,0,2);resolve_pending(direct);
  canonical_equal(s,direct);CHECK(notices.count==3&&!s.expansion.orb_count);
  CHECK(notices.values[1].event==CoreOrbNotice::Removed&&notices.values[0].event==CoreOrbNotice::Activation&&notices.values[2].event==CoreOrbNotice::Activation);
  CHECK(!notices.values[0].removed&&notices.values[2].removed);
  for(unsigned i=0;i<notices.count;++i){const auto&n=notices.values[i];CHECK(n.kind==kind&&n.slot==0&&n.slots==3&&n.evoke);if(n.event==CoreOrbNotice::Removed)continue;
   if(kind==unsigned(DOrb::Dark)||kind==unsigned(DOrb::Lightning))CHECK(n.targets&&!(n.targets&(n.targets-1u)));
   if(kind==unsigned(DOrb::Glass))CHECK(n.targets==(1u<<s.enemy_count)-1u);
   if(kind==unsigned(DOrb::Frost)||kind==unsigned(DOrb::Plasma))CHECK(!n.targets);
  }
  fresh();s.expansion.orb_count=1;s.defect.orb_kind[0]=uint8_t(kind);s.expansion.dark[0]=6;direct=s;notices={};
  const auto prior=set_core_observer(binding);queue_extension(s,XOp::DOrbPassive,0,-1);resolve_pending(s);set_core_observer(prior);
  queue_extension(direct,XOp::DOrbPassive,0,-1);resolve_pending(direct);
  canonical_equal(s,direct);CHECK(notices.count==1&&!notices.values[0].evoke&&s.expansion.orb_count==1);
 }
 // Gold-Plated Cables schedules its extra activation through WorldStage,
 // not a second DOrbPassive wrapper. The primitive observer must see both.
 fresh();init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicGoldPlatedCables));
 a.game.expansion.orb_count=1;a.game.defect.orb_kind[0]=uint8_t(DOrb::Frost);direct=a.game;
 Notices extra;CoreObserver binding;binding.context=&extra;binding.on_orb=Notices::receive;
 const auto previous=set_core_observer(binding);queue_extension(a.game,XOp::DOrbPassive,0,-1);resolve_pending(a.game);set_core_observer(previous);
 queue_extension(direct,XOp::DOrbPassive,0,-1);resolve_pending(direct);
 CHECK(extra.count==2&&!extra.values[0].evoke&&!extra.values[1].evoke);canonical_equal(a.game,direct);
 // A real scoped UI observer queues both casts even though only the second
 // changes OrbCount. The UI callback cannot consume any canonical RNG.
 fresh();a.game.expansion.orb_count=1;a.game.defect.orb_kind[0]=uint8_t(DOrb::Lightning);direct=a.game;
 a.begin_combat_feedback();{UiCoreObserver observer(a);queue_extension(a.game,XOp::DEvoke,0,2);resolve_pending(a.game);}
 queue_extension(direct,XOp::DEvoke,0,2);resolve_pending(direct);canonical_equal(a.game,direct);
 unsigned evoked=0,beams=0;for(unsigned i=0;i<a.combat_sparks.count;++i){evoked+=a.combat_sparks.bursts[i].kind==CombatSparkMotion::OrbEvoke;beams+=a.combat_sparks.bursts[i].kind==CombatSparkMotion::OrbBeam;}
 CHECK(evoked==2&&beams==2);
}
static CombatCardMotion::Cue transfer(Zone to){CombatCardMotion::Cue cue;cue.id=0;cue.from=uint8_t(Zone::Hand);cue.to=uint8_t(to);cue.card={Strike,0};return cue;}
static void cards(){
 fresh();auto&s=a.game;s.card_count=s.hand.count=1;s.hand.ids[0]=0;s.cards[0]={Strike,0};s.zones[0]=Zone::Hand;direct=s;
 CombatCardMotion m;m.begin(s,0);auto cue=transfer(Zone::Exhaust);m.apply(cue);m.set_hand({});CHECK(m.find(0)&&m.find(0)->mode==CombatCardMotion::Exhaust);
 m.advance(12);CHECK(m.find(0));m.advance(31);CHECK(!m.find(0));
 m.begin(s,0);cue=transfer(Zone::Power);cue.target_x=61;cue.target_y=106;m.apply(cue);m.set_hand({});
 CHECK(m.find(0)&&m.find(0)->mode==CombatCardMotion::Power);const unsigned duration=m.find(0)->duration;CHECK(duration>300&&duration<1500);
 m.advance(8);CHECK(m.find(0)->trail_count==6);m.advance((duration+32)/33);const auto*power=m.find(0);CHECK(power&&power->mode==CombatCardMotion::Tail);
 CHECK(CombatCardMotion::magnitude(power->x/1024-61)<=1&&CombatCardMotion::magnitude(power->y/1024-106)<=1);m.advance(100);CHECK(!m.find(0));
 m.begin(s,0);cue=transfer(Zone::Draw);cue.id=MaxCards;cue.from=uint8_t(Zone::Discard);m.apply(cue);CHECK(m.find(MaxCards)&&m.find(MaxCards)->mode==CombatCardMotion::Shuffle);
 m.advance(5);CHECK(m.find(MaxCards)->trail_count==5);m.advance(100);CHECK(!m.find(MaxCards));
 m.begin(s,0);m.set_hand({});m.find(0);cue=transfer(Zone::Hand);cue.from=CombatCardMotion::NewCard;m.apply(cue);
 CHECK(m.find(0)&&m.find(0)->mode==CombatCardMotion::Hand&&m.find(0)->generated&&m.find(0)->scale==0);
 CombatHand::Cards hand;hand.count=1;hand.ids[0]=0;m.set_hand(hand);m.advance(8);CHECK(m.find(0)->scale>0);CHECK(!std::memcmp(&s,&direct,sizeof s));
 CHECK(sizeof(CombatCardMotion)<=8192&&sizeof(CombatSparkMotion)<12288);
}
static void highlights(){
 fresh();auto&s=a.game;s.card_count=s.hand.count=1;s.hand.ids[0]=0;s.cards[0]={Strike,0};s.zones[0]=Zone::Hand;
 CHECK(hand_card_glow(s,{Strike,0},0,true)==1);CHECK(hand_card_glow(s,{Strike,0},0,false)==0);
 CHECK(hand_card_glow(s,{EventCardEnthralled,0},0,false)==3);CHECK(hand_card_glow(s,{Restlessness,0},0,true)==2);
 s.expansion.osty_hp=0;CHECK(hand_card_glow(s,{NecroFetch,0},0,true)==3);s.expansion.osty_hp=10;CHECK(hand_card_glow(s,{NecroFetch,0},0,true)==2);
 s.necro.fetch_played[0]=1;CHECK(hand_card_glow(s,{NecroFetch,0},0,true)==1);
 s.event_cards.plays_started=3;CHECK(hand_card_glow(s,{EventCardNormality,0},0,false)==3);
 HandCardHighlight h;CHECK(h.observe(1,false,0)==0);CHECK(h.observe(1,false,500)==4915);CHECK(h.observe(1,true,500)==4915);
 CHECK(h.observe(1,true,600)==9830);CHECK(h.observe(1,true,950)==4915);CHECK(h.observe(0,false,950)==4915);CHECK(h.observe(0,false,1450)==0);
 for(unsigned denominator:{74981u,100000u,149993u}){card_highlight::Fraction f(denominator);for(unsigned value=0;value<denominator;++value){const uint64_t q=uint64_t(value)*65536/denominator,q2=q*q/65536;CHECK(f.smooth(value)==q2*(3*65536-2*q)/65536);}}
}
static void source_orb_layout(){
 // Independent pinned NOrbManager formula: capacity controls both the
 // radius and the 125-degree arc, regardless of the number of filled orbs.
 fresh();const State before=a.game;CombatOrbPresentation layout;
 for(unsigned slots=1;slots<=10;++slots){
  layout.layout(slots,slots*1000);
  for(unsigned i=0;i<slots;++i){
   const double degrees=-150.0+(slots>1?125.0*i/(slots-1):0),r=225.0+75.0*(double(slots)-3.0)/7.0;
   const auto target=layout.position(i,slots*1000+450);
   CHECK(std::abs(target.x/1024.0+r*std::cos(degrees*3.141592653589793/180.0))<1.0);
   CHECK(std::abs(target.y/1024.0-r*std::sin(degrees*3.141592653589793/180.0))<1.0);
   const auto occupied=target;layout.model(i,unsigned(DOrb::Frost),slots*1000+500);layout.model(i,255,slots*1000+600);
   CHECK(layout.position(i,slots*1000+650).x==occupied.x&&layout.position(i,slots*1000+650).y==occupied.y);
  }
 }
 // A capacity change begins at the visible position; it never snaps an
 // existing orb to the origin while the new slots are being introduced.
 layout.layout(3,12000);const auto initial=layout.position(0,12500);layout.layout(6,12500);
 CHECK(layout.position(0,12500).x==initial.x&&layout.position(0,12500).y==initial.y);
 layout.layout(2,12700);const auto settled=layout.position(0,13200),source=CombatOrbPresentation::target(2,0);
 CHECK(settled.x==source.x&&settled.y==source.y);CHECK(!std::memcmp(&before,&a.game,sizeof before));
 // An actual dequeue moves surviving OBJECTS from their old arc positions,
 // preserves their birth times, fades exactly the removed object, and
 // introduces a new empty holder at the manager origin. Repeated evoke
 // activations above deliberately produce only one Removed event.
 CombatOrbPresentation moved;moved.layout(3,0);
 for(unsigned i=0;i<3;++i)moved.model(i,i,0);
 const auto first=moved.position(0,1000),second=moved.position(1,1000),third=moved.position(2,1000);
 moved.enqueue({1000,3,8,CoreOrbNotice::Removed,0,3,0},1000);
 CHECK(moved.items[0].kind==1&&moved.items[1].kind==2&&moved.items[2].kind==255);
 CHECK(moved.items[0].born==0&&moved.items[1].born==0&&moved.items[2].born==1000);
 CHECK(moved.position(0,1000).x==second.x&&moved.position(0,1000).y==second.y);
 CHECK(moved.position(1,1000).x==third.x&&moved.position(1,1000).y==third.y);
 CHECK(!moved.position(2,1000).x&&!moved.position(2,1000).y);
 CHECK(moved.activation(0,0,true,1000).x==first.x&&moved.ghosts[0].point.y==first.y);
 const auto middle=moved.position(0,1225);CHECK(std::abs(middle.x-(second.x+first.x)/2)<=1&&std::abs(middle.y-(second.y+first.y)/2)<=1);
 const auto old_holder=moved.position(2,1100),surviving=moved.position(0,1100);
 moved.enqueue({1100,0,0,CoreOrbNotice::Channeled,2,3,1},1100);
 CHECK(moved.position(2,1100).x==old_holder.x&&moved.position(2,1100).y==old_holder.y);
 CHECK(moved.position(0,1100).x==surviving.x&&moved.items[0].born==0&&moved.items[2].born==1100);
 moved.enqueue({1700,0,0,CoreOrbNotice::Capacity,0,5,0},1100);
 CHECK(moved.slots==3&&moved.count==1);const auto prediction=moved.activation(0,1,false,1800);
 CHECK(moved.slots==3&&moved.count==1);moved.advance(1800);CHECK(moved.slots==5&&moved.position(0,1800).x==prediction.x);
 CHECK(!moved.overflow&&sizeof moved<=2048);

}
static std::vector<uint8_t> read(const std::string&path){FILE*f=std::fopen(path.c_str(),"rb");CHECK(f);CHECK(!std::fseek(f,0,SEEK_END));const long size=std::ftell(f);CHECK(size>0);CHECK(!std::fseek(f,0,SEEK_SET));std::vector<uint8_t>data(size_t(size),0);CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));return data;}
static void actual_render(){
 fresh();auto&s=a.game;s.card_count=s.hand.count=1;s.hand.ids[0]=0;s.cards[0]={Strike,0};s.zones[0]=Zone::Hand;s.energy=10;a.animation=a.map_clock_ms=0;direct=s;
 auto fill=[](){for(auto&pixel:a.pixels)pixel=0x2124;};auto snapshot=[](){return std::vector<uint16_t>(a.pixels,a.pixels+320*240);};
 auto changed=[](const std::vector<uint16_t>&pixels){unsigned n=0;for(unsigned i=0;i<320*240;++i)n+=pixels[i]!=a.pixels[i];return n;};
 Canvas c{a};const auto pose=HandPose{160,150,HandFocusScale,0,1024};fill();draw_hand_tile(c,hand_tile(c,0),pose,false,255,0,true);const auto plain=snapshot();
 a.map_clock_ms=600;fill();draw_hand_tile(c,hand_tile(c,0),pose,false,255,0,true);CHECK(changed(plain)>0);
 // TextureRect KEEP_ASPECT_CENTERED must not stretch its square SDF to
 // the taller authored container. The glow stays within a 3px face inset.
 const int left=pose.cx-HandFaceW/2-3,right=pose.cx+(HandFaceW+1)/2+3;
 const int top=pose.cy-HandFaceH/2-3,bottom=pose.cy+(HandFaceH+1)/2+3;
 for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(x<left||x>=right||y<top||y>=bottom)
  CHECK(a.pixels[y*320+x]==plain[unsigned(y*320+x)]);
 fill();draw_hand_tile(c,hand_tile(c,0),pose,false,255,combat_exhaust_progress(250));CHECK(changed(plain)>100);
 a.combat_cards.begin(s,0);auto cue=transfer(Zone::Exhaust);a.combat_cards.apply(cue);a.combat_cards.set_hand({});a.animation=6;fill();const auto empty=snapshot();combat_card_secondary(c);CHECK(changed(empty)>0);
 for(unsigned kind=0;kind<unsigned(DOrb::Count);++kind){a.combat_sparks.reset();a.combat_sparks.secondary(0,80,90,CombatSparkMotion::OrbEvoke,kind,1);a.animation=5;fill();combat_secondary_sparks(c);CHECK(changed(empty)>0);}
 CHECK(!std::memcmp(&s,&direct,sizeof s));CHECK(a.content.healthy());
}
int main(int argc,char**argv){CHECK(argc==1||argc==2);observers();cards();highlights();source_orb_layout();
 if(argc==2){const std::string root=argv[1];auto common=read(root+"/nSpire2.common.tns"),language=read(root+"/nSpire2.eng.tns");SplitResourceSource split;
  CHECK(split.open(ResourceSource::memory(common.data(),common.size()),ResourceSource::memory(language.data(),language.size()),ResourceLanguage::English));CHECK(a.init(split.source(),{}));actual_render();}
 std::printf("combat secondary: %u checks passed; card metadata %zu B, burst pool %zu B%s\n",checks,sizeof(CombatCardMotion),sizeof(CombatSparkMotion),argc==2?", original assets rendered":"");
}
