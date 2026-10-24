#include "app.hpp"
#include "enemy_command_timing.hpp"
#include "rich_text.hpp"
#include "status.hpp"
#include "combat_layout.hpp"
#include "event_assets.hpp"
#include "world_run.hpp"
#include "original_events.hpp"
#include "localized_template.hpp"
#include "ui_core_observer.hpp"
#include "world_room_assets.hpp"
#include "world_rest_actors.hpp"
#include "presentation_blend.hpp"
#include "world_card_factory.hpp"
#include "nine_patch.hpp"
#include "loading_screen.hpp"
#include "card_hud_assets.hpp"
#include "feedback_assets.hpp"
#include "card_face_layout.hpp"
#include "fitted_text.hpp"
namespace spire {
#include "world_enemy_assets.inc"
#include "character_ui.inc"
#include "combat_feedback_ui.inc"
#include "canvas.inc"
static void world_region_image_key(const State&s,char*out,size_t size){
 // Run models retain names such as Overgrowth; both original-image families
 // register uppercase keys. Normalize only at the presentation boundary.
 if(size)out[0]=0;TextBuffer b{out,size,0};
 for(const char*p=world_region_key(s);*p;++p)b.ch(*p>='a'&&*p<='z'?char(*p-'a'+'A'):*p);
}
#include "native_controls.inc"
#include "world_text.inc"
#include "ui_text.inc"
#include "world_enemy_ui.inc"
#include "turn_banner_ui.inc"
#include "combat_preparation.inc"
#include "world_map_motion.inc"
#include "app_input.inc"
#include "reward_presentation.inc"
static const char* card_type_label(CardType type,const Content*content=nullptr){
    const char*keys[]={"CARD_TYPE.ATTACK","CARD_TYPE.SKILL","CARD_TYPE.POWER","CARD_TYPE.STATUS","CARD_TYPE.CURSE","CARD_TYPE.QUEST"};
    if(content&&unsigned(type)<6){const char*text=content->world_string("gameplay_ui",keys[unsigned(type)]);if(*text)return text;}
    switch(type){case CardType::Attack:return "攻击";case CardType::Skill:return "技能";
    case CardType::Power:return "能力";case CardType::Status:return "状态";case CardType::Curse:return "诅咒";case CardType::Quest:return "任务";}
    return "";
}
#include "card_face_ui.inc"
static void small_card(Canvas&c,Card card,int x,int y,bool selected,bool playable,int number,int fee=-2,int stars=-2,int instance=-1,int deck=-1,const EventCardFacts*facts=nullptr){
    if(selected){c.rect(x-2,y-2,51,70,rgb(91,74,47));c.box(x-2,y-2,51,70,Gold);}
    draw_card_face(c,card,x,y,47,66,fee,stars,instance,deck,facts);
    if(number)c.num(number,x+36,y+53,8,Muted);
    if(!playable)c.dim(x,y,47,66,125);
}
static void log_line(App&a,Log l,char*b,size_t n){
    b[0]=0;TextBuffer t{b,n,0,&a.content};switch(l.kind){
    case LogKind::Turn:t.text("回合 ");t.number(l.a);break;
    case LogKind::Play:t.text("打出 ");t.text(a.content.card_string(l.a));if(l.b)t.ch('+');break;
    case LogKind::Damage:t.text("对敌人 ");t.number(l.b+1);t.text(" 造成 ");t.number(l.a);t.text(" 点伤害（含格挡）");break;
    case LogKind::Block:t.text("获得格挡 ");t.number(l.a);break;
    case LogKind::Exhaust:t.text("消耗 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Draw:t.text("抽牌 ");t.number(l.a);break;
    case LogKind::Shuffle:t.text("弃牌堆洗回抽牌堆");break;
    case LogKind::Enemy:t.text("敌人 ");t.number(l.a+1);t.text(l.b==0?"：撞击":l.b==1?"：切击 + 格挡":"：嘶鸣 + 力量");break;
    case LogKind::Strength:t.text("获得力量 ");t.number(l.a);break;
    case LogKind::Pain:t.text("无惧疼痛 ");t.number(l.a);break;
    case LogKind::Embrace:t.text("黑暗之拥 ");t.number(l.a);break;
    case LogKind::Heal:t.text("恢复生命 ");t.number(l.a);break;
    case LogKind::Reward:case LogKind::Buy:t.text(l.kind==LogKind::Buy?"购买 ":"获得 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Upgrade:t.text("升级 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Gold:t.text("获得金币 ");t.number(l.a);break;
    case LogKind::Remove:t.text("移除 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Transform:t.text("变化 ");t.text(a.content.card_string(l.a));t.text(" → ");t.text(a.content.card_string(l.b));break;
    case LogKind::Event:t.text("事件选择已结算");break;
    case LogKind::Relic:t.text("获得遗物：");if(auto*d=relic_definition((uint32_t)l.a))t.text(d->name);break;
    case LogKind::RelicIndex:t.text("遗物：");if(l.a>=0&&l.a<WorldRelicKinds)t.text(a.relic_title(RelicId(l.a)));break;
    case LogKind::Potion:t.text("使用药水");break;
    case LogKind::LoseHp:t.text("失去生命 ");t.number(l.a);break;
    case LogKind::Energy:t.text("获得能量 ");t.number(l.a);break;
    case LogKind::NoDraw:t.text("本回合不能抽牌");break;
    case LogKind::Weak:t.text("给予虚弱 ");t.number(l.a);break;
    case LogKind::PoisonDamage:t.text("中毒伤害 ");t.number(l.a);break;
    case LogKind::Poison:t.text("给予中毒 ");t.number(l.a);break;
    case LogKind::Discard:t.text("丢弃 ");t.text(a.content.card_string(l.a));break;
    case LogKind::Create:t.text("生成 ");t.text(a.content.card_string(l.b));t.text(" × ");t.number(l.a);break;
    case LogKind::Dexterity:t.text("获得敏捷 ");t.number(l.a);break;
    case LogKind::Noxious:t.text("毒雾 ");t.number(l.a);break;
    case LogKind::Accuracy:t.text("精准 ");t.number(l.a);break;
    default:t.text("容量上限：本局已安全停止");break;}
}
#include "relic_ui.inc"
static void hud(Canvas&c){
    auto&s=c.a.game;c.rect(0,0,320,22,Panel);c.ui(U_TopBar,0,0,320,22);
    // top_bar.tscn order: portrait, red HP, gold, recessed potion tray,
    // room/floor information, then the right-aligned navigation controls.
    c.nine_ui(U_TopBarBackdrop,1,0,24,22,7,6,7,6);
    int portrait=c.a.content.ui_image(U_TopBarIronclad+int(s.character));
    if(portrait<0)portrait=c.a.content.ui_image(character_portrait(s.character));
    c.picture(portrait,3,1,20,20);
    if(const unsigned ascension=world_ascension(s)){
        c.ui(U_TopBarAscension,13,10,7,11);c.num(int(ascension),19,10,8,Gold);
    }
    const auto health=c.a.combat_health(0);
    c.picture(A_Heart,27,4,12,13);char b[32]={};TextBuffer t{b,sizeof b,0,&c.a.content};t.number(health.hp);t.ch('/');t.number(health.max_hp);
    c.text_fitted(b,41,5,43,8,rgb(255,85,85));
    c.picture(A_Coin,86,4,12,13);b[0]=0;t.n=0;t.number(c.a.reward_gold());c.text_fitted(b,100,5,25,8,Gold);
    c.nine_ui(U_TopBarBackdrop,127,1,53,20,7,6,7,6);
    const int slots=potion_slot_count(s),width=slots>3?14:17;
    c.a.potion_page=uint8_t(mn(int(c.a.potion_page),(slots-1)/3));
    const int first=int(c.a.potion_page)*3,visible=mn(slots-first,3);
    for(int i=0;i<visible;i++){
        int x=128+i*width;const int slot=first+i;Potion potion=potion_at(s,slot);
        const unsigned age=c.a.map_clock_ms-c.a.potion_removed_start;
        const bool removing=c.a.potion_removed!=Potion::Empty&&c.a.potion_removed_slot==slot&&age<(c.a.potion_removed_discard?400u:200u);
        if(potion!=Potion::Empty){if(!c.a.reward_pickup_hidden(RewardPickupKind::Potion,slot))c.picture(c.a.content.potion_image(potion),x+1,2,width-3,18);}
        else if(c.a.content.ui_image(U_PotionPlaceholder)>=0){const unsigned alpha=age<200&&c.a.potion_removed_slot==slot?0:age<400&&c.a.potion_removed_slot==slot?(age-200)*255/200:255;c.ui(U_PotionPlaceholder,x+1,3,width-3,15,alpha);}
        else{c.box(x+4,5,7,12,rgb(92,90,83));c.rect(x+5,2,5,3,Muted);}
        if(removing){
            const int duration=c.a.potion_removed_discard?400:200,t=int(age)*4096/duration;
            const int back=int((int64_t(t)*t/4096)*(int64_t(11066)*t/4096-6970)/4096);
            const int scale=c.a.potion_removed_discard?4096:4096-back,w=mx(1,(width-3)*scale/4096),h=mx(1,18*scale/4096);
            c.picture(c.a.content.potion_image(c.a.potion_removed),x+width/2-w/2,11-h/2-(c.a.potion_removed_discard?back*20/4096:0),w,h);
        }
        c.hit(x,0,width,22,HitKind::PotionSlot,slot,potion!=Potion::Empty);
    }
    if(slots>3){c.text("›",171,5,10,Gold);c.hit(170,0,11,22,HitKind::PotionPage,1);}
    c.ui(U_TopFloor,183,3,13,16);b[0]=0;t.n=0;t.number(s.floor);c.text_fitted(b,198,5,17,8,Gold);
    if(s.practice)c.text_fitted("图鉴",217,5,31,10,Gold,8,true);
    else c.ui(U_TopMap,224,2,18,18);
    c.hit(216,0,33,22,HitKind::Command,(int)(s.practice?Key::Collection:Key::Map),!s.world.active||s.phase!=Phase::Map);
    c.ui(U_TopDeck,257,2,18,18);c.hit(249,0,34,22,HitKind::Command,(int)Key::Inspect);
    c.ui(U_TopSettings,291,2,18,18);c.hit(283,0,37,22,HitKind::Command,(int)Key::Back);
    if(!c.a.panel&&(combat_phase(s.phase)||s.phase==Phase::Shop||s.phase==Phase::Camp||s.phase==Phase::Reward||s.phase==Phase::WorldRelicChoice||s.phase==Phase::OriginalEventChoice))owned_relic_strip(c);

}

#include "rooms_ui.inc"
#include "status_ui.inc"
#include "world_intent_ui.inc"
#include "combat_ui.inc"
static void large_card(Canvas&c,Card card,int x,int y,bool selected,int fee=-2,int stars=-2,int instance=-1,int deck_index=-1,const EventCardFacts*facts=nullptr){
    if(selected){c.box(x-2,y-2,84,116,Gold);c.box(x-1,y-1,82,114,Gold);}
    draw_card_face(c,card,x,y,80,112,fee,stars,instance,deck_index,facts);
}
#include "world_reward_ui.inc"
#include "reward_presentation_ui.inc"
static void reward_screen(Canvas&c){
    if(c.a.game.world.active){native_reward_screen(c);return;}
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);c.actor(0,54,130);c.dim(0,22,320,218,155);
    if(a.grimoire_select&&s.necro.grimoire_pending){deck_select(c);return;}
    if(a.reward_cards){
        c.centered("选择一张牌",0,36,320,20,Gold);
        for(int i=0;i<3;i++){int x=24+i*96;large_card(c,{s.reward[i],0},x,77,a.cursor==i);c.hit(x-3,72,86,121,HitKind::Choice,i);}
        c.button("详情 I",14,209,91,false,25);c.hit(10,205,99,31,HitKind::Command,(int)Key::Detail,a.cursor<3);
        c.ui(U_RewardSkip,211,209,92,25);c.centered("跳过",211,211,92,12,a.cursor==3?Gold:White);c.hit(207,205,104,31,HitKind::Choice,3);
        return;
    }
    const bool grimoire=s.necro.grimoire_pending!=0;
    c.ui(U_RewardPanel,50,34,220,192);c.ui(U_RewardBanner,38,32,244,54);c.centered("战利品",60,47,200,20,Gold);
    for(int i=0;i<3;i++){
        int y=(grimoire?82:98)+i*(grimoire?30:33);bool claimed=s.reward_claimed&(1<<i);
        bool available=i!=2||s.reward_potion!=Potion::Empty;
        c.ui(U_RewardButton,70,y,180,29,(claimed||!available)?110:255);
        if(a.cursor==i&&!claimed&&available)c.box(68,y-1,184,31,Gold);
        if(i==0)c.ui(U_RewardGold,76,y+3,23,23);
        if(i==1)c.ui(U_RewardCard,76,y+3,23,23);
        if(i==2&&available)c.picture(a.content.potion_image(s.reward_potion),78,y+2,20,25);
        char text[96]={};TextBuffer t{text,sizeof text,0,&c.a.content};
        if(claimed)t.text("已领取");
        else if(i==0){t.number(s.last_gold);t.text(" 金币");}
        else if(i==1)t.text("选择一张牌");
        else if(available)t.text(potion_definition(s.reward_potion)->name);
        else t.text("没有药水掉落");
        c.text(text,107,y+6,12,claimed?Muted:White,137);
        const UiAction actions[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion};
        c.action(68,y-1,184,31,actions[i],!claimed&&available);
    }
    if(grimoire){
        c.ui(U_RewardButton,70,172,180,29);if(a.cursor==3)c.box(68,171,184,31,Gold);
        c.ui(U_NecroPowerStart+(int)NPower::ForbiddenGrimoire,76,175,23,23);
        char label[80]={};TextBuffer t{label,sizeof label,0,&c.a.content};t.text("魔典移牌 × ");t.number(s.necro.grimoire_pending);
        c.text(label,107,178,12,White,137);c.action(68,171,184,31,UiAction::RewardGrimoire);
    }
    native_proceed(c,213,211,99,25,"前进",true,a.cursor==(grimoire?4:3));c.action(209,208,107,30,UiAction::LeaveRewards);
}

#include "world_camp_ui.inc"
static void camp_screen(Canvas&c){
    if(c.a.game.world.active){native_camp_screen(c);return;}
    auto&a=c.a;c.picture(A_CampBackground,0,22,320,218);c.clip(character_camp_animation(a.game.character),188,61,a.animation*33,false);
    if(a.game.character==Character::Necrobinder)c.clip(AnimOstyCamp,247,100,a.animation*33,false);
    c.picture(a.content.ui_image(U_CampFlames)+(int)((a.animation*33/125)%160),130,76,46,58);
    c.picture(A_CampFire,128,111,55,34,(unsigned)(225+(a.animation%16<8?a.animation%8:15-a.animation%16)*4));
    c.centered("休息处",0,43,320,20,Gold);
    const int xs[2]={54,189};
    for(int i=0;i<2;i++){
        bool enabled=!i||has_upgrade(a.game);int x=xs[i];
        if(a.cursor==i)c.ui(U_RestOutline,x-4,137,88,62);
        c.ui(i?U_RestSmith:U_RestHeal,x,140,80,57,enabled?255:100);
        c.centered(i?"锻造":"休息",x-4,193,88,16,a.cursor==i?Gold:White);
        c.hit(x-9,136,98,80,HitKind::Choice,i,enabled);
    }
    c.dim(0,219,320,21,200);char info[128]={};TextBuffer t{info,sizeof info,0,&c.a.content};
    if(a.cursor==0){t.text("回复 ");t.number(rest_heal_amount(a.game));t.text((a.game.relics&RegalPillow)?" 点生命（包含皇家枕头）":" 点生命（最大生命的 30%）");}
    else t.text("升级牌组中的一张牌。");
    c.centered(info,0,221,320,10,White);
}

#include "potion_ui.inc"
static void panels(Canvas&c){
    auto&a=c.a;auto&s=a.game;if(a.panel==4){potion_popup_screen(c);return;}a.hit_count=0;if(a.panel==3){map_screen(c,true);c.close_button();return;}
    c.rect(4,24,312,212,Panel);c.box(4,24,312,212,Gold);
    if(a.panel==1){
        const char*names[]={"牌组","抽牌堆","弃牌堆","消耗堆","能力"};
        for(int tab=0;tab<5;tab++){int x=10+tab*54;
            c.text(names[tab],x+3,28,10,tab==a.pile_tab?Gold:Muted,50);
            if(tab==a.pile_tab)c.rect(x,45,48,1,Gold);c.hit(x,25,52,23,HitKind::PileTab,tab);}
        int ids[MaxCards],n=0;
        if(a.pile_tab==0){n=s.deck_count;for(int i=0;i<n;i++)ids[i]=i;}
        else{const Pile&p=pile(s,a.pile_tab==1?Zone::Draw:a.pile_tab==2?Zone::Discard:a.pile_tab==3?Zone::Exhaust:Zone::Power);n=p.count;for(int i=0;i<n;i++)ids[i]=p.ids[i];
            // Inspecting the draw pile must never disclose its actual shuffle order.
            if(a.pile_tab==1)for(int i=1;i<n;i++){int id=ids[i],j=i;while(j&&s.cards[ids[j-1]].kind>s.cards[id].kind){ids[j]=ids[j-1];--j;}ids[j]=id;}}
        a.inspect_cursor=mn(a.inspect_cursor,mx(0,n-1));a.scroll=mn((a.scroll/10)*10,mx(0,((n-1)/10)*10));
        int portraits[10];unsigned portrait_count=0;
        for(int i=a.scroll;i<mn(n,a.scroll+10);++i){
            Card card=a.pile_tab==0?s.deck[ids[i]]:s.cards[ids[i]];
            portraits[portrait_count++]=a.card_face_image(card,a.pile_tab==0?-1:ids[i],a.pile_tab==0?ids[i]:-1);
        }
        a.content.prepare_images(portraits,portrait_count);
        for(int i=a.scroll;i<mn(n,a.scroll+10);i++){
            Card card=a.pile_tab==0?s.deck[ids[i]]:s.cards[ids[i]];
            int x=24+(i-a.scroll)%5*54,y=52+(i-a.scroll)/5*74;
            bool hovered=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+50&&a.pointer.y>=y-2&&a.pointer.y<y+68;
            small_card(c,card,x,y,hovered||(!a.pointer.visible&&i==a.inspect_cursor),true,0,a.pile_tab==0?cost(card):cost(s,ids[i]),card_star_cost(s,card,a.pile_tab==0?-1:ids[i]),a.pile_tab==0?-1:ids[i],a.pile_tab==0?ids[i]:-1);
            c.hit(x-2,y-2,51,70,a.pile_tab==0?HitKind::DeckCard:HitKind::InstanceCard,ids[i]);
        }
        if(!n)c.centered("空",0,105,320,12,Muted);
        c.text("‹",14,201,16,Gold);c.hit(7,198,37,27,HitKind::Scroll,-2,a.scroll>0);
        c.text("›",296,201,16,Gold);c.hit(279,198,34,27,HitKind::Scroll,2,a.scroll+10<n);
        char page[64]={};TextBuffer b{page,sizeof page,0,&c.a.content};b.number(n);b.text(" 张  ·  ");b.number(a.scroll/10+1);b.text(" / ");b.number(mx(1,(n+9)/10));
        c.centered(page,45,202,230,12,Gold);
        c.centered(a.pile_tab==1?"无序查看 · 点击卡牌看详情":"点击卡牌看详情 · ←→ 切换牌堆",0,221,320,10,Muted);
    }else if(a.panel==2){
        c.centered("本局记录（最近 24 条）",5,27,310,12,Gold);a.scroll=mn(a.scroll,mx(0,s.log_count-10));int first=mx(0,s.log_count-10-a.scroll);
        for(int i=first;i<mn(s.log_count,first+10);i++){char b[160]={};log_line(a,s.logs[i],b,sizeof b);c.text(b,13,49+(i-first)*16,10,White,295);}c.text("↑↓ 滚动 · Esc 返回",15,216,10,Muted);
    }else if(a.panel==5){
        owned_relic_panel(c);

    }
    c.close_button();
    if(a.panel==2){c.text("↑",299,53,12,Gold);c.hit(294,50,23,24,HitKind::Scroll,-1);c.text("↓",299,157,12,Gold);c.hit(294,153,23,24,HitKind::Scroll,1);}
}
int App::detail_energy()const{
    if(detail_card<0)return -2;
    Card card=card_from_token(detail_card);
    const EventCardFacts*facts=detail_has_facts?&detail_facts:event_card_facts(game,card,-1,detail_deck_index);
    int energy=detail_instance>=0?card_preview_cost(game,card,detail_instance):detail_cost_override==-2?card_offer_cost(card,facts):detail_cost_override;
    if(detail_instance>=0&&choice_free_energy(game)&&cost(card)>=0){
        for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==detail_instance){energy=card_free_preview_cost(game,card,detail_instance);break;}
    }
    return energy;
}
int App::detail_stars()const{
    if(detail_card<0)return 0;
    Card card=card_from_token(detail_card);int stars=card_star_cost(game,card,library?-1:detail_instance);
    if(!library&&stars>=0&&choice_free_energy(game)){
        for(unsigned i=0;i<game.expansion.choices.count;i++){
            int token=game.expansion.choices.ids[i];
            if((detail_instance>=0&&token==detail_instance)||
               (detail_instance<0&&game.expansion.choice==XChoice::Offer&&token>=MaxCards&&token<MaxCards+3&&game.expansion.offers[token-MaxCards].kind==card.kind))return 0;
        }
    }
    return stars;
}
static void detail_screen(Canvas&c){
    auto&a=c.a;Card card=card_from_token(a.detail_card);
    a.hit_count=0;c.dim(0,0,320,240,170);c.rect(7,22,306,211,Panel);c.box(7,22,306,211,Gold);
    char title[128]={};TextBuffer name{title,sizeof title,0,&a.content};name.text(a.content.card_string(card.kind));if(card.upgraded)name.text("+");
    c.text_fitted(title,18,28,270,16,card.upgraded?Green:Gold,10,true);
    int type=(int)a.card_face_type(card,a.detail_instance,a.detail_deck_index,a.detail_has_facts?&a.detail_facts:nullptr);
    char description[2048]={};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,a.detail_instance,a.detail_deck_index);
    draw_card_face(c,card,22,62,86,121,a.detail_energy(),a.detail_stars(),a.detail_instance,a.detail_deck_index,a.detail_has_facts?&a.detail_facts:nullptr,description);
    if(!a.library&&combat_phase(a.game.phase)&&a.detail_instance>=0&&a.detail_instance<a.game.card_count&&
       a.game.cards[a.detail_instance].kind==card.kind&&a.game.silent.inky[a.detail_instance])c.ui(U_Inky,56,177,18,18);
    int lines=c.paragraph(description,123,66,177,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-8));
    c.paragraph(description,123,66,177,8,a.detail_scroll);
    c.text(card_type_label((CardType)type,&a.content),124,48,10,Muted);
    const bool lab=a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
    const bool upgrade_preview=card.upgraded||card_upgradeable(card);
    if(lab){
        c.button("返回",11,205,67,false,25);c.hit(9,202,72,31,HitKind::DetailClose);
        c.button(card.upgraded?"基础":upgrade_preview?"升级":"不可升级",87,205,67,false,25);c.action(84,202,74,31,UiAction::ToggleCardUpgrade,upgrade_preview);
        const bool room=a.game.card_count<MaxCards;
        c.button(room?(a.game.hand.count<MaxHand?"加入手牌":"加入弃牌堆"):"卡牌已满",168,205,135,room,25);
        c.action(163,202,143,31,UiAction::LabCard,room);
    }else{
        c.button("返回",21,204,98,false,25);c.hit(20,202,101,28,HitKind::DetailClose);
        c.button(card.upgraded?"查看基础":upgrade_preview?"预览升级":"不可升级",166,204,132,upgrade_preview,25);c.action(163,202,139,28,UiAction::ToggleCardUpgrade,upgrade_preview);
    }
    c.close_button(290,26);c.hit(288,24,25,24,HitKind::DetailClose);
    if(lines>8&&!a.keyword_hover.details){c.hit(285,55,25,25,HitKind::Scroll,-1);c.hit(285,174,25,25,HitKind::Scroll,1);}
    if(!a.keyword_hover.details){c.centered("V 词条",19,190,92,10,Gold);c.hit(18,187,94,15,HitKind::Command,int(Key::Statuses));}
}
#include "reference_map_ui.inc"
#include "ui_frontend.inc"
#include "card_choice_ui.inc"
#include "world_choice_ui.inc"
#include "world_game_over_ui.inc"
#include "deck_confirm_ui.inc"
#include "keyword_tooltip_ui.inc"
static void pointer_overlay(Canvas&c){
    auto&a=c.a;
    // The two authored black transition layers composite before the host
    // pointer. Per-row coverage preserves the original moving gradient.
    if(a.map_travel.pending||a.map_travel.fading_in){
        for(int y=0;y<ScreenH;++y){const unsigned alpha=a.map_transition_alpha(y);
            if(alpha==255)c.rect(0,y,ScreenW,1,0);
            else if(alpha)for(int x=0;x<ScreenW;++x)c.paint(y*c.pitch+x,0,alpha);
        }
        if(a.map_transition_blocks_pointer())a.hit_count=0;
    }
    auto&p=a.pointer;if(!p.visible)return;
    if(a.panel==4&&a.potion_targeting){const int x=128+(a.scroll%3)*(potion_slot_count(a.game)>3?14:17)+7;c.line(x,21,p.x,p.y,Blue,true);c.box(p.x-5,p.y-5,11,11,Gold);}
    if(p.armed_card>=0&&p.armed_card<a.game.card_count){
        if(a.game.zones[p.armed_card]!=Zone::Hand||a.game.phase!=Phase::Combat)p.armed_card=-1;
        else{c.line(160,180,p.x,p.y,Blue,true);c.box(p.x-5,p.y-5,11,11,Gold);}
    }
    // Small high-contrast cursor, clipped by the same raster bounds as the UI.
    for(int y=0;y<12;y++)for(int x=0;x<=y/2;x++)c.rect(p.x+x,p.y+y,1,1,x==0||x==y/2||y==11?Ink:White);
    c.rect(p.x+4,p.y+10,2,5,Ink);c.rect(p.x+4,p.y+10,1,4,White);
}
void App::render(){
    begin_card_face_frame();
    sync_reward_presentation();
    sync_map_window();
    if(!character_select)character_select_presentation.observe(false,0,-1,map_clock_ms);
    sync_combat_preparation();
    if(combat_preparation.active){
        hit_count=0;loading_screen(pixels,ResourceStage::Images,combat_preparation.cursor,combat_preparation.count,map_clock_ms/33,boot_text(BootText::PreparingBattle,boot_language(content.english())),true,boot_language(content.english()));
        combat_preparation.shown=true;Canvas c{*this};c.rect(35,146,250,15,0x10c3);c.centered("准备战斗",35,147,250,10,White);
        if(paused)native_pause_screen(c);if(frontend_confirm)frontend_confirm_screen(c);pointer_overlay(c);return;
    }
    map_visuals.update(game,map_clock_ms);
    // A capstone overlay hides the map without closing it. Its marker
    // lifecycle must not replay when the deck or compendium is dismissed.
    map_visuals.set_open(game,game.world.active&&!title&&!character_select&&!map_preview.open&&
        (game.phase==Phase::Map||map_window.is_open||panel==3),map_clock_ms);
    Canvas c{*this};sync_combat_feedback();sync_relic_feedback();sync_character_feedback();sync_enemy_feedback();sync_turn_banner();sync_health_bars();sync_map_reticles();hit_count=0;c.rect(0,0,320,240,Ink);
    // Opaque screens do not redraw the hidden title/combat scene or its clips.
    if(map_preview.open){reference_map_screen(c);pointer_overlay(c);return;}
    if(library){library_screen(c);if(detail_card>=0)detail_screen(c);if(detail_item>=0)item_detail_screen(c);keyword_tooltips(c);pointer_overlay(c);return;}
    if(character_select){character_screen(c);pointer_overlay(c);return;}
    if(title){title_screen(c);
    }else{
        // The room beneath a modal can clamp its own text scroll. It must not
        // replace the active potion/deck/log/relic panel's independent cursor.
        const int panel_scroll=scroll;
        const int map_cursor=cursor;
        if(map_window.is_open){cursor=map_room_cursor;scroll=map_room_scroll;}
        const bool combat_linger=int32_t(combat_linger_until-animation)>0;
        if(combat_linger||(combat_feedback.active&&choice_phase(game.phase)))combat_screen(c);else switch(game.phase){
        case Phase::Map:if(!game.world.active||!map_window.visible)map_screen(c);break;
        case Phase::Shop:shop_screen(c);break;
        case Phase::Event:event_screen(c);break;
        case Phase::Camp:camp_screen(c);break;
        case Phase::Upgrade:case Phase::Remove:case Phase::Transform:c.picture(A_Title,0,0);deck_select(c);break;
        case Phase::Reward:reward_screen(c);break;
        case Phase::Combat:case Phase::ExhaustChoice:case Phase::DiscardChoice:case Phase::ExhaustManyChoice:case Phase::ReturnChoice:case Phase::ExtendedChoice:combat_screen(c);break;
        case Phase::Treasure:treasure_screen(c);break;
        case Phase::WorldRelicChoice:case Phase::OriginalEventChoice:c.picture(A_Title,0,0);break;
        default:if(game.world.active&&(game.phase==Phase::Complete||game.phase==Phase::Dead)){native_game_over_screen(c);break;}c.picture(A_Title,0,0);c.dim(0,20,320,220,120);c.centered(game.phase==Phase::Complete?"远征完成":game.phase==Phase::Dead?"倒在了高塔下":"达到安全容量上限",0,78,320,20,Gold);c.centered("Enter 返回标题",0,150,320,12);c.hit(0,130,320,65,HitKind::Command,(int)Key::Confirm);if(!game.world.active)c.centered("原型路线结束，并非完整原版通关。",0,210,320,10,Muted);break;
        }
        // The underlying live room may clamp its own navigation while it is
        // hidden. Never let that overwrite the selected map column or a
        // covering capstone's scroll position.
        if(map_window.is_open){map_room_cursor=cursor;map_room_scroll=scroll;cursor=map_cursor;scroll=panel_scroll;}
        if(map_window.visible)world_map_window_screen(c);
        if(!combat_linger&&(!panel||panel==4)&&!library&&combat_choice_ready()&&(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice))world_choice_screen(c);
        else if(!combat_linger&&!panel&&!library&&combat_choice_ready()&&choice_phase(game.phase))card_choice_screen(c);
        if(panel&&!(panel==3&&map_window.visible)){scroll=panel_scroll;panels(c);}hud(c);
        if(!combat_linger||combat_feedback.active)turn_banner_screen(c);
        reward_pickup_screen(c);
        if(paused)native_pause_screen(c);

    }
    if(settings_open)frontend_settings_screen(c);
    if(frontend_confirm)frontend_confirm_screen(c);
    if(help){
        hit_count=0;c.hit(6,7,308,226,HitKind::Close);c.rect(6,7,308,226,Panel);c.box(6,7,308,226,Gold);c.centered("操作说明",6,13,308,16,Gold);
        const char*lines[]={"方向键 / 4、6、8、2：导航","Enter / 5：出牌、选路或确认","空格：结束回合","Tab：牌组与牌堆    L：记录","M：地图  P：药水  R：遗物  I：详情","S：保存  O：载入  C：图鉴  V：状态","Esc：返回或打开菜单","事件中：↑↓ 阅读，←→ 选项","三幕路线，含首领、古代之民与事件。","触摸板：轻触/按下点击，按住可拖牌。"};
        for(int i=0;i<10;i++)c.text(lines[i],16,41+i*17,12,i<8?White:Muted,291);c.centered("Enter / Esc 返回",0,216,320,10,Gold);
    }
    if(character_select)character_screen(c);
    if(library)library_screen(c);
    if(deck_candidate>=0&&!paused&&!help&&!panel)deck_confirm_screen(c);
    if(detail_card>=0)detail_screen(c);
    if(detail_item>=0)item_detail_screen(c);
    if(status_actor>=0)status_screen(c);
    if(event_reader)event_reader_screen(c);
    if(map_preview.open)reference_map_screen(c);
    if(notice_ticks&&detail_item<0&&detail_card<0&&!panel&&game.phase!=Phase::Treasure&&game.phase!=Phase::Event&&game.phase!=Phase::Shop&&status_actor<0&&!map_preview.open&&!library&&!character_select&&deck_candidate<0&&!choice_phase(game.phase)){c.rect(2,219,316,20,Panel);c.box(2,219,316,20,Gold);c.centered(notice,3,221,314,10,Gold);}
    potion_hover_tip(c);
    owned_relic_tooltip(c);
    enemy_intent_tooltip(c);
    battle_status_tooltip(c);
    keyword_tooltips(c);
    pointer_overlay(c);
}
} // namespace spire
