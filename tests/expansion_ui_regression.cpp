#include "app.hpp"
#include "status.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData stored;
static unsigned checks,card_acquisitions,potion_acquisitions;
static bool real_profile;
static const char* screenshot_directory;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static bool number_in(const char* text,unsigned value){
    while(*text){
        if(*text>='0'&&*text<='9'){
            char* end=nullptr;unsigned long found=std::strtoul(text,&end,10);
            if(found==value)return true;text=end;
        }else ++text;
    }
    return false;
}
// Optional local screenshots contain game-derived art and must remain private.
// The caller creates the output directory; normal CTest writes no screenshots.
static void capture(const char* name){
    if(!screenshot_directory)return;
    a.render();std::string path=std::string(screenshot_directory)+"/"+name+".ppm";
    FILE* f=std::fopen(path.c_str(),"wb");CHECK(f);
    CHECK(std::fprintf(f,"P6\n%d %d\n255\n",ScreenW,ScreenH)>0);
    uint8_t row[ScreenW*3];
    for(int y=0;y<ScreenH;y++){
        for(int x=0;x<ScreenW;x++){
            uint16_t p=a.pixels[y*ScreenW+x];
            row[x*3]=(uint8_t)(((p>>11)&31)*255/31);
            row[x*3+1]=(uint8_t)(((p>>5)&63)*255/63);
            row[x*3+2]=(uint8_t)((p&31)*255/31);
        }
        CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);
    }
    CHECK(std::fclose(f)==0);
}
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static SaveData snapshot(){SaveData b{};CHECK(validate(a.game));CHECK(save(a.game,b));return b;}
static void unchanged(const SaveData&b){auto now=snapshot();CHECK(b.size==now.size&&!std::memcmp(b.bytes,now.bytes,b.size));}
static void put(std::vector<uint8_t>&b,unsigned at,unsigned value,unsigned width=4){for(unsigned i=0;i<width;i++)b[at+i]=(uint8_t)(value>>(8*i));}
// Deliberately synthetic: a single flat RGB portrait and ASCII placeholder text.
// This tests the current parser/render/input integration, never original art.
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned minimum_images=CardKinds>A_Count+160?CardKinds:A_Count+160;
    constexpr unsigned images=minimum_images>U_Count?minimum_images:U_Count,strings=ContentStrings;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16;
    constexpr unsigned payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<strings;i++){
        const char*text=i<CardKinds*3?(i%3?"Damage {d}; Block {b}; Draw {n}; Power {p};":definitions[i/3].key):"Synthetic UI text";
        put(b,stringdir+i*8,(unsigned)b.size());put(b,stringdir+i*8+4,(unsigned)std::strlen(text));
        b.insert(b.end(),text,text+std::strlen(text)+1);
    }
    for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;i++)for(unsigned j=0;j<4;j++)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,(unsigned)b.size());uint32_t hash=2166136261u;
    for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);
    return b;
}
static int append(Card card,Zone zone){
    auto&s=a.game;int id=s.card_count++;s.cards[id]=card;s.zones[id]=zone;
    auto&p=zone==Zone::Hand?s.hand:zone==Zone::Draw?s.draw:zone==Zone::Discard?s.discard:s.exhaust;
    p.ids[p.count++]=(uint16_t)id;return id;
}
static void setup(bool practice=true){
    new_run(a.game,71,false);auto&s=a.game;s.practice=practice;s.phase=Phase::Combat;
    s.current_node=0;s.floor=s.turn=1;s.enemy_count=2;s.energy=20;s.deck_count=1;s.deck[0]={Strike,0};
    s.enemies[0]=s.enemies[1]={46,46,0,0,0,0};
    for(int i=0;i<18;i++)append({i%2?Strike:Defend,0},Zone::Draw);
    append({Strike,0},Zone::Discard);append({Defend,0},Zone::Discard);
    a.title=a.paused=a.help=a.library=a.character_select=a.map_preview.open=false;
    a.library_tab=a.library_page=a.library_selected=a.library_filter=a.library_sort=0;a.library_upgraded=false;
    a.reset_presentation();a.callbacks={save_cb,load_cb};a.notice_ticks=0;CHECK(validate(s));
}
static bool exists(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value)return true;return false;}
static HitRegion hit(HitKind kind,int value){
    a.render();for(int i=0;i<a.hit_count;i++)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];
    std::fprintf(stderr,"missing hit %d/%d\n",(int)kind,value);CHECK(false);return {};
}
static void tap(HitKind kind,int value){
    auto h=hit(kind,value);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);
}
static void action(UiAction action){tap(HitKind::UiAction,(int)action);}
static void open_model(unsigned model,bool upgraded=false){
    if(!a.library)a.input(Key::Collection);action(UiAction::LibraryCards);
    action(card_colorless(model)?UiAction::FilterColorless:UiAction::FilterAll);
    CardKind ids[CardKinds];unsigned count=a.library_items(ids),index=0;
    while(index<count&&ids[index]!=model)++index;CHECK(index<count);
    while(a.library_page<(int)index/8)action(UiAction::NextPage);
    tap(HitKind::LibraryItem,(int)index);CHECK(card_from_token(a.detail_card).kind==model);
    if((card_from_token(a.detail_card).upgraded!=0)!=upgraded)action(UiAction::ToggleCardUpgrade);
}
static void open_potion(Potion potion){
    if(!a.library)a.input(Key::Collection);action(UiAction::LibraryPotions);
    int index=(int)potion-1;while(a.library_page<index/18)action(UiAction::NextPage);
    tap(HitKind::LibraryItem,index);CHECK(a.detail_item==RelicKinds+index);
}
static void acquire_potion(Potion potion){open_potion(potion);action(UiAction::LabPotion);CHECK(!a.library&&a.game.potions[0]==potion);}
static void drink(){a.input(Key::Potions);CHECK(a.panel==4);a.input(Key::Confirm);CHECK(a.panel==0);}
static void modal_only(){
    a.render();for(int i=0;i<a.hit_count;i++)CHECK(a.hits[i].kind!=HitKind::Hand&&a.hits[i].kind!=HitKind::Enemy&&a.hits[i].kind!=HitKind::Battlefield);
}
static void acquisitions(){
    for(unsigned kind=0;kind<CardKinds;kind++)if(card_colorless(kind))for(unsigned up=0;up<2;up++){
        setup();auto before=snapshot();tap(HitKind::Command,(int)Key::Collection);CHECK(a.library);unchanged(before);
        open_model(kind,up!=0);modal_only();unchanged(before);int id=a.game.card_count,deck=a.game.deck_count;
        if(real_profile){
            auto portrait=a.content.image(a.content.card_image(kind));CHECK(portrait.pixels&&portrait.w==82&&portrait.h==60);
            char text[1024];a.card_description({(CardKind)kind,(uint8_t)up},-1,text,sizeof text);
            CHECK(*text&&!std::strchr(text,'{')&&!std::strchr(text,'}'));
        }
        if(kind==Discovery)capture(up?"discovery-upgrade":"discovery-base");
        action(UiAction::LabCard);CHECK(!a.library&&a.detail_card<0);CHECK(a.game.card_count==id+1&&a.game.deck_count==deck);
        CHECK(a.game.hand.count==1&&a.game.hand.ids[0]==id&&a.game.cards[id].kind==kind&&a.game.cards[id].upgraded==up);
        CHECK(a.game.zones[id]==Zone::Hand);CHECK(std::strstr(a.notice,"手牌"));CHECK(validate(a.game));++card_acquisitions;
    }
    CHECK(card_acquisitions==106);
    for(unsigned id=1;id<=PotionKinds;id++){
        setup();auto before=snapshot();open_potion((Potion)id);modal_only();unchanged(before);
        if(real_profile)CHECK(a.content.image(a.content.potion_image((Potion)id)).pixels);
        if((Potion)id==Potion::ColorlessPotion)capture("colorless-potion-detail");
        action(UiAction::LabPotion);CHECK(!a.library&&a.detail_item<0&&a.game.potions[0]==(Potion)id&&a.game.potions[1]==Potion::Empty);
        CHECK(validate(a.game));++potion_acquisitions;
    }
    CHECK(potion_acquisitions==64);
    setup(false);CHECK(exists(HitKind::Command,(int)Key::Map));CHECK(!exists(HitKind::Command,(int)Key::Collection));
}
static void acquisition_guards(){
    for(int mode=0;mode<5;mode++){
        setup(mode!=0);open_model(Discovery);
        if(mode==1)a.title=true;if(mode==2)a.paused=true;if(mode==3)a.help=true;if(mode==4)a.panel=1;
        auto before=snapshot();CHECK(!exists(HitKind::UiAction,(int)UiAction::LabCard));
        a.ui_action(UiAction::LabCard);a.input(Key::EndTurn);unchanged(before);
        a.detail_card=-1;open_potion(Potion::ColorlessPotion);before=snapshot();
        CHECK(!exists(HitKind::UiAction,(int)UiAction::LabPotion));a.ui_action(UiAction::LabPotion);a.input(Key::EndTurn);unchanged(before);
    }
    setup();for(int i=0;i<MaxHand;i++)append({Strike,0},Zone::Hand);
    open_model(Discovery);int id=a.game.card_count;action(UiAction::LabCard);
    CHECK(a.game.hand.count==MaxHand&&a.game.zones[id]==Zone::Discard);CHECK(std::strstr(a.notice,"弃牌堆"));CHECK(validate(a.game));
    setup();while(a.game.card_count<MaxCards)append({Strike,0},Zone::Discard);
    open_model(Discovery);auto before=snapshot();CHECK(!hit(HitKind::UiAction,(int)UiAction::LabCard).enabled);
    action(UiAction::LabCard);a.input(Key::EndTurn);unchanged(before);
    setup();a.game.potions[0]=Potion::Block;a.game.potions[1]=Potion::Fire;open_potion(Potion::ColorlessPotion);before=snapshot();
    CHECK(!hit(HitKind::UiAction,(int)UiAction::LabPotion).enabled);action(UiAction::LabPotion);a.input(Key::EndTurn);unchanged(before);
    setup();acquire_potion(Potion::ColorlessPotion);drink();CHECK(a.game.phase==Phase::ExtendedChoice);
    a.input(Key::Collection);open_model(Discovery);before=snapshot();CHECK(!exists(HitKind::UiAction,(int)UiAction::LabCard));
    a.ui_action(UiAction::LabCard);a.input(Key::EndTurn);unchanged(before);
}
static void offers(){
    const Potion offer_potions[]={Potion::AttackPotion,Potion::ColorlessPotion,Potion::PowerPotion,Potion::SkillPotion};
    for(Potion potion:offer_potions){
        setup();acquire_potion(potion);drink();CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::Offer);
        CHECK(a.game.expansion.choices.count==3);auto before=snapshot();modal_only();
        if(potion==Potion::ColorlessPotion)capture("colorless-potion-offer");
        CHECK(!hit(HitKind::UiAction,(int)UiAction::ConfirmChoice).enabled);action(UiAction::ConfirmChoice);unchanged(before);
        for(unsigned i=0;i<3;i++){
            int token=a.game.expansion.choices.ids[i];CHECK(token>=MaxCards&&token<MaxCards+3);
            if(potion==Potion::ColorlessPotion)CHECK(card_colorless(choice_card(a.game,token).kind));
            tap(HitKind::CombatChoice,token);CHECK(a.choice_selected_count==1&&a.choice_selected[0]==token);
            a.input(Key::Detail);CHECK((cost(choice_card(a.game,token))<0?a.detail_cost_override<0:a.detail_cost_override==0)&&a.detail_instance==-1);
            CHECK(card_from_token(a.detail_card).kind==choice_card(a.game,token).kind);modal_only();a.input(Key::Back);unchanged(before);
        }
        a.input(Key::Save);CHECK(stored.size);action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat&&a.game.hand.count==1);
        CHECK(cost(a.game,a.game.hand.ids[0])==(cost(a.game.cards[a.game.hand.ids[0]])<0?-1:0)&&!a.game.expansion.potion_active);CHECK(validate(a.game));
        a.input(Key::Load);CHECK(a.game.phase==Phase::ExtendedChoice&&a.choice_selected_count==0);unchanged(before);
        auto h=hit(HitKind::CombatChoice,a.game.expansion.choices.ids[0]);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);
        a.input(Key::Collection);a.pointer_button(false);CHECK(a.library&&a.choice_selected_count==0);unchanged(before);a.input(Key::Back);
        a.cursor=1;a.input(Key::Confirm);a.input(Key::EndTurn);CHECK(a.game.phase==Phase::Combat&&a.game.hand.count==1);
    }
    setup();open_model(Discovery);action(UiAction::LabCard);a.input(Key::Confirm);CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.playing>=0);
    auto before=snapshot();tap(HitKind::CombatChoice,a.game.expansion.choices.ids[2]);unchanged(before);action(UiAction::ConfirmChoice);
    CHECK(a.game.phase==Phase::Combat&&a.game.hand.count==1&&a.game.expansion.cards_finished==1);CHECK(validate(a.game));
}
static void free_x_costs(){
    for(unsigned kind:{(unsigned)Whirlwind,(unsigned)Volley}){
        setup();acquire_potion(kind==Volley?Potion::ColorlessPotion:Potion::AttackPotion);drink();
        CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::Offer);
        a.game.expansion.offers[0]={(CardKind)kind,0};auto before=snapshot();
        tap(HitKind::CombatChoice,MaxCards);if(kind==Volley)capture("x-cost-offer");
        a.input(Key::Detail);CHECK(card_from_token(a.detail_card).kind==kind&&a.detail_cost_override<0);
        a.input(Key::Back);unchanged(before);action(UiAction::ConfirmChoice);
        CHECK(a.game.phase==Phase::Combat&&a.game.hand.count==1&&cost(a.game,a.game.hand.ids[0])==-1);
        setup();int id=append({(CardKind)kind,0},Zone::Discard);
        acquire_potion(Potion::LiquidMemories);drink();before=snapshot();tap(HitKind::CombatChoice,id);
        a.input(Key::Detail);CHECK(a.detail_instance==id&&a.detail_cost_override<0);
        a.input(Key::Back);unchanged(before);action(UiAction::ConfirmChoice);
        CHECK(a.game.phase==Phase::Combat&&a.game.hand.ids[0]==id&&cost(a.game,id)==-1);CHECK(validate(a.game));
    }
}
static void pile_choices_and_instances(){
    setup();for(int i=0;i<26;i++)append({Strike,0},Zone::Discard);
    acquire_potion(Potion::LiquidMemories);drink();CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::Discard);
    int id=a.game.expansion.choices.ids[22];auto before=snapshot();modal_only();
    action(UiAction::NextChoicePage);action(UiAction::NextChoicePage);CHECK(a.cursor==20);
    tap(HitKind::CombatChoice,id);a.input(Key::Detail);CHECK(a.detail_instance==id&&a.detail_cost_override==0);
    a.input(Key::Back);unchanged(before);action(UiAction::PreviousChoicePage);CHECK(a.choice_selected_count==1&&a.choice_selected[0]==id);unchanged(before);
    action(UiAction::ConfirmChoice);CHECK(a.game.phase==Phase::Combat&&a.game.hand.ids[0]==id&&cost(a.game,id)==0);CHECK(validate(a.game));
    setup();int left=append({SovereignBlade,0},Zone::Hand),right=append({SovereignBlade,0},Zone::Hand);
    a.game.expansion.blade_damage[left]=25;a.game.expansion.blade_damage[right]=47;a.game.expansion.turn_cost[left]=1;
    before=snapshot();a.cursor=0;a.input(Key::Detail);CHECK(a.detail_instance==left&&a.detail_cost_override==0);
    char text[256];a.card_description(a.game.cards[left],0,text,sizeof text,a.detail_instance);CHECK(number_in(text,25));capture("blade-instance-25");
    a.input(Key::Back);a.cursor=1;a.input(Key::Detail);CHECK(a.detail_instance==right&&a.detail_cost_override==2&&a.detail_energy()==2);
    a.card_description(a.game.cards[right],0,text,sizeof text,a.detail_instance);CHECK(number_in(text,47));capture("blade-instance-47");
    a.input(Key::Back);unchanged(before);open_model(SovereignBlade);a.card_description({SovereignBlade,0},0,text,sizeof text);
    CHECK(number_in(text,10));capture("blade-catalog");unchanged(before);
}
static void statuses(){
    struct Case{Potion potion;int actor,status,amount;};
    const Case cases[]={
        {Potion::LuckyTonic,0,20,1},{Potion::GhostInAJar,0,21,1},{Potion::RegenPotion,0,23,5},
        {Potion::Clarity,0,24,3},{Potion::RadiantTincture,0,25,3},{Potion::Duplicator,0,27,1},
        {Potion::HeartOfIron,0,19,7},{Potion::PowderedDemise,1,40,9},{Potion::BeetleJuice,1,41,4},
        {Potion::PotionOfDoom,1,42,33},{Potion::StarPotion,0,44,3},{Potion::BoneBrew,0,45,15}
    };
    for(auto test:cases){
        setup();acquire_potion(test.potion);drink();CHECK(status_amount(a.game,test.actor,test.status)==test.amount);
        auto before=snapshot();a.input(Key::Statuses);if(test.actor)a.input(Key::Right);CHECK(a.status_actor==test.actor);modal_only();
        int ids[StatusKinds],count=collect_statuses(a.game,test.actor,ids),selected=0;
        while(selected<count&&ids[selected]!=test.status)++selected;CHECK(selected<count);a.status_selected=selected;
        if(test.potion==Potion::Clarity)capture("clarity-status");
        tap(HitKind::StatusItem,selected);a.input(Key::EndTurn);a.input(Key::Collection);CHECK(!a.library&&a.status_actor==test.actor);unchanged(before);
        a.input(Key::Back);CHECK(a.status_actor<0);unchanged(before);
    }
    setup();open_model(TheBomb);action(UiAction::LabCard);a.input(Key::Confirm);CHECK(status_amount(a.game,0,43)==1);
    auto before=snapshot();a.input(Key::Statuses);a.render();unchanged(before);a.input(Key::Back);
}
static void shop_and_rewards(){
    for(unsigned kind=0;kind<CardKinds;kind++)if(card_colorless(kind)){
        setup(false);auto&s=a.game;s.phase=Phase::Map;s.current_node=1;s.next_row=1;s.gold=9999;CHECK(choose_node(s,1));
        s.shop_cards[5]=(CardKind)kind;a.reset_presentation();action(UiAction::OpenShop);a.cursor=5;
        auto before=snapshot();a.input(Key::Detail);CHECK(card_from_token(a.detail_card).kind==kind);a.input(Key::Back);unchanged(before);
        int gold=s.gold,count=s.deck_count,price=s.shop_prices[5];tap(HitKind::Choice,5);
        CHECK(s.deck_count==count+1&&s.deck[count].kind==kind&&s.gold==gold-price&&(s.shop_sold&(1u<<5)));CHECK(validate(s));
    }
    for(unsigned id=1;id<=PotionKinds;id++){
        setup(false);auto&s=a.game;s.phase=Phase::Reward;s.reward_potion=(Potion)id;s.reward_claimed=0;a.reset_presentation();
        action(UiAction::RewardPotion);CHECK(s.potions[0]==(Potion)id&&(s.reward_claimed&4));auto before=snapshot();
        action(UiAction::RewardPotion);unchanged(before);
    }
}
int main(int argc,char** argv){
    CHECK(argc<=3);std::vector<uint8_t> content;
    if(argc>1){
        real_profile=true;FILE* f=std::fopen(argv[1],"rb");CHECK(f);
        CHECK(std::fseek(f,0,SEEK_END)==0);long size=std::ftell(f);CHECK(size>=40&&size<=MaxPack);
        CHECK(std::fseek(f,0,SEEK_SET)==0);content.resize((size_t)size);
        CHECK(std::fread(content.data(),1,content.size(),f)==content.size());CHECK(std::fclose(f)==0);
    }else content=synthetic_profile();
    if(argc>2)screenshot_directory=argv[2];
    CHECK(a.init(content.data(),content.size(),{save_cb,load_cb}));
    acquisitions();acquisition_guards();offers();free_x_costs();pile_choices_and_instances();statuses();shop_and_rewards();
    std::printf("expansion UI: %u checks; %u colorless variants acquired; %u potions acquired; %s NSP2/11 profile\n",checks,card_acquisitions,potion_acquisitions,real_profile?"supplied private":"synthetic");
}
