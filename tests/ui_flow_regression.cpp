#include "app.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static unsigned checks=0;static App a;
#define CHECK(x) do{++checks;if(!(x)){fprintf(stderr,"FAIL line %d %s\n",__LINE__,#x);exit(1);}}while(0)
static SaveData before;
static void snapshot(){CHECK(save(a.game,before));}
static void unchanged(){SaveData b{};CHECK(save(a.game,b));CHECK(b.size==before.size&&!memcmp(b.bytes,before.bytes,b.size));}
static void init(){a=App{};new_run(a.game,7,true);a.title=false;a.render();}
static bool findclick(HitKind kind,int value){a.render();for(int i=0;i<a.hit_count;i++){auto h=a.hits[i];if(h.kind==kind&&h.value==value){a.pointer_move(h.x+h.w/2,h.y+h.h/2);a.pointer_button(true);a.pointer_button(false);return true;}}return false;}
static void action(UiAction x){CHECK(findclick(HitKind::UiAction,(int)x));}
int main(){
    init();snapshot();a.title=true;a.menu=0;a.input(Key::Confirm);CHECK(a.character_select);unchanged();a.input(Key::Back);CHECK(!a.character_select);unchanged();
    a.menu=2;a.input(Key::Confirm);CHECK(a.library);unchanged();
    for(int tab=0;tab<3;tab++){
        action((UiAction)((int)UiAction::LibraryCards+tab));CHECK(a.library_tab==tab);
        int total=tab==0?CardKinds:tab==1?RelicKinds:PotionKinds;
        for(int i=0;i<total;i++){a.library_selected=i;a.library_page=tab==0?i/8:i/18;a.render();CHECK(findclick(HitKind::LibraryItem,i));
            if(tab==0){CHECK(card_from_token(a.detail_card).kind==i);action(UiAction::ToggleCardUpgrade);CHECK(card_from_token(a.detail_card).upgraded==card_upgradeable({(CardKind)i,0}));a.input(Key::Back);}else{CHECK(a.detail_item>=0);a.input(Key::Back);}unchanged();}
    }
    a.input(Key::Back);CHECK(!a.library);unchanged();
    a.title=false;a.game.phase=Phase::Reward;a.game.reward_claimed=0;a.game.last_gold=20;a.game.reward_potion=Potion::Fruit;a.game.reward[0]=Offering;a.game.reward[1]=Strike;a.game.reward[2]=Bludgeon;
    int gold=a.game.gold;action(UiAction::RewardGold);CHECK(a.game.gold==gold+20);action(UiAction::RewardGold);CHECK(a.game.gold==gold+20);
    action(UiAction::RewardCards);CHECK(a.reward_cards);a.input(Key::Detail);CHECK(a.detail_card==Offering);a.input(Key::Back);CHECK(a.reward_cards);CHECK(findclick(HitKind::Choice,0));CHECK(!a.reward_cards&&(a.game.reward_claimed&2));
    action(UiAction::RewardPotion);CHECK(a.game.potions[0]==Potion::Fruit);snapshot();a.input(Key::Potions);a.render();CHECK(a.panel==4);action(UiAction::DiscardPotion);CHECK(a.game.potions[0]==Potion::Empty);a.input(Key::Back);
    action(UiAction::LeaveRewards);CHECK(a.game.phase==Phase::Complete);
    init();a.game.practice=0;a.game.current_node=1;a.game.next_row=1;a.game.phase=Phase::Map;a.game.gold=9999;CHECK(choose_node(a.game,1));a.ui_action(UiAction::OpenShop);a.render();
    for(int i=0;i<ShopRemove;i++){
        a.cursor=i;a.input(Key::Detail);CHECK(i<7?a.detail_card>=0:a.detail_item>=0);a.input(Key::Back);CHECK(a.game.phase==Phase::Shop);
        if(i>=ShopPotionStart){discard_potion(a.game,0);discard_potion(a.game,1);}CHECK(findclick(HitKind::Choice,i));CHECK(a.game.shop_sold&(1u<<i));
    }
    CHECK(findclick(HitKind::Choice,ShopRemove));CHECK(a.game.phase==Phase::Remove);a.input(Key::Back);CHECK(a.cursor==ShopRemove&&a.game.phase==Phase::Shop);
    init();snapshot();a.input(Key::Inspect);a.render();
    // Relic row must not draw/register over the pile tabs.
    const auto*tab=a.hit_at(20,30);CHECK(tab&&tab->kind==HitKind::PileTab);unchanged();
    a.pointer_move(37,70);a.pointer_button(true);a.input(Key::Back);a.pointer_button(false);CHECK(a.detail_card<0);unchanged();
    printf("ui flow: %u checks passed\n",checks);
}
