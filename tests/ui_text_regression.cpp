#include "../src/app.cpp"
#include "content_fixture.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace spire;
static App a;
static SaveData before,after;
static unsigned checks,route_calls;
static bool actual_pack;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"ui text: %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)

static void reset(Character character=Character::Ironclad,bool practice=false){
    new_run(a.game,781,practice,character);a.title=a.library=a.character_select=a.run_settings=false;
    a.grimoire_select=false;a.choice_selected_count=0;a.selected_ascension=0;
    CHECK(validate(a.game));
}
static void snapshot(){CHECK(validate(a.game));CHECK(save(a.game,before));}
static void unchanged(){CHECK(save(a.game,after));CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static bool braces(const char*text){return std::strchr(text,'{')||std::strchr(text,'}');}
static std::string ui(const char*source){char buffer[1024]{};const char*out=a.ui_text(source,buffer,sizeof buffer);CHECK(out);return out;}
static void number(const char*source,int expected){
    const auto shown=ui(source);CHECK(!braces(shown.c_str()));char digits[32];std::snprintf(digits,sizeof digits,"%d",expected);
    CHECK(shown.find(digits)!=std::string::npos);
}
static void single_remove(){reset();a.game.phase=Phase::Shop;CHECK(buy(a.game,ShopRemove));CHECK(a.game.phase==Phase::Remove&&validate(a.game));}
static void two_remove(){
    reset();a.game.phase=Phase::Map;
    CHECK(original_event_begin(a.game,OriginalEventId::LuminousChoir,0x74657874,0));
    CHECK(original_event_choose(a.game,0));
    CHECK(a.game.phase==Phase::OriginalEventChoice&&a.game.original_events.selection==OriginalEventSelection::Remove);
    CHECK(a.world_choice_min()==2&&a.world_choice_max()==2&&validate(a.game));
}
static void optional_exhaust(){
    reset(Character::Ironclad,true);auto&s=a.game;int hand=-1,draw=-1;
    for(int i=0;i<s.hand.count;++i)if(s.cards[s.hand.ids[i]].kind==Purity)hand=i;
    for(int i=0;i<s.draw.count;++i)if(s.cards[s.draw.ids[i]].kind==Purity)draw=i;
    if(hand<0){CHECK(draw>=0&&s.hand.count);hand=s.hand.count-1;
        const auto chosen=s.draw.ids[draw],displaced=s.hand.ids[hand];s.hand.ids[hand]=chosen;s.draw.ids[draw]=displaced;
        s.zones[chosen]=Zone::Hand;s.zones[displaced]=Zone::Draw;}
    CHECK(play(s,hand,0));CHECK(s.phase==Phase::ExhaustManyChoice&&choice_min(s)==0&&choice_max(s)==3&&validate(s));
}
static void colored_amount(int amount){
    const auto shown=ui("[blue]{Amount}[/blue]");CHECK(!braces(shown.c_str()));
    RichReader reader(shown.c_str());RichChar ch{};CHECK(reader.next(ch));CHECK(ch.codepoint==unsigned('0'+amount)&&ch.style.color==3);CHECK(!reader.next(ch));
    if(!actual_pack)return;
    Canvas c{a};char digits[2]={char('0'+amount),0};CHECK(c.width("[blue]{Amount}[/blue]",12)==c.width(digits,12));
    // Pixel comparison exercises the final text boundary, not a duplicate
    // string-only formatter. Every canvas label entry uses that boundary.
    c.rect(0,0,320,240,Ink);c.text(digits,13,17,12,Blue);const std::vector<uint16_t>expected(a.pixels,a.pixels+320*240);
    c.rect(0,0,320,240,Ink);c.text("[blue]{Amount}[/blue]",13,17,12,White);CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
    c.rect(0,0,320,240,Ink);c.paragraph("[blue]{Amount}[/blue]",13,17,100,1,0,12,White);CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
    c.rect(0,0,320,240,Ink);c.text_fitted("[blue]{Amount}[/blue]",13,17,100,12,White,12);CHECK(!std::memcmp(expected.data(),a.pixels,sizeof a.pixels));
}
static void selection_contexts(){
    single_remove();snapshot();number("{Amount}",1);colored_amount(1);
    if(actual_pack){if(a.content.english()){const auto shown=ui("移除一张牌");CHECK(!braces(shown.c_str()));
            CHECK(shown.find("a card")!=std::string::npos&&shown.find("cards")==std::string::npos);
        }else number("移除一张牌",1);}unchanged();
    two_remove();snapshot();number("{Amount}",2);colored_amount(2);
    if(actual_pack){number(world_choice_heading(a.game),2);const auto initial=ui(world_choice_heading(a.game));
        a.choice_selected_count=1;CHECK(ui(world_choice_heading(a.game))==initial);a.choice_selected_count=2;CHECK(ui(world_choice_heading(a.game))==initial);}
    unchanged();optional_exhaust();snapshot();number("{Amount}",3);colored_amount(3);
    if(actual_pack)number("选择要消耗的手牌",3);unchanged();
    reset();const char*unbound="{Amount}";char output[64];CHECK(a.ui_text(unbound,output,sizeof output)==unbound);
}
static void hotkeys_and_frontend(){
    reset();snapshot();
    for(const char*name:{"Hotkey","HotKey"}){
        char raw[128];std::snprintf(raw,sizeof raw,"A{%s:choose(None|Tab):|[{%s}]|?}Z",name,name);
        CHECK(ui(raw)=="AZ");
        char output[128];UiTextContext context{&a,"Tab"};LocalizedTemplate formatter(output,sizeof output,ui_text_value,&context);
        formatter.format(raw);CHECK(!formatter.truncated()&&!std::strcmp(output,"A[Tab]Z"));
    }
    if(actual_pack){const auto deck=ui("牌组"),exhaust=ui("消耗堆");CHECK(!braces(deck.c_str())&&!braces(exhaust.c_str()));
        CHECK(deck.find("Tab")!=std::string::npos&&exhaust.find("Tab")==std::string::npos&&exhaust.find("None")==std::string::npos);}
    a.character_select=true;a.selected_ascension=9;CHECK(ui("{ascension}")=="9");
    if(actual_pack)number("进阶等级",9);
    a.character_select=false;a.run_settings=true;a.selected_ascension=4;CHECK(ui("{ascension}")=="4");
    a.run_settings=false;CHECK(ui("{actNumber}")=="1");CHECK(ui("{singleStarIcon}")== (a.content.english()?"Stars":"辉星"));unchanged();
    CHECK(world_begin_run(a.game,reference::ActKind::Overgrowth,7));snapshot();a.selected_ascension=2;
    CHECK(ui("{ascension}")=="7"&&ui("{actNumber}")=="1");if(actual_pack){number("进阶等级",7);number("第 {actNumber} 幕",1);}unchanged();
}
static void bounded_fallback(){
    reset();snapshot();char output[1024];std::memset(output,0x5a,sizeof output);
    const std::string unknown=std::string(1200,'L')+" {Unbound.Event.Value} "+std::string(400,'R');
    const char*shown=a.ui_text(unknown.c_str(),output,sizeof output);
    CHECK(shown==unknown.c_str()&&std::strlen(shown)==unknown.size()&&output[0]=='Z');
    CHECK(a.ui_text(unknown.c_str(),nullptr,0)==unknown.c_str());
    const std::string known=std::string(1200,'L')+" {actNumber} "+std::string(400,'R');
    CHECK(a.ui_text(known.c_str(),output,sizeof output)==known.c_str());
    CHECK(std::strlen(a.ui_text(known.c_str(),output,sizeof output))==known.size());
    const char*short_known="X{actNumber}";char fit[3],small[2];
    CHECK(!std::strcmp(a.ui_text(short_known,fit,sizeof fit),"X1"));CHECK(a.ui_text(short_known,small,sizeof small)==short_known);
    UiTextContext context{&a,"None"};LocalizedTemplate exact(fit,sizeof fit,ui_text_value,&context);exact.format(short_known);CHECK(!exact.truncated());
    LocalizedTemplate too_small(small,sizeof small,ui_text_value,&context);too_small.format(short_known);CHECK(too_small.truncated());
    const char*model="[gold]{n}[/gold] {OwnerName}";CHECK(a.ui_text(model,output,sizeof output)==model);
    CHECK(ui("{Unbound.Event.Value}/{actNumber}")=="{Unbound.Event.Value}/1");unchanged();
}

// Each model fixture uses the production power queue where one exists and is
// validated/saved before display. Positive live amounts are never supplied by
// a generic zero-fallback resolver. Enemy-only statuses use an actual enemy.
static int prepare_status(int id){
    CHECK(id>=0&&id<LegacyStatusKinds);Character character=Character::Ironclad;
    if(id>=SilentStatusStart&&id<DefectStatusStart)character=Character::Silent;
    else if(id>=DefectStatusStart&&id<RegentStatusStart)character=Character::Defect;
    else if(id>=RegentStatusStart&&id<NecroStatusStart)character=Character::Regent;
    else if(id>=NecroStatusStart)character=Character::Necrobinder;
    if(id==44)character=Character::Regent;if(id==45)character=Character::Necrobinder;if(id==46)character=Character::Defect;
    reset(character,true);auto&s=a.game;CHECK(encounter_setup(s,EncounterId::NibbitsWeak));int actor=0;
    if(id<18){switch(id){
        case 0:s.strength=7;break;case 1:s.dexterity=7;break;case 2:s.vigor=7;break;case 3:s.thorns=7;break;
        case 4:s.enemies[0].weak=7;actor=1;break;case 5:s.enemies[0].vulnerable=7;actor=1;break;case 6:s.enemies[0].poison=7;actor=1;break;
        case 7:s.pain=7;s.power_count=1;s.power_order[0]=0;break;case 8:s.embrace=7;s.power_count=1;s.power_order[0]=1;break;
        case 9:s.no_draw=1;break;case 10:s.no_block=7;break;case 11:s.noxious_fumes=7;break;case 12:s.accuracy=7;break;
        case 13:s.demon_form=7;break;case 14:s.infinite_blades=7;break;case 15:s.flame_barrier=7;break;
        case 16:s.block_next_turn=7;break;case 17:s.draw_next_turn=7;break;
    }}else if(id<40)queue_extension(s,XOp::Power,id-18,7);
    else if(id==40){s.expansion.demise[0]=7;actor=1;}
    else if(id==41){s.expansion.beetle[0]=7;actor=1;}
    else if(id==42)s.necro.player_doom=7;
    else if(id==43){s.expansion.bomb_turns[0]=2;s.expansion.bomb_damage[0]=40;}
    else if(id==44)s.expansion.stars=7;
    else if(id==45){s.expansion.osty_max_hp=10;s.expansion.osty_hp=7;}
    else if(id==46)queue_extension(s,XOp::DChannel,int(DOrb::Lightning),2);
    else if(id<SilentStatusStart)queue_extension(s,XOp::IPower,id-IroncladStatusStart,7);
    else if(id<CharacterExtraStatusStart)queue_extension(s,XOp::SPower,id-SilentStatusStart,7);
    else if(id==CharacterExtraStatusStart){s.silent.nightmare_active[2]=1;s.silent.nightmares[2].card={Bash,1};s.silent.power_order[s.silent.power_count++]=SPowerCount+2;}
    else if(id==CharacterExtraStatusStart+1){s.silent.strangle[0]=7;actor=1;}
    else if(id==CharacterExtraStatusStart+2){s.silent.enemy_artifact[0]=7;actor=1;}
    else if(id==CharacterExtraStatusStart+3){s.expansion.restore_strength[0]=7;actor=1;}
    else if(id==CharacterExtraStatusStart+4)s.ironclad.setup_strength=7;
    else if(id==CharacterExtraStatusStart+5)s.silent.extra_rewards=7;
    else if(id<RegentStatusStart){const int power=id-DefectStatusStart;
        if(power>=int(DPower::FocusedStrikeFocus))queue_extension(s,XOp::DFocus,power==int(DPower::HyperbeamFocus)?-7:7,power-int(DPower::FocusedStrikeFocus)+1);
        else queue_extension(s,XOp::DPower,power,7);
    }else if(id<NecroStatusStart)queue_extension(s,XOp::RPower,id-RegentStatusStart,7);
    else if(id<FiveCharacterExtraStatusStart)queue_extension(s,XOp::NPower,id-NecroStatusStart,7);
    else {actor=1;switch(id-FiveCharacterExtraStatusStart){
        case 0:s.regent.conqueror[0]=7;break;case 1:s.necro.debilitate[0]=7;break;case 2:s.necro.hang[0]=7;break;
        case 3:s.necro.oblivion[0]=7;break;case 4:s.necro.sic_em[0]=7;break;
    }}
    resolve_pending(s);if(!validate(s))std::fprintf(stderr,"invalid status fixture %d\n",id);
    CHECK(validate(s));CHECK(status_amount(s,actor,id)>0);return actor;
}
struct ExpectedPowerValues {int amount;const char*first;int first_value;const char*second=nullptr;int second_value=0;};
static LocalizedValue expected_power_value(const void*opaque,const char*name){
    const auto&v=*static_cast<const ExpectedPowerValues*>(opaque);
    if(!std::strcmp(name,"Amount"))return wt_number(v.amount);
    if(v.first&&!std::strcmp(name,v.first))return wt_number(v.first_value);
    if(v.second&&!std::strcmp(name,v.second))return wt_number(v.second_value);
    return {};
}
static std::string expected_power(const char*key,const ExpectedPowerValues&values){
    char text[4096]{};const auto&c=a.content;
    LocalizedTemplate formatter(text,sizeof text,expected_power_value,&values,c.world_title("static_hover_tips","ENERGY_COUNT"),c.world_title("static_hover_tips","STAR_COUNT"),c.english());
    formatter.format(c.world_string("powers",key));CHECK(!formatter.truncated()&&*text&&!braces(text));return text;
}
static int model_value(const char*key,int instance,const char*field,int actor=0){
    WorldTextContext context{&a,"powers",{},instance,actor,0};wt_model_key(context,key);const auto value=wt_resolve(&context,field);
    CHECK(value.known&&!value.text&&value.scale==1);return int(value.number);
}
static void exact_power_line(const char*key,int slot,int amount,const ExpectedPowerValues&values){
    if(!actual_pack)return;char text[4096]{};a.localized_text("powers",key,text,sizeof text,slot,0,amount);CHECK(expected_power(key,values)==text);
}
static void exact_status(int id,const std::string&expected){
    if(!actual_pack)return;char text[4096]{};ui_status_description(a,0,id,text,sizeof text);CHECK(expected==text&&!braces(text));
}
static void live_power_values(){
    for(IPower power:{IPower::CrimsonMantle,IPower::Inferno}){
        const int id=IroncladStatusStart+int(power);prepare_status(id);queue_extension(a.game,XOp::IPower,int(power),7);resolve_pending(a.game);
        const char*key=power==IPower::CrimsonMantle?"CRIMSON_MANTLE_POWER.smartDescription":"INFERNO_POWER.smartDescription";
        const int damage=power==IPower::CrimsonMantle?a.game.ironclad.crimson_self_damage:a.game.ironclad.inferno_self_damage;
        CHECK(damage==2&&status_amount(a.game,0,id)==14);snapshot();CHECK(model_value(key,-1,"SelfDamage")==damage);
        CHECK(model_value(key,-1,"SelfDamage",1)==0);exact_power_line(key,-1,14,{14,"SelfDamage",2});
        if(actual_pack)exact_status(id,expected_power(key,{14,"SelfDamage",2}));unchanged();
    }
    reset(Character::Ironclad,true);queue_extension(a.game,XOp::Power,int(XPower::Panache),10);queue_extension(a.game,XOp::Power,int(XPower::Panache),14);resolve_pending(a.game);
    // These are saved, independently valid progress counters. Giving the two
    // slots different progress catches aggregation/first-slot substitutions.
    a.game.expansion.panache_count[0]=1;a.game.expansion.panache_count[1]=4;a.game.expansion.panache_skip[0]=a.game.expansion.panache_skip[1]=0;
    snapshot();const char*panache="PANACHE_POWER.smartDescription";
    CHECK(status_amount(a.game,0,32)==24&&model_value(panache,0,"CardsLeft")==4&&model_value(panache,1,"CardsLeft")==1);
    CHECK(model_value(panache,-1,"CardsLeft")==5&&model_value(panache,2,"CardsLeft")==5);
    exact_power_line(panache,0,10,{10,"CardsLeft",4});exact_power_line(panache,1,14,{14,"CardsLeft",1});
    if(actual_pack)exact_status(32,expected_power(panache,{10,"CardsLeft",4})+"\n"+expected_power(panache,{14,"CardsLeft",1}));unchanged();

    reset(Character::Ironclad,true);queue_extension(a.game,XOp::Bomb,50,2);queue_extension(a.game,XOp::Bomb,40,1);resolve_pending(a.game);snapshot();
    const char*bomb="THE_BOMB_POWER.smartDescription";CHECK(status_amount(a.game,0,43)==2&&model_value(bomb,0,"Damage")==50&&model_value(bomb,1,"Damage")==40);
    CHECK(model_value(bomb,-1,"Damage")==40&&model_value(bomb,2,"Damage")==40);
    exact_power_line(bomb,0,2,{2,"Damage",50});exact_power_line(bomb,1,1,{1,"Damage",40});
    if(actual_pack)exact_status(43,expected_power(bomb,{2,"Damage",50})+"\n"+expected_power(bomb,{1,"Damage",40}));unchanged();

    reset(Character::Regent,true);queue_extension(a.game,XOp::RPower,int(RPower::Monologue),1);queue_extension(a.game,XOp::RPower,int(RPower::Monologue),1);
    resolve_pending(a.game);queue_extension(a.game,XOp::RMonologueStrength,0,5);queue_extension(a.game,XOp::RMonologueStrength,1,2);resolve_pending(a.game);snapshot();
    const char*monologue="MONOLOGUE_POWER.smartDescription";CHECK(a.game.regent.monologue_applied[0]==5&&a.game.regent.monologue_applied[1]==2);
    CHECK(model_value(monologue,0,"StrengthApplied")==5&&model_value(monologue,1,"StrengthApplied")==2);
    CHECK(model_value(monologue,0,"StrengthPower")==1&&model_value(monologue,1,"StrengthPower")==1);
    CHECK(model_value(monologue,-1,"StrengthApplied")==0&&model_value(monologue,2,"StrengthApplied")==0);
    exact_power_line(monologue,0,1,{1,"StrengthPower",1,"StrengthApplied",5});exact_power_line(monologue,1,1,{1,"StrengthPower",1,"StrengthApplied",2});
    if(actual_pack)exact_status(RegentStatusStart+int(RPower::Monologue),expected_power(monologue,{1,"StrengthPower",1,"StrengthApplied",5})+"\n"+expected_power(monologue,{1,"StrengthPower",1,"StrengthApplied",2}));unchanged();
    std::puts("{\"operation\":\"live_power_values\",\"power_models\":5,\"independent_instance_pairs\":3,\"canonical_fallback_checked\":true,\"state_bytes_unchanged\":true}");
}
static std::set<std::string> fields(const char*text){
    std::set<std::string>out;for(const char*p=text;*p;++p)if(*p=='{'){
        const char*start=p+1,*end=start;while((*end>='A'&&*end<='Z')||(*end>='a'&&*end<='z')||(*end>='0'&&*end<='9')||*end=='_'||*end=='.')++end;
        if(end>start)out.emplace(start,size_t(end-start));
    }return out;
}
static std::string unhex(const std::string&input){
    CHECK(input.size()%2==0);std::string output;
    auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t i=0;i<input.size();i+=2){int high=digit(input[i]),low=digit(input[i+1]);CHECK(high>=0&&low>=0);output.push_back(char(high*16+low));}return output;
}
static unsigned catalog_routes(const char*path){
    std::ifstream input(path);CHECK(input.good());std::string line;unsigned bindings=0,dynamic=0,failures=0;
    while(std::getline(input,line)){
        std::istringstream row(line);std::string slot_text,route,ids,reference,source_hex;
        CHECK(bool(std::getline(row,slot_text,'\t'))&&bool(std::getline(row,route,'\t'))&&bool(std::getline(row,ids,'\t'))&&bool(std::getline(row,reference,'\t'))&&bool(std::getline(row,source_hex)));
        unsigned slot=unsigned(std::stoul(slot_text));CHECK(slot==LocaleUiTextBase+bindings++);const auto source=unhex(source_hex);
        const std::string expected=a.content.string(slot),bound=a.content.ui_text(source.c_str());CHECK(bound==expected);
        if(route=="static")continue;++dynamic;std::set<std::string>remaining;bool unresolved=false;unsigned calls=0;
        auto observe=[&](const char*text){const auto names=fields(text);remaining.insert(names.begin(),names.end());unresolved|=braces(text)||!*text;++calls;++route_calls;};
        if(route=="status"){
            std::istringstream list(ids);std::string entry;while(std::getline(list,entry,',')){
                const int id=std::stoi(entry),actor=prepare_status(id);snapshot();char text[4096]{};
                ui_status_description(a,actor,id,text,sizeof text);observe(text);unchanged();
            }
        }else if(route=="relic"){
            reset();init_world_relics(a.game);const std::string key=reference.substr(7,reference.find('.',7)-7);RelicId found=WorldRelicKinds;
            for(unsigned id=0;id<WorldRelicKinds;++id)if(key==world_relic_key(RelicId(id)))found=RelicId(id);
            CHECK(found<WorldRelicKinds);snapshot();char text[4096]{};a.relic_description(found,text,sizeof text);observe(text);unchanged();
        }else if(route=="selection"){
            for(unsigned context=0;context<3;++context){if(!context)single_remove();else if(context==1)two_remove();else optional_exhaust();snapshot();observe(ui(source.c_str()).c_str());unchanged();}
        }else if(route=="hotkey"||route=="ascension"||route=="act"){
            reset();a.character_select=true;a.selected_ascension=9;snapshot();observe(ui(source.c_str()).c_str());unchanged();
        }else unresolved=true;
        failures+=unresolved;std::printf("{\"operation\":\"binding\",\"slot\":%u,\"route\":\"%s\",\"route_calls\":%u,\"unresolved\":%s,\"remaining_fields\":[",slot,route.c_str(),calls,unresolved?"true":"false");
        bool comma=false;for(const auto&name:remaining){std::printf("%s\"%s\"",comma?",":"",name.c_str());comma=true;}std::puts("]}");
    }
    CHECK(bindings==935);std::printf("{\"operation\":\"catalog\",\"bindings\":%u,\"dynamic_bindings\":%u,\"route_calls\":%u,\"unresolved_bindings\":%u}\n",bindings,dynamic,route_calls,failures);return failures;
}
int main(int argc,char**argv){
    if(argc>3){std::fprintf(stderr,"Usage: ui_text_regression [REAL_PACK [AUDIT_CATALOG_TSV]]\n");return 2;}
    std::vector<uint8_t>pack;
    if(argc>=2){FILE*file=std::fopen(argv[1],"rb");CHECK(file);pack.resize(spire_test::pack_size(file));CHECK(std::fread(pack.data(),1,pack.size(),file)==pack.size());CHECK(!std::fclose(file));
        CHECK(a.init(pack.data(),pack.size(),{}));CHECK(a.content.version()>=LocaleContentVersion);actual_pack=true;}
    selection_contexts();hotkeys_and_frontend();bounded_fallback();live_power_values();
    unsigned failures=argc==3?catalog_routes(argv[2]):0;
    CHECK(!actual_pack||a.content.healthy());std::printf("{\"operation\":\"regression\",\"checks\":%u,\"actual_pack\":%s,\"unresolved_bindings\":%u,\"passed\":%s}\n",checks,actual_pack?"true":"false",failures,failures?"false":"true");return failures?1:0;
}
