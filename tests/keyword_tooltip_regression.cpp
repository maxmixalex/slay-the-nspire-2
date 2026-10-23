#include "app.hpp"
#include "content_fixture.hpp"
#include "combat_layout.hpp"
#include "world_card_factory.hpp"
#include "original_events.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks;
static bool actual;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"keyword tooltip: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
struct TextIndex{const char*key;uint16_t index;};
static const TextIndex indexes[]={
#include "../src/world_text_index.inc"
};
static const TextIndex native_indexes[]={
#include "../src/native_ui_text_index.inc"
};
static void put(std::vector<uint8_t>&b,unsigned at,unsigned n,unsigned bytes=4){for(unsigned i=0;i<bytes;++i)b[at+i]=uint8_t(n>>(8*i));}
static uint32_t crc(const std::vector<uint8_t>&b){uint32_t c=~0u;for(uint8_t v:b){c^=v;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int32_t(c&1)));}return ~c;}
static unsigned group(std::vector<uint8_t>&b,const std::vector<uint8_t>&raw,unsigned type,unsigned id){
    if(b.size()%2)b.push_back(0);const unsigned at=unsigned(b.size());b.resize(at+32);
    const unsigned fields[]={0x31524754,type,id,unsigned(raw.size()),unsigned(raw.size())+4,crc(raw),1,0};
    for(unsigned i=0;i<8;++i)put(b,at+i*4,fields[i]);
    b.push_back(1);b.push_back(uint8_t((raw.size()-1)>>8));b.push_back(uint8_t(raw.size()-1));
    b.insert(b.end(),raw.begin(),raw.end());b.push_back(0);return at;
}
static std::vector<uint8_t> fixture(){
    constexpr unsigned images=NativeUiAuxiliaryCount,stringdir=40+images*16,clipdir=stringdir+NativeUiStringCount*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,base=uidir+NativeUiAuxiliaryCount*4,payload=base+96;
    std::vector<uint8_t>b(payload+82*60*2);put(b,0,0x3250534e);put(b,4,NativeUiContentVersion);put(b,8,0x41cef1ea);
    put(b,16,images);put(b,24,NativeUiStringCount);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;++i){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    std::vector<std::string> strings(NativeUiStringCount,"SYNTHETIC");
    for(unsigned i=0;i<CardKinds;++i){strings[i*3]=definitions[i].key;strings[i*3+1]=strings[i*3+2]="Card [gold]Keyword[/gold].\nA readable preview.";}
    for(const auto&e:indexes){std::string key=e.key;strings[e.index]=key.find(".title")!=std::string::npos?key.substr(key.find('.')+1):"Synthetic explanation.\nEvery line remains accessible.\nNo rule is inferred from a colored word.";
        if(key=="static_hover_tips.SUMMON_DYNAMIC.title")strings[e.index]="Summon {Summon}";
        if(key=="static_hover_tips.SUMMON_DYNAMIC.description")strings[e.index]="Summon amount [blue]{Summon}[/blue].";
        if(key=="static_hover_tips.REPLAY_DYNAMIC.description")strings[e.index]="Replay [blue]{Times}[/blue] times.";
        if(key=="card_keywords.EXHAUST.description")strings[e.index]="One\nTwo\nThree\nFour\nFive\nSix\nSeven\nEight\nNine\nTen\nEleven\nTwelve\nThirteen\nFourteen\nFifteen\nSixteen\nSeventeen";
    }
    for(const auto&e:native_indexes){std::string key=e.key;strings[e.index]=key.find(".title")!=std::string::npos?key:"Synthetic orb explanation.";}
    for(unsigned i=0;i<NativeUiStringCount;++i){put(b,stringdir+i*8,unsigned(b.size()));put(b,stringdir+i*8+4,unsigned(strings[i].size()));b.insert(b.end(),strings[i].begin(),strings[i].end());b.push_back(0);}
    for(unsigned i=0;i<AnimationKinds;++i){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;++i)for(unsigned j=0;j<4;++j)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<NativeUiAuxiliaryCount;++i)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    // One unused auxiliary image exercises a valid v16 tile directory. Card
    // pictures remain 82x60; the appended text IDs never alias a card string.
    const unsigned fields[]={0x314c4954,1,16,14,1,1,1,1,base+80,base+64,770,85,524288,617323,0,0};
    for(unsigned i=0;i<16;++i)put(b,base+i*4,fields[i]);
    std::vector<uint8_t>tile(770),record(18);tile[0]=tile[1]=1;put(record,0,2);put(record,4,4);put(record,8,2);
    const unsigned tiles=group(b,tile,1,0),indexes_at=group(b,record,2,1);
    put(b,base+64,tiles);put(b,base+68,32+unsigned(tile.size())+4);put(b,base+72,indexes_at);put(b,base+76,32+unsigned(record.size())+4);
    put(b,base+80,base);put(b,base+92,unsigned(record.size()));const unsigned image_at=40+(images-1)*16;
    put(b,image_at,base+80);put(b,image_at+4,1,2);put(b,image_at+6,1,2);put(b,image_at+8,6);put(b,image_at+12,16);
    put(b,12,unsigned(b.size()));uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();++i){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static void setup(){
    new_run(a.game,73,false);auto&s=a.game;s.phase=Phase::Combat;s.current_node=0;s.floor=s.turn=s.enemy_count=1;s.energy=99;
    s.deck_count=s.card_count=s.hand.count=0;s.enemies[0]={46,46,0,0,0,0};
    const CardKind cards[]={Bash,Defend,BladeDance,DefectRainbow,RegentBigBang};
    for(auto kind:cards){int id=s.card_count++;s.deck[s.deck_count++]={kind,0};s.cards[id]={kind,0};s.zones[id]=Zone::Hand;s.hand.ids[s.hand.count++]=uint16_t(id);}
    a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=a.map_preview.open=false;a.frontend_confirm=0;a.reset_presentation();a.pointer.visible=false;
    CHECK(validate(s));CHECK(save(s,before));const unsigned start_animation=a.animation;
    a.render();unsigned loading_steps=0;
    // Real compressed actor frames enter the bounded preparation screen;
    // direct synthetic images are already cached. Test the same settled
    // hand in both cases before looking up pointer/keyboard hit regions.
    while(a.combat_preparation.active&&loading_steps++<1024){a.advance(33);a.render();CHECK(a.animation==start_animation);}
    CHECK(!a.combat_preparation.active&&a.content.healthy());
    CHECK(save(s,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
}
static void snapshot(){CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static HoverTipList tips(Card card,int id=-1,const EventCardFacts*facts=nullptr){HoverTipList result{};collect_card_hover_tips(a,card,result,id,-1,facts);CHECK(!result.overflow);return result;}
static HitRegion hit(HitKind kind,int value){for(int i=0;i<a.hit_count;++i)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];CHECK(false);return {};}
static void capture(const char*name){
    if(!screenshots)return;std::string path=std::string(screenshots)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
    for(auto p:a.pixels){unsigned char rgb[3]={uint8_t((p>>11)*255/31),uint8_t(((p>>5)&63)*255/63),uint8_t((p&31)*255/31)};CHECK(std::fwrite(rgb,1,3,f)==3);}CHECK(!std::fclose(f));
}
static void associations(){
    setup();snapshot();auto b=tips({Bash,0});CHECK(b.count==1&&b.items[0].kind==HoverTipKind::Power&&hover_key_equal(b.items[0].key,"VULNERABLE_POWER")&&b.items[0].debuff);
    b=tips({Defend,0});CHECK(b.count==1&&b.find(HoverTipKind::Static,"BLOCK")==0);
    b=tips({BladeDance,0});CHECK(b.count==2&&b.items[0].kind==HoverTipKind::Card&&b.items[0].card.kind==Shiv&&b.find(HoverTipKind::Keyword,"EXHAUST")==1);
    b=tips({Dazed,0});CHECK(b.count==3&&b.find(HoverTipKind::Keyword,"ETHEREAL")==0&&b.find(HoverTipKind::Keyword,"EXHAUST")==1&&b.find(HoverTipKind::Keyword,"UNPLAYABLE")==2);
    b=tips({StormOfSteel,1});CHECK(b.items[0].kind==HoverTipKind::Card&&b.items[0].card.kind==Shiv&&b.items[0].card.upgraded);
    b=tips({DefectDualcast,0});CHECK(b.find(HoverTipKind::Static,"EVOKE")>=0);
    for(const auto&row:native_indexes){const char*key=std::strchr(row.key,'.');CHECK(key);CHECK(*a.content.world_string("orbs",key+1));}
    b=tips({RegentBigBang,0});CHECK(b.find(HoverTipKind::Static,"ENERGY")>=0&&b.find(HoverTipKind::Static,"FORGE")>=0&&b.find(HoverTipKind::Card,"SOVEREIGN_BLADE")>=0&&b.find(HoverTipKind::Keyword,"RETAIN")>=0);
    b=tips({Purity,0});CHECK(b.find(HoverTipKind::Keyword,"RETAIN")>=0);b=tips({Backstab,0});CHECK(b.find(HoverTipKind::Keyword,"INNATE")>=0);
    b=tips({NecroNecroMastery,1});int index=b.find(HoverTipKind::Static,"SUMMON_DYNAMIC");CHECK(index>=0&&b.items[index].amount==8);
    CardHoverTarget target{};target.card={NecroNecroMastery,1};char title[160]{},body[2048]{};format_card_hover_tip(a,target,b.items[index],title,sizeof title,body,sizeof body);CHECK(std::strstr(title,"8")&&std::strstr(body,"8")&&!std::strchr(body,'{'));
    EventCardFacts facts{};facts.science_type=unsigned(CardType::Skill);facts.rider=unsigned(TinkerRider::Sapping);facts.enchantment=EventEnchantment::RoyallyApproved;
    b=tips({EventCardMadScience,0},-1,&facts);CHECK(b.find(HoverTipKind::Power,"WEAK_POWER")>=0&&b.find(HoverTipKind::Power,"VULNERABLE_POWER")>=0&&b.find(HoverTipKind::Static,"BLOCK")>=0&&b.find(HoverTipKind::Keyword,"INNATE")>=0&&b.find(HoverTipKind::Keyword,"RETAIN")>=0);unchanged();
    // Local modifiers belong to this particular copy, never an equal model
    // in the collection or an unrelated generated-card preview.
    a.game.silent.sly[0]=a.game.silent.retain[0]=a.game.silent.inky[0]=a.game.necro.ethereal[0]=1;a.game.expansion.replay[0]=2;
    b=tips({Bash,0},0);CHECK(b.find(HoverTipKind::Keyword,"SLY")>=0&&b.find(HoverTipKind::Keyword,"RETAIN")>=0&&b.find(HoverTipKind::Keyword,"ETHEREAL")>=0&&b.find(HoverTipKind::Keyword,"EXHAUST")>=0&&b.find(HoverTipKind::Enchantment,"INKY")>=0&&b.find(HoverTipKind::Power,"WEAK_POWER")>=0);
    index=b.find(HoverTipKind::Static,"REPLAY_DYNAMIC");CHECK(index>=0&&b.items[index].amount==2);
    a.library=true;b=tips({Bash,0});CHECK(b.count==1);a.library=false;
    a.game.enemy_world.affliction[0]=unsigned(EnemyAffliction::Hexed);b=tips({Bash,0},0);CHECK(b.find(HoverTipKind::Affliction,"HEXED")>=0);
    a.game.enemy_world.active=1;a.game.enemy_world.models[0].power[unsigned(EnemyPower::Sandpit)]=3;
    b=tips({EventCardFranticEscape,0});CHECK(b.count==1&&b.items[0].smart&&b.items[0].amount==3&&b.items[0].actor==1);setup();
}
static void duplicate_rules(){
    HoverTipList b{};HoverTip weak{HoverTipKind::Power,"WEAK_POWER"},vulnerable{HoverTipKind::Power,"VULNERABLE_POWER"};
    b.add(weak);b.add(vulnerable);b.add(weak);CHECK(b.count==2&&b.find(HoverTipKind::Power,"WEAK_POWER")==1);
    weak.smart=true;weak.amount=3;b.add(weak);weak.smart=false;weak.amount=0;b.add(weak);CHECK(b.count==2&&b.items[1].smart&&b.items[1].amount==3);
    b.add({HoverTipKind::Keyword,"ETHEREAL"});b.add({HoverTipKind::Keyword,"EXHAUST"});b.add({HoverTipKind::Keyword,"ETHEREAL"});CHECK(b.find(HoverTipKind::Keyword,"ETHEREAL")==2);
    b.add({HoverTipKind::Static,"SUMMON_DYNAMIC"});b.add({HoverTipKind::Static,"SUMMON_STATIC"});b.finish();CHECK(b.find(HoverTipKind::Static,"SUMMON_DYNAMIC")<0);
}
static void complete_catalog_and_layout(){
    setup();snapshot();a.library=true;
    for(unsigned kind=0;kind<CardKinds;++kind)for(unsigned up=0;up<2;++up){
        CardHoverTarget owner{};owner.card={CardKind(kind),uint8_t(up)};owner.owner={120,122,80,112};auto b=tips(owner.card);
        for(unsigned i=0;i<b.count;++i){char title[160]{},body[2048]{};format_card_hover_tip(a,owner,b.items[i],title,sizeof title,body,sizeof body);
            if(!*title||!*body||std::strchr(body,'{')||std::strchr(title,'{'))std::fprintf(stderr,"unresolved hover %s -> %s: %s / %s\n",definitions[kind].key,b.items[i].key,title,body);
            CHECK(*title&&*body&&!std::strchr(body,'{')&&!std::strchr(title,'{'));
        }
        for(bool details:{false,true}){a.keyword_hover.details=owner.detail=details;int visited[MaxCardHoverTips]{};
            const auto first=card_hover_page(a,owner,b);CHECK(first.pages>0&&first.pages<=128);
            for(int p=0;p<first.pages;++p){const auto page=card_hover_page(a,owner,b,p);CHECK(page.pages==first.pages);if(b.count)CHECK(page.count>0);
                for(unsigned j=0;j<page.count;++j){const auto&piece=page.pieces[j];const auto&r=piece.box;CHECK(piece.tip<b.count&&piece.lines>0&&piece.skip==visited[piece.tip]);visited[piece.tip]+=piece.lines;
                    CHECK(r.x>=0&&r.y>=24&&r.w>0&&r.h>0&&r.x+r.w<=320&&r.y+r.h<=(details?191:218));}
            }
            for(unsigned i=0;i<b.count;++i)CHECK(visited[i]>0);
        }
    }
    for(int x:{0,3,55,120,200,270,319})for(int y:{0,40,122,239}){auto r=card_hover_column({x,y,48,67});CHECK(r.x>=0&&r.x+r.w<=320&&r.y>=0&&r.y+r.h<=240);}
    a.library=false;a.keyword_hover={};unchanged();
}
static void pointer_and_keyboard(){
    setup();snapshot();CardHoverTarget owner{};CHECK(!card_hover_target(a,owner));
    auto rail=hand_rail(a.game.hand.count,0);a.pointer_move(rail.x+rail.w/2,224);a.render();CHECK(card_hover_target(a,owner)&&owner.instance==0&&owner.card.kind==Bash);
    capture("keyword-hand-bash");const int hits=a.hit_count;for(int n=0;n<3;++n){a.render();CHECK(a.hit_count==hits&&card_hover_target(a,owner)&&owner.instance==0);}unchanged();
    a.pointer_button(true);CHECK(!card_hover_target(a,owner));a.pointer_button(false);CHECK(a.pointer.armed_card==0&&!card_hover_target(a,owner));a.pointer_cancel();unchanged();
    a.pointer_move(300,23);a.render();CHECK(!card_hover_target(a,owner));a.input(Key::Right);a.render();CHECK(card_hover_target(a,owner)&&a.hand_focus);unchanged();
    a.paused=true;CHECK(!card_hover_target(a,owner));a.paused=false;a.help=true;CHECK(!card_hover_target(a,owner));a.help=false;
    a.settings_open=true;CHECK(!card_hover_target(a,owner));a.settings_open=false;a.frontend_confirm=1;CHECK(!card_hover_target(a,owner));a.frontend_confirm=0;
    a.open_card_detail({DefectRainbow,0});a.pointer.visible=false;a.render();CHECK(!card_hover_target(a,owner));a.input(Key::Statuses);a.render();CHECK(a.keyword_hover.details&&card_hover_target(a,owner)&&owner.detail);
    capture("keyword-detail-orbs");auto b=tips(owner.card);auto page=card_hover_page(a,owner,b);a.input(Key::Down);CHECK(a.keyword_hover.page==(page.pages>1?1:0));a.pointer_scroll(-1);CHECK(a.keyword_hover.page==0);
    a.input(Key::Back);CHECK(!a.keyword_hover.details&&a.detail_card>=0);a.input(Key::Back);CHECK(a.detail_card<0);unchanged();
    a.open_card_detail({Bash,0});a.pointer_move(55,110);a.render();CHECK(card_hover_target(a,owner));a.pointer_move(250,215);CHECK(!card_hover_target(a,owner));a.render();
    auto control=hit(HitKind::Command,int(Key::Statuses));a.pointer_move(control.x+control.w/2,control.y+control.h/2);a.pointer_button(true);a.pointer_button(false);a.render();CHECK(a.keyword_hover.details);unchanged();
    a.input(Key::Back);a.input(Key::Back);a.library=true;a.library_filter=a.library_sort=a.library_page=a.library_tab=a.library_selected=0;a.pointer.visible=false;a.render();CHECK(card_hover_target(a,owner));
    auto item=hit(HitKind::LibraryItem,2);a.pointer_move(item.x+item.w/2,item.y+item.h/2);a.render();CHECK(card_hover_target(a,owner));capture("keyword-library");unchanged();
    a.library=false;a.panel=1;a.pile_tab=0;a.inspect_cursor=a.scroll=0;a.pointer.visible=false;a.render();CHECK(card_hover_target(a,owner)&&owner.deck==0);unchanged();
    a.reset_presentation();CHECK(!a.keyword_hover.details&&!a.keyword_hover.owner&&!a.keyword_hover.page);unchanged();
    // Visible tips must not turn a legal card drag into a tooltip command.
    setup();rail=hand_rail(a.game.hand.count,0);a.pointer_move(rail.x+rail.w/2,224);a.pointer_button(true);a.pointer_move(238,82);a.pointer_button(false);
    CHECK(a.game.hand.count==4&&a.game.enemies[0].hp==38&&a.game.energy==97);CHECK(validate(a.game));
}
static void event_begin(OriginalEventId id,unsigned seed=77){
    new_run(a.game,seed,false);CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,0));
    if(id!=OriginalEventId::Neow){a.game.original_events={};CHECK(original_event_begin(a.game,id,seed,1));}
    a.game.gold=1000;a.title=a.library=a.paused=a.help=a.character_select=a.settings_open=false;
    a.run_settings=false;a.map_preview.open=false;a.frontend_confirm=0;a.reset_presentation();a.pointer.visible=false;
    CHECK(validate(a.game));
}
static HoverTipList event_tips(int option){HoverTipList out{};collect_event_hover_tips(a,option,out);CHECK(!out.overflow);return out;}
static void event_associations(){
    event_begin(OriginalEventId::Amalgamator);snapshot();
    auto t=event_tips(0);CHECK(t.find(HoverTipKind::Card,"ULTIMATE_STRIKE")>=0);
    t=event_tips(1);CHECK(t.find(HoverTipKind::Card,"ULTIMATE_DEFEND")>=0&&t.find(HoverTipKind::Static,"BLOCK")>=0);unchanged();
    event_begin(OriginalEventId::AromaOfChaos);snapshot();t=event_tips(0);CHECK(t.count==1&&t.find(HoverTipKind::Static,"TRANSFORM")==0);CHECK(event_tips(1).count==0);unchanged();
    event_begin(OriginalEventId::GraveOfTheForgotten);snapshot();t=event_tips(0);
    CHECK(t.find(HoverTipKind::Enchantment,"SOULS_POWER")>=0&&t.find(HoverTipKind::Card,"DECAY")>=0&&t.find(HoverTipKind::Keyword,"EXHAUST")>=0);unchanged();
    event_begin(OriginalEventId::SelfHelpBook);snapshot();t=event_tips(0);
    CHECK(t.count&&t.items[0].kind==HoverTipKind::Enchantment&&t.items[0].amount==2);unchanged();
    event_begin(OriginalEventId::DrowningBeacon);snapshot();t=event_tips(0);CHECK(t.find(HoverTipKind::Potion,"GLOWWATER_POTION")>=0);
    t=event_tips(1);CHECK(t.find(HoverTipKind::Relic,"FRESNEL_LENS")>=0&&t.find(HoverTipKind::Enchantment,"NIMBLE")>=0);unchanged();
    event_begin(OriginalEventId::TeaMaster);snapshot();t=event_tips(1);
    CHECK(t.find(HoverTipKind::Relic,"EMBER_TEA")<0&&t.find(HoverTipKind::Power,"STRENGTH_POWER")>=0);unchanged();
    event_begin(OriginalEventId::SlipperyBridge);auto&e=a.game.original_events;CHECK(e.values[1]>=0);
    a.game.event_cards.deck[e.values[1]].energy_offset=-1;snapshot();t=event_tips(0);CHECK(t.count==1&&t.items[0].kind==HoverTipKind::Card);
    CHECK(t.items[0].card.kind==a.game.deck[e.values[1]].kind&&t.items[0].has_facts&&t.items[0].facts.energy_offset==-1);unchanged();
    // Ancient offers expose associated mechanics without duplicating their
    // own description. Reading a prepared transformation must not reroll it.
    event_begin(OriginalEventId::Neow);a.game.original_events.offers[0]=WorldRelicArchaicTooth;
    CHECK(world_relic_prepare(a.game,WorldRelicArchaicTooth));snapshot();t=event_tips(0);
    CHECK(t.find(HoverTipKind::Relic,"ARCHAIC_TOOTH")<0);unsigned cards=0;for(unsigned i=0;i<t.count;++i)if(t.items[i].kind==HoverTipKind::Card)++cards;CHECK(cards==2);unchanged();
    // Same event, different live pages: the merchant's Guilty and the noble's
    // Innocent are separate associations, not a keyword scan of their prose.
    event_begin(OriginalEventId::Trial);CHECK(original_event_choose(a.game,0));snapshot();
    t=event_tips(1);const auto page=a.game.original_events.page;
    CHECK(t.find(HoverTipKind::Card,page==2?"SHAME":page==3?"REGRET":"DOUBT")>=0);unchanged();
}
static void event_pointer_keyboard_and_text(){
    const OriginalEventId ids[]={OriginalEventId::Amalgamator,OriginalEventId::AromaOfChaos,OriginalEventId::Bugslayer,
      OriginalEventId::DrowningBeacon,OriginalEventId::FieldOfManSizedHoles,OriginalEventId::GraveOfTheForgotten,
      OriginalEventId::TeaMaster,OriginalEventId::SelfHelpBook,OriginalEventId::WaterloggedScriptorium,OriginalEventId::WoodCarvings};
    for(auto id:ids){event_begin(id);snapshot();a.render();
        for(unsigned option=0;option<original_event_option_count(a.game);++option){
            a.cursor=int(option);a.pointer.visible=false;a.render();CardHoverTarget owner{};CHECK(event_hover_target(a,owner));CHECK(owner.event_option==int(option));
            auto t=event_tips(int(option));for(unsigned i=0;i<t.count;++i){char title[160]{},body[2048]{};
                format_card_hover_tip(a,owner,t.items[i],title,sizeof title,body,sizeof body);
                if(!*title||!*body||std::strchr(body,'{'))std::fprintf(stderr,"event hover unresolved %s / %s: %s\n",original_event_key(id),t.items[i].key,body);
                CHECK(*title&&*body&&!std::strchr(body,'{')&&!std::strchr(title,'{'));
            }
            auto h=hit(HitKind::Choice,int(option));a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.render();CHECK(event_hover_target(a,owner)&&owner.event_option==int(option));
            if(t.count){a.pointer.visible=false;CHECK(keyword_detail_input(a,Key::Statuses));a.render();CHECK(a.keyword_hover.details);CHECK(event_hover_target(a,owner));
                const auto pages=card_hover_page(a,owner,t,0);CHECK(pages.pages>0);keyword_hover_scroll(a,1000);a.render();CHECK(a.keyword_hover.page<pages.pages);
                CHECK(keyword_detail_input(a,Key::Back));CHECK(!a.keyword_hover.details);a.render();}
        }unchanged();
    }
    event_begin(OriginalEventId::Bugslayer);a.render();capture("event-option-tooltip");snapshot();
    CHECK(keyword_detail_input(a,Key::Statuses));a.render();capture("event-option-tooltip-details");unchanged();
    CHECK(keyword_detail_input(a,Key::Back));a.input(Key::Detail);a.render();CardHoverTarget owner{};CHECK(!event_hover_target(a,owner));
}
int main(int argc,char**argv){
    std::vector<uint8_t>data;actual=argc>1;if(actual){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}else data=fixture();
    CHECK(a.init(data.data(),data.size(),{}));if(argc>2)screenshots=argv[2];associations();duplicate_rules();complete_catalog_and_layout();pointer_and_keyboard();event_associations();event_pointer_keyboard_and_text();
    std::printf("keyword tooltip: %u checks passed (%s profile)\n",checks,actual?"private":"synthetic");
}
