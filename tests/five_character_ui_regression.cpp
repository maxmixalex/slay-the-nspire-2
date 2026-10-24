#include "app.hpp"
#include "card_grid_fixture.hpp"
#include "content_fixture.hpp"
#include "status.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
static App a;
static SaveData stored;
static unsigned checks;
static bool real_profile;
static const char* screenshots;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static bool save_cb(const State&s){return save(s,stored);}
static bool load_cb(State&s){return load(s,stored.bytes,stored.size);}
static void put(std::vector<uint8_t>&b,unsigned at,unsigned n,unsigned bytes=4){for(unsigned i=0;i<bytes;i++)b[at+i]=(uint8_t)(n>>(8*i));}
static std::vector<uint8_t> synthetic_profile(){
    constexpr unsigned images=CardKinds>U_Count?CardKinds:U_Count,strings=ContentStrings;
    constexpr unsigned stringdir=40+images*16,clipdir=stringdir+strings*8;
    constexpr unsigned carddir=clipdir+AnimationKinds*16,uidir=carddir+CardKinds*16,payload=uidir+U_Count*4;
    std::vector<uint8_t>b(payload+82*60*2);
    put(b,0,0x3250534e);put(b,4,ContentVersion);put(b,8,0x41cef1ea);put(b,16,images);
    put(b,24,strings);put(b,28,AnimationKinds);put(b,32,CardKinds);
    for(unsigned i=0;i<images;i++){put(b,40+i*16,payload);put(b,44+i*16,82,2);put(b,46+i*16,60,2);put(b,52+i*16,82*60*2);}
    for(unsigned i=0;i<strings;i++){
        unsigned model=i/3;const char*text=i<CardKinds*3?(i%3?"Synthetic card description.":definitions[model].key):"Synthetic UI text.";
        if(model==DefectGeneticAlgorithm&&i%3)text="Block {Block=1}.";
        if(model==NecroTheScythe&&i%3)text="Damage {Damage=13}.";
        if(model==NecroNoEscape&&i%3)text="NoEscape.\x1c\nTarget {CalculatedDoom=10}.\x1d";
        if(model==NecroTimesUp&&i%3)text="TimesUp.\x1c\nTarget {CalculatedDamage=0}.\x1d";
        put(b,stringdir+i*8,(unsigned)b.size());put(b,stringdir+i*8+4,(unsigned)std::strlen(text));b.insert(b.end(),text,text+std::strlen(text)+1);
    }
    for(unsigned i=0;i<AnimationKinds;i++){put(b,clipdir+i*16+4,1,2);put(b,clipdir+i*16+6,33,2);}
    for(unsigned i=0;i<CardKinds;i++)for(unsigned j=0;j<4;j++)put(b,carddir+i*16+j*4,j?i*3+j-1:i);
    for(unsigned i=0;i<U_Count;i++)put(b,uidir+i*4,i==U_CampFlames?A_Count:i);
    put(b,12,(unsigned)b.size());uint32_t hash=2166136261u;for(unsigned i=40;i<b.size();i++){hash^=b[i];hash*=16777619u;}put(b,36,hash);return b;
}
static HitRegion hit(HitKind kind,int value){
    a.render();for(int i=a.hit_count-1;i>=0;i--)if(a.hits[i].kind==kind&&a.hits[i].value==value)return a.hits[i];
    std::fprintf(stderr,"missing hit %d/%d phase=%d detail=%d\n",(int)kind,value,(int)a.game.phase,a.detail_card);CHECK(false);return {};
}
static void tap(HitKind kind,int value){auto h=hit(kind,value);CHECK(h.enabled);a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);}
// Advance the same clock and render synchronization used by the player.
// In particular, clearing busy cannot end a queued combat presentation tail.
static void wait_action_ready(){
    for(unsigned frame=0;a.combat_actions_locked()&&frame<1800;++frame){a.advance(33);a.render();}
    CHECK(!a.combat_actions_locked());
}
static void wait_choice_ready(){
    for(unsigned frame=0;!a.combat_choice_ready()&&frame<1800;++frame){a.advance(33);a.render();}
    CHECK(a.combat_choice_ready());
}
static void action(UiAction action){tap(HitKind::UiAction,(int)action);}
static void settle_fixture_preparation(){
    a.render();for(unsigned frame=0;a.combat_preparation.active&&frame<1024;++frame){a.advance(33);a.render();}
    CHECK(!a.combat_preparation.active);
}
static void fixture_card(){wait_action_ready();CHECK(a.detail_card>=0&&lab_card(a.game,card_from_token(a.detail_card)));a.library=false;a.reset_presentation();settle_fixture_preparation();}
static SaveData snapshot(){SaveData s{};CHECK(validate(a.game));CHECK(save(a.game,s));return s;}
static void unchanged(const SaveData&before){auto after=snapshot();CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));}
static void capture(const char*name){
    if(!screenshots)return;a.render();std::string path=std::string(screenshots)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);
    CHECK(std::fprintf(f,"P6\n320 240\n255\n")>0);uint8_t row[320*3];
    for(int y=0;y<240;y++){for(int x=0;x<320;x++){uint16_t p=a.pixels[y*320+x];row[x*3]=(uint8_t)(((p>>11)&31)*255/31);row[x*3+1]=(uint8_t)(((p>>5)&63)*255/63);row[x*3+2]=(uint8_t)((p&31)*255/31);}CHECK(std::fwrite(row,1,sizeof row,f)==sizeof row);}CHECK(!std::fclose(f));
}
static void start(Character character){
    new_run(a.game,71,true,character);a.title=a.paused=a.help=a.library=a.character_select=a.map_preview.open=a.settings_open=a.compendium_open=a.statistics_open=a.history_open=false;
    a.selected_character=character;a.reset_presentation();
    a.library_tab=a.library_page=a.library_selected=a.library_filter=a.library_sort=0;a.library_upgraded=false;
    CHECK(a.game.character==character&&a.game.phase==Phase::Combat&&a.game.practice);CHECK(validate(a.game));settle_fixture_preparation();
}
static void clear_hand(){auto&s=a.game;while(s.hand.count){unsigned id=s.hand.ids[--s.hand.count];s.discard.ids[s.discard.count++]=(uint16_t)id;s.zones[id]=Zone::Discard;}CHECK(validate(s));}
static int add(Card card){
    wait_action_ready();if(a.game.hand.count==MaxHand)clear_hand();unsigned id=a.game.card_count;
    a.library=true;a.open_card_detail(card);fixture_card();
    CHECK(!a.library&&a.detail_card<0&&a.game.card_count==id+1);CHECK(a.game.cards[id].kind==card.kind&&a.game.cards[id].upgraded==card.upgraded);
    CHECK(!a.game.effect_count&&a.game.playing<0&&validate(a.game));return (int)id;
}
static void play_ui(int id){
    int index=-1;for(int i=0;i<a.game.hand.count;i++)if(a.game.hand.ids[i]==id)index=i;CHECK(index>=0);
    wait_action_ready();a.cursor=index;a.input(Key::Confirm);a.input(Key::Confirm);a.render();
}
static int deck_index(CardKind kind){for(int i=0;i<a.game.deck_count;i++)if(a.game.deck[i].kind==kind)return i;CHECK(false);return -1;}
static int selected_status(){int ids[StatusKinds];int n=collect_statuses(a.game,a.status_actor,ids);CHECK(a.status_selected>=0&&a.status_selected<n);return ids[a.status_selected];}
static void directory_and_characters(){
    const int filters[]={1,2,4,5,6};
    for(unsigned character=0;character<CharacterKinds;character++){
        start((Character)character);a.input(Key::Collection);action(UiAction::LibraryCards);CHECK(spire_test::choose_library_pool(a,filters[character]));
        CardKind ids[CardKinds];unsigned n=a.library_items(ids);const unsigned expected[]={85,86,86,86,86};CHECK(n==expected[character]);
        for(unsigned i=0;i<n;i++)CHECK(card_in_pool(ids[i],(Character)character)||(character==1&&ids[i]==Shiv)||ids[i]>=EventCardBadLuck);
        auto before=snapshot();a.render();unchanged(before);
        if(character==2)capture("defect-library");if(character==3)capture("regent-library");if(character==4)capture("necro-library");
        a.input(Key::Back);a.input(Key::Back);a.character_select=true;
        if(character==2)capture("defect-character");if(character==3)capture("regent-character");if(character==4)capture("necro-character");
    }
    a.selected_character=Character::Ironclad;a.input(Key::Left);CHECK(a.selected_character==Character::Necrobinder);a.input(Key::Right);CHECK(a.selected_character==Character::Ironclad);
    start(Character::Regent);a.input(Key::Collection);action(UiAction::LibraryCards);const int debris=spire_test::library_model_index(a,Debris);CHECK(debris>=0);
    a.library_upgraded=true;a.select_library_item(debris);CHECK(card_from_token(a.detail_card).kind==Debris&&!card_from_token(a.detail_card).upgraded);
    a.render();for(int i=0;i<a.hit_count;++i)CHECK(a.hits[i].kind!=HitKind::UiAction||a.hits[i].value!=int(UiAction::ToggleCardUpgrade));
    auto before=snapshot();a.input(Key::Confirm);unchanged(before);CHECK(card_from_token(a.detail_card).kind==Debris&&!card_from_token(a.detail_card).upgraded);
    fixture_card();CHECK(a.game.cards[a.game.card_count-1].kind==Debris);
    // All eleven generated/status models render correctly from explicit fixtures.
    for(CardKind kind=Dazed;kind<CardKinds;kind++)for(int up=0;up<=(card_upgradeable({kind,0})?1:0);up++){
        int id=add({kind,(uint8_t)up});a.open_choice_detail(id);CHECK(card_from_token(a.detail_card).kind==kind&&card_from_token(a.detail_card).upgraded==up);
        before=snapshot();a.render();unchanged(before);a.input(Key::Back);
    }
}
static void stars_and_upgraded_transform(){
    start(Character::Regent);clear_hand();int pulse=add({RegentAstralPulse,0});a.game.expansion.stars=2;
    CHECK(!hit(HitKind::Hand,pulse).enabled);a.open_choice_detail(pulse);CHECK(a.detail_energy()==0&&a.detail_stars()==3);capture("regent-star-cost");a.input(Key::Back);
    auto before=snapshot();RelicMask flash=a.game.relic_flash;CHECK(flash&DivineRight);
    play_ui(pulse);unchanged(before);CHECK(a.game.zones[pulse]==Zone::Hand&&a.game.relic_flash==flash);
    a.game.expansion.stars=3;CHECK(hit(HitKind::Hand,pulse).enabled);play_ui(pulse);CHECK(a.game.expansion.stars==0);
    int dust=add({RegentStardust,1});a.open_choice_detail(dust);CHECK(a.detail_stars()==-1);capture("regent-star-x");a.input(Key::Back);
    a.game.energy=10;int form=add({RegentVoidForm,1});play_ui(form);CHECK(a.game.phase==Phase::Combat&&a.game.turn==2);
    pulse=add({RegentAstralPulse,0});a.open_choice_detail(pulse);CHECK(a.detail_energy()==0&&a.detail_stars()==0);a.input(Key::Back);
    dust=add({RegentStardust,0});a.open_choice_detail(dust);CHECK(a.detail_stars()==-1);a.input(Key::Back);capture("regent-void-form");
    start(Character::Regent);clear_hand();a.game.energy=10;int target=add({RegentKinglyKick,0}),begone=add({RegentBegone,1});play_ui(begone);
    CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::RTransformHand&&a.game.expansion.choice_free==1);
    a.open_choice_detail(target);CHECK(a.detail_energy()==cost(a.game,target)&&a.detail_energy()>0);a.input(Key::Back);
    wait_choice_ready();tap(HitKind::CombatChoice,target);capture("regent-minion-choice");action(UiAction::ConfirmChoice);
    CHECK(a.game.cards[target].kind==MinionStrike&&a.game.cards[target].upgraded);CHECK(validate(a.game));
    // A local zero Set still passes through BorrowedTime. Full-resource free
    // selections must preview the resulting energy and the independent stars.
    start(Character::Regent);clear_hand();a.game.energy=10;target=add({RegentReflect,0});
    play_ui(add({NecroBorrowedTime,0}));a.game.potions[0]=Potion::TouchOfInsanity;
    wait_action_ready();a.input(Key::Potions);CHECK(a.hud_focus==2&&!a.panel);a.input(Key::Confirm);CHECK(a.panel==4);a.input(Key::Confirm);
    CHECK(a.game.phase==Phase::ExtendedChoice&&a.game.expansion.choice==XChoice::Free);
    a.open_choice_detail(target);CHECK(a.detail_energy()==1&&a.detail_stars()==0);
    before=snapshot();capture("regent-free-resource-choice");unchanged(before);a.input(Key::Back);
    wait_choice_ready();tap(HitKind::CombatChoice,target);action(UiAction::ConfirmChoice);
    CHECK(cost(a.game,target)==1&&card_star_cost(a.game,a.game.cards[target],target)==0);
    CHECK(a.game.expansion.star_combat_free[target]&&validate(a.game));
}
static void orbs_and_osty(){
    start(Character::Defect);auto&s=a.game;s.expansion.orb_slots=10;s.expansion.orb_count=5;s.expansion.powers[(int)XPower::Focus]=-2;
    for(int i=0;i<MaxOrbs;i++){s.defect.orb_kind[i]=(uint8_t)(i<5?i:0);s.expansion.dark[i]=(int16_t)(i==0?23:i==4?4:0);}
    auto before=snapshot();capture("defect-five-orbs");action(UiAction::ShowOrbs);CHECK(selected_status()==46&&status_amount(s,0,39)==-2);capture("defect-orb-detail");
    action(UiAction::MoreStatus);CHECK(a.status_scroll==5);capture("defect-empty-orb-slots");a.input(Key::Confirm);CHECK(a.status_scroll==0);unchanged(before);a.input(Key::Back);
    start(Character::Necrobinder);clear_hand();a.game.energy=10;CHECK(a.game.expansion.osty_hp==1);
    int guard=add({NecroBodyguard,0});play_ui(guard);CHECK(a.game.expansion.osty_hp==6&&a.game.expansion.osty_max_hp==6);
    action(UiAction::ShowOsty);CHECK(selected_status()==45);capture("necro-osty-detail");a.input(Key::Back);
    int shards=add({NecroBoneShards,0});play_ui(shards);CHECK(a.game.expansion.osty_hp==0&&a.osty_motion.clip==AnimOstyDie);capture("necro-osty-down");
    guard=add({NecroBodyguard,0});play_ui(guard);CHECK(a.game.expansion.osty_hp==5&&a.osty_motion.clip==AnimOstyRevive);
    a.game.necro.player_doom=7;a.open_statuses(0);CHECK(status_amount(a.game,0,42)==7);before=snapshot();a.render();unchanged(before);a.input(Key::Back);capture("necro-osty-revived");
}
static void permanent_card_previews(){
    for(Character character:{Character::Defect,Character::Necrobinder}){
        start(character);CardKind kind=character==Character::Defect?DefectGeneticAlgorithm:NecroTheScythe;int index=deck_index(kind);
        if(character==Character::Defect)a.game.defect.genetic_deck[index]=12;else a.game.necro.scythe_deck[index]=27;
        a.game.strength=5;a.game.dexterity=7;a.game.enemies[0].vulnerable=2;
        a.panel=1;a.pile_tab=0;a.inspect_cursor=index;CHECK(spire_test::focus_card_grid(a));auto before=snapshot();tap(HitKind::DeckCard,index);
        CHECK(a.detail_deck_index==index&&a.detail_instance==-1&&card_from_token(a.detail_card).kind==kind);
        char text[1024]={};a.card_description(a.game.deck[index],0,text,sizeof text,-1,index);CHECK(std::strstr(text,character==Character::Defect?"13":"40"));
        CHECK(!std::strchr(text,'{'));action(UiAction::ToggleCardUpgrade);a.card_description(card_from_token(a.detail_card),0,text,sizeof text,-1,index);
        CHECK(std::strstr(text,character==Character::Defect?"13":"40"));unchanged(before);capture(character==Character::Defect?"genetic-deck-preview":"scythe-deck-preview");
        a.input(Key::Back);a.input(Key::Back);a.input(Key::Collection);action(UiAction::LibraryCards);a.open_card_detail({kind,0});a.card_description({kind,0},0,text,sizeof text,-1,index);
        CHECK(!std::strstr(text,character==Character::Defect?"13":"40"));a.input(Key::Back);a.input(Key::Back);
    }
}
static void target_conditional_previews(){
    for(CardKind kind:{NecroNoEscape,NecroTimesUp}){
        start(Character::Necrobinder);clear_hand();Card card{kind,0};int id=add(card);
        a.game.expansion.doom[0]=23;auto before=snapshot();char text[1024]={};
        a.card_description(card,0,text,sizeof text,id);
        CHECK(std::strchr(text,'\n')&&std::strstr(text,kind==NecroNoEscape?"20":"23")&&!std::strchr(text,'{'));
        a.game.expansion.doom[0]=40;a.card_description(card,0,text,sizeof text,id);
        CHECK(std::strstr(text,kind==NecroNoEscape?"30":"40"));a.game.expansion.doom[0]=23;
        for(int target:{-1,(int)a.game.enemy_count}){a.card_description(card,target,text,sizeof text,id);CHECK(!std::strchr(text,'\n')&&!std::strchr(text,'{'));}
        int hp=a.game.enemies[0].hp;a.game.enemies[0].hp=0;a.card_description(card,0,text,sizeof text,id);CHECK(!std::strchr(text,'\n'));a.game.enemies[0].hp=(int16_t)hp;
        a.library=true;a.card_description(card,0,text,sizeof text,id);CHECK(!std::strchr(text,'\n'));a.library=false;
        Card original=a.game.deck[0];a.game.deck[0]=card;a.card_description(card,0,text,sizeof text,-1,0);CHECK(!std::strchr(text,'\n'));a.game.deck[0]=original;
        unchanged(before);
    }
}
static void grimoire_and_relic_width(){
    start(Character::Necrobinder);clear_hand();a.game.energy=10;
    play_ui(add({NecroForbiddenGrimoire,0}));play_ui(add({NecroForbiddenGrimoire,1}));
    for(int i=0;i<a.game.enemy_count;i++)a.game.enemies[i].hp=1;play_ui(add({DefectSweepingBeam,0}));
    CHECK(a.game.phase==Phase::Reward&&a.game.necro.grimoire_pending==2);capture("necro-grimoire-reward");action(UiAction::RewardGrimoire);
    int eternal=deck_index(NecroForbiddenGrimoire);a.cursor=eternal;CHECK(spire_test::focus_card_grid(a));CHECK(!hit(HitKind::Choice,eternal).enabled);auto before=snapshot();a.input(Key::Confirm);CHECK(a.deck_candidate<0);unchanged(before);
    int index=deck_index(NecroTheScythe);a.cursor=index;CHECK(spire_test::focus_card_grid(a));tap(HitKind::Choice,index);CHECK(a.deck_candidate_valid());capture("necro-grimoire-confirm");before=snapshot();action(UiAction::CancelDeckChoice);unchanged(before);
    a.cursor=index;tap(HitKind::Choice,index);unsigned deck=a.game.deck_count;int gold=a.game.gold;action(UiAction::ConfirmDeckChoice);
    CHECK(a.game.deck_count==deck-1&&a.game.gold==gold&&a.game.necro.grimoire_pending==1&&a.grimoire_select);
    deck=a.game.deck_count;action(UiAction::SkipGrimoire);CHECK(a.game.deck_count==deck&&!a.game.necro.grimoire_pending&&!a.grimoire_select);CHECK(validate(a.game));
    start(Character::Ironclad);CHECK(gain_relic(a.game,BoundPhylactery));a.sync_relic_feedback();CHECK(a.relic_flash_mask&BoundPhylactery);
    int owned=-1;for(int i=0;i<a.game.relic_count;i++)if(relic_definitions[a.game.relic_order[i]].id==BoundPhylactery)owned=i;
    CHECK(owned>=0);before=snapshot();tap(HitKind::OwnedRelic,owned);CHECK(a.detail_item==32);unchanged(before);
}
static void complete_status_directory(){
    start(Character::Defect);auto&s=a.game;
    // Presentation mappings can be tested independently of acquisition order.
    for(int i=0;i<DPowerCount;i++)s.defect.powers[i]=3;
    for(int i=0;i<RPowerCount;i++)s.regent.powers[i]=4;
    for(int i=0;i<NPowerCount;i++)s.necro.powers[i]=5;
    for(int i=0;i<DPowerCount;i++)CHECK(status_amount(s,0,DefectStatusStart+i)==3);
    for(int i=0;i<RPowerCount;i++)CHECK(status_amount(s,0,RegentStatusStart+i)==4);
    for(int i=0;i<NPowerCount;i++)CHECK(status_amount(s,0,NecroStatusStart+i)==5);
    s.regent.conqueror[0]=2;s.necro.debilitate[0]=3;s.necro.hang[0]=4;s.necro.oblivion[0]=5;s.necro.sic_em[0]=6;
    for(int i=0;i<5;i++)CHECK(status_amount(s,1,FiveCharacterExtraStatusStart+i)==i+2);
    a.open_statuses(0);int n=collect_statuses(s,0,nullptr);CHECK(n>=DPowerCount+RPowerCount+NPowerCount);
    for(int i=0;i<n;i++){a.status_selected=i;a.render();CHECK(selected_status()>=0);if(real_profile)CHECK(a.content.image(a.content.ui_image(status_definitions[selected_status()].image)).pixels);}
    capture("five-character-statuses");a.input(Key::Back);
}
int main(int argc,char**argv){
    std::vector<uint8_t>b;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);b.resize(spire_test::pack_size(f));size_t n=std::fread(b.data(),1,b.size(),f);CHECK(!std::ferror(f));std::fclose(f);b.resize(n);real_profile=true;}else b=synthetic_profile();
    if(argc>2)screenshots=argv[2];CHECK(a.init(b.data(),b.size(),{save_cb,load_cb}));
    directory_and_characters();stars_and_upgraded_transform();orbs_and_osty();permanent_card_previews();target_conditional_previews();grimoire_and_relic_width();complete_status_directory();
    std::printf("five-character UI: %u checks passed; %s profile\n",checks,real_profile?"actual":"synthetic");
}
