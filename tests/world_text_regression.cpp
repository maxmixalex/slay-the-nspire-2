#include "app.hpp"
#include "content_fixture.hpp"
#include "original_events.hpp"
#include "rich_text.hpp"
#include "intent_hover_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static bool actual_pack;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"world text: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
struct TextIndex{const char*key;uint16_t index;};
static const TextIndex indexes[]={
#include "../src/world_text_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=uint8_t(value>>(i*8));}
static std::vector<uint8_t> synthetic_profile(){
 // Binding fixtures use the exact format selectors, with neutral prose and
 // synthetic art. The optional private pack exercises the original locale.
 constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count;
 constexpr unsigned stringdir=40+images*16,clipdir=stringdir+ContentStrings*8;
 constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
 std::vector<uint8_t>b(payload+82*60*2);put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,24,ContentStrings);put(b,28,AnimationKinds);put(b,32,CardKinds);
 for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
 std::vector<std::string>strings(ContentStrings,"SYNTHETIC");
 for(unsigned i=0;i<CardKinds;i++){strings[i*3]=definitions[i].key;strings[i*3+1]=strings[i*3+2]="CARD";}
 for(const auto&entry:indexes){std::string key=entry.key;std::string text=key;
  if(key=="relics.DUSTY_TOME.description"||key=="relics.DUSTY_TOME.eventDescription")text="GAIN {AncientCard.StringValue:cond:[gold]{}+[/gold]|GENERIC}";
  if(key=="relics.ARCHAIC_TOOTH.description"||key=="relics.ARCHAIC_TOOTH.eventDescription")text="CHANGE {StarterCard.StringValue:cond:[gold]{StarterCard}[/gold] TO [gold]{AncientCard}[/gold]|GENERIC}";
  if(key=="relics.TOUCH_OF_OROBAS.description"||key=="relics.TOUCH_OF_OROBAS.eventDescription")text="REPLACE {StarterRelic.StringValue:cond:[gold]{StarterRelic}[/gold] TO [gold]{UpgradedRelic}[/gold]|GENERIC}";
  if(key=="relics.SEA_GLASS.description")text="CHOOSE {Cards} {Character.StringValue:cond:[gold]{}[/gold]|GENERIC}";
  if(key=="relics.PAELS_TOOTH.description")text="REMOVE {Cards}{CardTitles.StringValue:cond:\n\nLIST\n{}|}";
  if(key=="ancients.PAEL.talk.firstVisitEver.0-0.ancient")text="[thinky_dots]READABLE\n[aqua]PAEL[/aqua][/thinky_dots]";
  if(key=="events.COLOSSAL_FLOWER.pages.INITIAL.description")text="TEXT[green]TEXT[/green]TEXT[red][jitter]TEXT[/jitter][/red]TEXT\n\nTEXT[sine][rainbow freq=0.3 sat=0.8 val=1]COLOR[/rainbow][/sine]TEXT[aqua]TEXT[/aqua]TEXT[red]TEXT[/red]TEXT\n\nTEXT[gold]TEXT[/gold]TEXT\n";
  if(key=="powers.SHRINK_POWER.smartDescription")text="{ApplierName.StringValue:cond:{} ACTIVE|{OwnerName}} {Amount:cond:==1?TURN|>1?[blue]{}[/blue] TURNS|}";
  if(key=="powers.NIGHTMARE_POWER.smartDescription")text="{Amount} COPIES OF {Card}";
  if(key=="intents.ATTACK.description")text="{OwnerName}: [red]{Damage}[/red] x {Repeat}";
  if(key=="intents.STATUS.description")text="CARDS {CardCount}";
  if(key=="events.CRYSTAL_SPHERE.minigame.divinationsRemain")text="REMAINING {Count}";
  if(key=="events.CRYSTAL_SPHERE.pages.INITIAL.description")text="UNRELATED {Count}";
  if(key=="relics.FAKE_ANCHOR.description")text="{Unbound.StringValue:cond:MUST NOT|DISAPPEAR}";
  CHECK(entry.index<strings.size());strings[entry.index]=text;
 }
 for(unsigned i=0;i<ContentStrings;i++){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
 for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
 for(unsigned i=0;i<CardKinds;i++)for(unsigned j=0;j<4;j++)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
 for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
 put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static void reset(unsigned character){new_run(a.game,0x6d328,false,Character(character));init_world_relics(a.game);a.game.phase=Phase::Map;a.title=a.library=false;CHECK(validate(a.game));}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void resolved(const char*text){CHECK(text&&*text);if(std::strchr(text,'{'))std::fprintf(stderr,"unresolved: %s\n",text);CHECK(!std::strchr(text,'{')&&!std::strchr(text,'}'));}
static std::string description(RelicId id,bool event=false,int position=-1){char text[4096]{};
 if(event){char key[128]{};std::snprintf(key,sizeof key,"%s.eventDescription",world_relic_key(id));a.localized_text("relics",key,text,sizeof text,id);}
 else a.relic_description(id,text,sizeof text,position);
 resolved(text);return text;
}
static std::string bound_title(RelicId id,const char*name,int position=-1){OriginalTextReference ref;CHECK(world_relic_description_reference(a.game,id,name,ref,position));
 if(ref.kind==OriginalTextReferenceKind::Card){Card card=card_from_token(ref.model);std::string title=a.content.card_string(card.kind);if(card.upgraded)title+='+';return title;}
 if(ref.kind==OriginalTextReferenceKind::Relic)return a.relic_title(RelicId(ref.model));
 CHECK(ref.kind==OriginalTextReferenceKind::Localized);return a.content.world_string(ref.table,ref.key);
}
static int owned(RelicId id){for(unsigned i=0;i<a.game.world_relics.count;i++)if(a.game.world_relics.entries[i].id==id)return int(i);CHECK(false);return -1;}
static void prepared_and_owned(){
 struct Sample{RelicId relic;const char*first,*second;};
 const Sample samples[]={{WorldRelicDustyTome,"AncientCard",nullptr},{WorldRelicArchaicTooth,"StarterCard","AncientCard"},{WorldRelicTouchOfOrobas,"StarterRelic","UpgradedRelic"},{WorldRelicSeaGlass,"Character",nullptr}};
 const CardKind starters[]={Bash,Neutralize,DefectDualcast,RegentFallingStar,NecroUnleash};
 for(unsigned character=0;character<5;character++)for(const auto&sample:samples)for(unsigned upgraded=0;upgraded<(sample.relic==WorldRelicArchaicTooth?2u:1u);upgraded++){
  reset(character);snapshot();std::string generic=description(sample.relic);if(!actual_pack)CHECK(generic.find("GENERIC")!=std::string::npos);unchanged();
  if(upgraded){bool found=false;for(unsigned i=0;i<a.game.deck_count;i++)if(a.game.deck[i].kind==starters[character]){a.game.deck[i].upgraded=1;found=true;}CHECK(found);}
  CHECK(world_relic_setup_offer(a.game,sample.relic,sample.relic==WorldRelicSeaGlass?int((character+1)%5):-1));
  const std::string first=bound_title(sample.relic,sample.first),second=sample.second?bound_title(sample.relic,sample.second):"";CHECK(!first.empty());
  snapshot();std::string shown=description(sample.relic);CHECK(shown.find(first)!=std::string::npos);if(sample.second)CHECK(shown.find(second)!=std::string::npos);
  // The synthetic Chinese-shaped template explicitly appends '+'. Original
  // languages own that punctuation; English does not append the same suffix.
  if(sample.relic==WorldRelicDustyTome&&!actual_pack)CHECK(shown.find(first+"+")!=std::string::npos);
  if(sample.relic!=WorldRelicSeaGlass){std::string event=description(sample.relic,true);CHECK(event.find(first)!=std::string::npos);if(sample.second)CHECK(event.find(second)!=std::string::npos);}
  unchanged();CHECK(load(a.game,before.bytes,before.size));CHECK(description(sample.relic)==shown);unchanged();
  if(sample.relic==WorldRelicSeaGlass)continue; // Its acquisition opens card rewards.
  CHECK(gain_world_relic(a.game,sample.relic));resolve_pending(a.game);CHECK(!world_relic_pending(a.game)&&a.game.phase!=Phase::Error);
  int position=owned(sample.relic);snapshot();CHECK(bound_title(sample.relic,sample.first,position)==first);if(sample.second)CHECK(bound_title(sample.relic,sample.second,position)==second);
  CHECK(description(sample.relic,false,position)==shown);unchanged();CHECK(load(a.game,before.bytes,before.size));CHECK(description(sample.relic,false,position)==shown);unchanged();
 }
}
static void other_original_forms(){
 reset(0);snapshot();description(WorldRelicPaelsTooth);unchanged();char text[4096]{};
 for(int amount:{0,1,2,-1}){a.localized_text("powers","SHRINK_POWER.smartDescription",text,sizeof text,int(EnemyPower::Shrink),0,amount);resolved(text);CHECK(!std::strstr(text,">1?")&&!std::strstr(text,"==1?"));unchanged();}
 a.localized_text("ancients","PAEL.talk.firstVisitEver.0-0.ancient",text,sizeof text);resolved(text);CHECK(std::strstr(text,"[thinky_dots]"));
 RichReader reader(text);RichChar ch{};unsigned count=0;while(reader.next(ch)){CHECK(ch.codepoint!='['&&ch.style.thinky_dots);++count;}CHECK(count>0);unchanged();
 if(!actual_pack){a.relic_description(WorldRelicFakeAnchor,text,sizeof text);CHECK(!std::strcmp(text,"{Unbound.StringValue:cond:MUST NOT|DISAPPEAR}"));unchanged();}
}
static void nightmare_snapshot_names(){
 reset(1);auto&silent=a.game.silent;
 for(unsigned slot:{2u,5u}){
  silent.nightmare_active[slot]=1;silent.power_order[silent.power_count++]=uint8_t(SPowerCount+slot);
  silent.nightmares[slot].card={slot==2?Strike:Bash,uint8_t(slot==5)};
 }
 snapshot();char text[1024]{};
 for(unsigned slot:{2u,5u}){
  a.localized_text("powers","NIGHTMARE_POWER.smartDescription",text,sizeof text,int(slot),0,3);resolved(text);
  const Card card=silent.nightmares[slot].card;std::string title=a.content.card_string(card.kind);if(card.upgraded)title+='+';
  CHECK(std::strstr(text,title.c_str())&&std::strstr(text,"3"));unchanged();
 }
 a.localized_text("powers","NIGHTMARE_POWER.smartDescription",text,sizeof text,0,0,3);
 CHECK(std::strstr(text,"{Card}"));unchanged();
}
static void canonical_blade_context(){
 if(a.content.version()<LocaleContentVersion)return;
 reset(3);enter_combat(a.game);CHECK(encounter_setup(a.game,EncounterId::NibbitsWeak));
 auto&r=a.game.regent;const RPower powers[]={RPower::Parry,RPower::SeekingEdge,RPower::SwordSage};const int amounts[]={17,1,2};
 for(unsigned i=0;i<3;i++){r.powers[unsigned(powers[i])]=amounts[i];r.power_order[r.power_count++]=uint8_t(powers[i]);}
 snapshot();char catalog[4096]{},detached[4096]{},live[4096]{};const Card card{SovereignBlade,0};const EventCardFacts canonical{};
 a.library=true;a.card_description(card,0,catalog,sizeof catalog);resolved(catalog);unchanged();
 a.library=false;a.card_description(card,0,detached,sizeof detached,-1,-1,&canonical);resolved(detached);CHECK(!std::strcmp(catalog,detached));unchanged();
 a.card_description(card,0,live,sizeof live);resolved(live);CHECK(std::strcmp(catalog,live));
 CHECK(std::strstr(live,a.content.world_title("static_hover_tips","REPLAY_DYNAMIC")));unchanged();
}
static void original_resource_labels(){
 if(a.content.version()<LocaleContentVersion)return;
 reset(3);snapshot();
 // The pinned star formatter repeats sprite-font icons. Its standalone
 // binding uses the same sprite; localized prose still names the resource.
 const std::string star=InlineStarText;
 const std::string energy=InlineEnergyText;
 CHECK(!star.empty()&&!energy.empty());
 CHECK(description(WorldRelicDivineRight).find(star)!=std::string::npos);unchanged();
 struct Sample{const char*table,*key;const std::string*label;};
 const Sample samples[]={{"potions","STAR_POTION.description",&star},{"potions","ENERGY_POTION.description",&energy},{"static_hover_tips","STAR_COUNT.description",&star}};
 for(const auto&sample:samples){char text[2048]{};a.localized_text(sample.table,sample.key,text,sizeof text);resolved(text);CHECK(std::strstr(text,sample.label->c_str()));unchanged();}
}
static void crystal_board_count(){
 reset(0);CHECK(original_event_begin(a.game,OriginalEventId::CrystalSphere,0x71ef23,0));
 CHECK(original_event_choose(a.game,1));CHECK(a.game.phase==Phase::OriginalEventChoice);
 const auto remaining=a.game.original_events.crystal_charges;CHECK(remaining>0);char text[256]{};
 for(unsigned charges=0;charges<=remaining;++charges){
  a.game.original_events.crystal_charges=uint8_t(charges);snapshot();
  a.localized_text("events","CRYSTAL_SPHERE.minigame.divinationsRemain",text,sizeof text);
  resolved(text);char number[16]{};std::snprintf(number,sizeof number,"%u",charges);CHECK(std::strstr(text,number));
  unchanged();CHECK(load(a.game,before.bytes,before.size));
  char restored[256]{};a.localized_text("events","CRYSTAL_SPHERE.minigame.divinationsRemain",restored,sizeof restored);
  CHECK(!std::strcmp(text,restored));unchanged();
 }
 if(!actual_pack){
  a.localized_text("events","CRYSTAL_SPHERE.pages.INITIAL.description",text,sizeof text);
  CHECK(std::strstr(text,"{Count}"));
  reset(0);a.localized_text("events","CRYSTAL_SPHERE.minigame.divinationsRemain",text,sizeof text);
  CHECK(std::strstr(text,"{Count}"));
 }
}
static void colossal_flower(){
 reset(0);CHECK(original_event_begin(a.game,OriginalEventId::ColossalFlower,0x71ef23,0));a.reset_presentation();a.pointer.visible=false;
 snapshot();char text[8192]{};a.original_event_text(-1,true,text,sizeof text);resolved(text);
 CHECK(!std::strcmp(text,a.content.world_string("events","COLOSSAL_FLOWER.pages.INITIAL.description")));
 CHECK(std::strstr(text,"[sine][rainbow freq=0.3 sat=0.8 val=1]")&&std::strstr(text,"[/rainbow][/sine]"));
 RichReader reader(text);RichChar ch{};unsigned colored=0,row=0,x=0,first_row=0;bool finished=false;
 while(reader.next(ch)){
  CHECK(ch.codepoint!='['&&ch.codepoint!=']');auto glyph=a.content.glyph(ch.codepoint,10,rich_font_style(ch));unsigned advance=glyph.advance;
  if(ch.codepoint=='\n'||(x&&x+advance>164)){x=0;++row;if(ch.codepoint=='\n')continue;}
  if(ch.style.rainbow){CHECK(!finished&&ch.style.wave);CHECK(ch.style.rainbow_parameters.frequency==300&&ch.style.rainbow_parameters.saturation==800&&ch.style.rainbow_parameters.value==1000&&ch.style.rainbow_parameters.speed==1000);if(!colored)first_row=row;++colored;}
  else if(colored)finished=true;
  x+=advance;
 }
 CHECK(colored>0&&finished);unchanged();CHECK(load(a.game,before.bytes,before.size));char restored[8192]{};a.original_event_text(-1,true,restored,sizeof restored);CHECK(!std::strcmp(text,restored));unchanged();
 if(actual_pack){
  // Finish the original title/body/option entrance and the focus tween
  // before isolating rainbow time. Hold the unrelated sine/jitter frame.
  // freq=.3 must return to exactly the same colors after three turns / 10s.
  a.animation=0;a.map_clock_ms=0;a.scroll=int(first_row);a.render();
  a.map_clock_ms=10000;a.render();a.map_clock_ms=20000;a.render();unchanged();
  HitRegion story{};for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::UiAction&&a.hits[i].value==int(UiAction::ReadEvent))story=a.hits[i];
  CHECK(story.x==153&&story.y==54&&story.w==152&&story.h>=53&&story.y+story.h<221);
  std::vector<uint16_t>first(a.pixels,a.pixels+ScreenW*ScreenH);
  a.map_clock_ms=21000;a.render();unsigned changes=0;
  for(unsigned i=0;i<first.size();i++)if(first[i]!=a.pixels[i]){CHECK(int(i%ScreenW)>=story.x&&int(i%ScreenW)<story.x+story.w&&int(i/ScreenW)>=story.y&&int(i/ScreenW)<story.y+story.h);++changes;}
  CHECK(changes>0);unchanged();a.map_clock_ms=30000;a.render();CHECK(!std::memcmp(first.data(),a.pixels,first.size()*sizeof(uint16_t)));unchanged();
  // An opaque covering screen removes this label from the rendered tree.
  // Reopening the same body creates a fresh ItemFX origin, even though its
  // text and the global presentation time have not returned to zero.
  a.library=true;a.map_clock_ms=31000;a.render();a.library=false;
  a.map_clock_ms=32000;a.render();CHECK(!std::memcmp(first.data(),a.pixels,first.size()*sizeof(uint16_t)));unchanged();
  a.map_clock_ms=33000;a.render();CHECK(std::memcmp(first.data(),a.pixels,first.size()*sizeof(uint16_t)));unchanged();
 }
}
static std::string attack_description(int slot){
 char text[1024]{};a.localized_text("intents","ATTACK.description",text,sizeof text,slot,slot+1);resolved(text);return text;
}
static bool intent_panel(){
 return intent_hover_panel(a);
}
static void deferred_intent_bindings(){
 struct Sample{EnemyId kind;const char*key;};
 for(const Sample sample: {Sample{EnemyId::Noisebot,"STATUS.description"},Sample{EnemyId::Byrdonis,"ATTACK.description"}}){
  reset(0);enter_combat(a.game);CHECK(encounter_setup(a.game,EncounterId::NibbitsWeak));
  auto&model=a.game.enemy_world.models[0];model.kind=sample.kind;model.node=0;
  a.reset_presentation();a.begin_combat_feedback();CHECK(a.combat_feedback.active);
  const auto shown=a.combat_intent(0);CHECK(sample.kind==EnemyId::Noisebot?shown.status_cards==2:shown.hits==3);
  char original[1024]{},retained[1024]{},current[1024]{};snapshot();
  a.localized_text("intents",sample.key,original,sizeof original,0,1);resolved(original);unchanged();
  // Use real model intents to distinguish both status-card count and hit
  // repetition from their already advanced canonical values.
  model.kind=EnemyId::Nibbit;model.node=0;snapshot();
  a.localized_text("intents",sample.key,retained,sizeof retained,0,1);resolved(retained);
  CHECK(!std::strcmp(original,retained));unchanged();
  a.combat_feedback.reset();a.localized_text("intents",sample.key,current,sizeof current,0,1);resolved(current);
  CHECK(std::strcmp(original,current));unchanged();
 }
}
static void deferred_intent_text(){
 for(unsigned fast=0;fast<2;++fast)for(unsigned pending=0;pending<2;++pending){
  reset(0);enter_combat(a.game);CHECK(encounter_setup(a.game,EncounterId::NibbitsNormal));
  a.game.hp=a.game.max_hp=999;a.game.enemy_world.models[0].node=0;a.game.enemy_world.models[1].node=1;
  if(pending){init_world_relics(a.game);CHECK(gain_world_relic(a.game,WorldRelicToastyMittens));resolve_pending(a.game);}
  a.fast_mode=fast!=0;a.reset_presentation();a.pointer.visible=false;a.render();
  for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}
  CHECK(!a.combat_preparation.active);const auto first=attack_description(0),second=attack_description(1);
  a.input(Key::EndTurn);CHECK(a.combat_feedback.active&&a.combat_feedback.finished==!pending);
  CHECK(a.game.phase==(pending?Phase::WorldRelicChoice:Phase::Combat));
  snapshot();CHECK(attack_description(0)==first&&attack_description(1)==second);unchanged();
  // The real EndTurn has advanced both moves. A hover must remain usable
  // during the banner, retaining the same text as the still-visible icon.
  a.render();bool found=false;
  for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==HitKind::EnemyIntent&&a.hits[i].value==0){
   a.pointer.x=a.hits[i].x+a.hits[i].w/2;a.pointer.y=a.hits[i].y+a.hits[i].h/2;found=true;break;
  }
  CHECK(found);a.pointer.visible=true;a.render();CHECK(intent_panel());unchanged();
  const auto hide=a.combat_feedback.hide_at[0],done=a.combat_feedback.end_at;
  CHECK(hide>a.animation);a.animation=hide-1;a.render();CHECK(intent_panel()&&attack_description(0)==first);unchanged();
  if(pending){
   // The canonical relic-choice phase must not suppress a tooltip while
   // the display still shows the preceding battle and its original intent.
   CHECK(!a.combat_choice_ready());a.animation=a.combat_feedback.choice_at;a.render();
   CHECK(a.combat_choice_ready()&&!intent_panel());unchanged();a.pointer.visible=false;continue;
  }
  HoverRect rect{};CHECK(intent_hover_panel(a,&rect));std::vector<uint16_t> title;
  const int title_x=rect.x+4,title_y=rect.y+3,title_height=a.content.font_size(5,5);
  for(int y=title_y;y<title_y+title_height;++y)for(int x=title_x;x<title_x+56;++x)title.push_back(a.pixels[y*ScreenW+x]);
  // Reusing a canonical slot cannot rename the retained actor. This fixture
  // changes no displayed cue and keeps the complete saved state valid.
  a.game.enemy_world.models[0].kind=EnemyId::LouseProgenitor;
  a.game.enemy_world.models[0].combat_id=a.game.enemy_world.next_creature_id++;
  snapshot();CHECK(attack_description(0)==first);a.render();CHECK(intent_panel());unchanged();
  size_t title_at=0;for(int y=title_y;y<title_y+title_height;++y)for(int x=title_x;x<title_x+56;++x)CHECK(a.pixels[y*ScreenW+x]==title[title_at++]);
  a.animation=hide;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)==255&&intent_panel());unchanged();
  a.animation=hide+12;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)==255&&intent_panel());unchanged();
  a.animation=hide+20;a.render();CHECK(a.combat_intent(0).flags&&a.combat_intent_alpha(0)>0&&a.combat_intent_alpha(0)<255&&intent_panel());unchanged();
  CHECK(done>hide+28);a.animation=hide+28;a.render();CHECK(!a.combat_intent(0).flags&&!intent_panel());unchanged();
  a.animation=done;a.render();CHECK(!a.combat_feedback.active);CHECK(attack_description(0)!=first);unchanged();
  a.pointer.visible=false;
 }
}
int main(int argc,char**argv){std::vector<uint8_t>data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));actual_pack=true;}else data=synthetic_profile();
 CHECK(a.init(data.data(),data.size(),{}));prepared_and_owned();other_original_forms();nightmare_snapshot_names();canonical_blade_context();original_resource_labels();crystal_board_count();colossal_flower();deferred_intent_bindings();deferred_intent_text();std::printf("world text: %u checks passed (%s)\n",checks,actual_pack?"private original pack":"synthetic bindings");
}
