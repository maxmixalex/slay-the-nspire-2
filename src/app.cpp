#include "app.hpp"
#include "rich_text.hpp"
#include "status.hpp"
#include "combat_layout.hpp"
#include "event_assets.hpp"
namespace spire {
#include "canvas.inc"
#include "app_input.inc"
static const char* card_type_label(CardType type){
    switch(type){case CardType::Attack:return "攻击";case CardType::Skill:return "技能";
    case CardType::Power:return "能力";case CardType::Status:return "状态";case CardType::Curse:return "诅咒";}
    return "";
}
static void small_card(Canvas&c,Card card,int x,int y,bool selected,bool playable,int number,int fee=-2){
    int frame=c.a.content.frame_image(card);
    if(selected){c.rect(x-2,y-2,51,70,rgb(91,74,47));c.box(x-2,y-2,51,70,Gold);}
    c.picture(frame,x,y,47,66);c.picture(c.a.content.card_image(card.kind),x+3,y+15,41,30);c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y+1,47,11);
    // Card art and text are independent; the description remains legible below the stage.
    c.centered(c.a.content.card_string(card.kind),x+3,y+48,41,10,card.upgraded?Green:White);
    c.picture(c.a.content.energy_image(card),x-1,y-1,15,15);int energy=fee==-2?cost(card):fee;c.energy_label(energy,x+4,y-2,12);
    if(card.upgraded)c.text("+",x+35,y+1,10,Green);if(number)c.num(number,x+32,y+33,10,Muted);
    if(!playable)c.dim(x,y,47,66,125);
}
static void log_line(App&a,Log l,char*b,size_t n){
    b[0]=0;TextBuffer t{b,n,0};switch(l.kind){
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
    case LogKind::Relic:t.text("获得遗物：");if(auto*d=relic_definition((unsigned)l.a))t.text(d->name);break;
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
    c.picture(A_Heart,5,4,13,13);char b[32]={};TextBuffer t{b,sizeof b,0};t.number(s.hp);t.ch('/');t.number(s.max_hp);c.text(b,20,3,10,White);
    c.picture(A_Coin,79,4,12,13);c.num(s.gold,94,3,10,Gold);
    c.ui(U_TopFloor,130,4,11,13);c.num(s.floor,144,3,10,White);
    for(int i=0;i<2;i++){
        int x=163+i*23;
        if(s.potions[i]!=Potion::Empty)c.picture(c.a.content.potion_image(s.potions[i]),x,1,18,20);
        else{c.box(x+4,5,10,12,rgb(92,90,83));c.rect(x+7,2,4,3,Muted);}
        c.hit(x,0,22,22,HitKind::Command,(int)Key::Potions);
    }
    if(s.practice)c.text("图鉴",219,3,10,Gold);
    else c.ui(U_TopMap,224,2,18,18);
    c.hit(216,0,33,22,HitKind::Command,(int)(s.practice?Key::Collection:Key::Map));
    c.ui(U_TopDeck,257,2,18,18);c.hit(249,0,34,22,HitKind::Command,(int)Key::Inspect);
    c.ui(U_TopSettings,291,2,18,18);c.hit(283,0,37,22,HitKind::Command,(int)Key::Back);
    if(!c.a.panel&&(combat_phase(s.phase)||s.phase==Phase::Shop||s.phase==Phase::Camp))owned_relic_strip(c);

}

#include "rooms_ui.inc"
#include "status_ui.inc"
#include "combat_ui.inc"
static void large_card(Canvas&c,Card card,int x,int y,bool selected,int fee=-2){
    int type=(int)definitions[card.kind].type;
    if(selected){c.box(x-2,y-2,84,116,Gold);c.box(x-1,y-1,82,114,Gold);}
    c.picture(c.a.content.frame_image(card),x,y,80,112);
    c.picture(c.a.content.card_image(card.kind),x+5,y+20,70,51);
    c.ui(card_rarity(card.kind)==4?U_RareBanner:card_rarity(card.kind)==3?U_BigBanner:U_CommonBanner,x,y+1,80,17);
    const char*name=c.a.content.card_string(card.kind);
    for(int dx=-1;dx<=1;dx++)for(int dy=-1;dy<=1;dy++)if(dx||dy)c.centered(name,x+5+dx,y+1+dy,70,10,Ink);
    c.centered(name,x+5,y+1,70,10,card.upgraded?Green:White);
    c.picture(c.a.content.energy_image(card),x-3,y-5,19,19);
    int energy=fee==-2?cost(card):fee;c.energy_label(energy,x+3,y-4,12);
    char text[320]={};c.a.card_description(card,-1,text,sizeof text);c.dim(x+5,y+73,70,29,135);c.paragraph(text,x+7,y+74,66,2,0,10,White);
    c.centered(card_type_label((CardType)type),x+4,y+99,72,10,Muted);
}
static void reward_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;c.ui(U_BattleBackground,0,22);c.actor(0,54,130);c.dim(0,22,320,218,155);
    if(a.reward_cards){
        c.centered("选择一张牌",0,36,320,20,Gold);
        for(int i=0;i<3;i++){int x=24+i*96;large_card(c,{s.reward[i],0},x,77,a.cursor==i);c.hit(x-3,72,86,121,HitKind::Choice,i);}
        c.button("详情 I",14,209,91,false,25);c.hit(10,205,99,31,HitKind::Command,(int)Key::Detail,a.cursor<3);
        c.ui(U_RewardSkip,211,209,92,25);c.centered("跳过",211,211,92,12,a.cursor==3?Gold:White);c.hit(207,205,104,31,HitKind::Choice,3);
        return;
    }
    c.ui(U_RewardPanel,50,34,220,192);c.ui(U_RewardBanner,38,32,244,54);c.centered("战利品",60,47,200,20,Gold);
    for(int i=0;i<3;i++){
        int y=98+i*33;bool claimed=s.reward_claimed&(1<<i);
        bool available=i!=2||s.reward_potion!=Potion::Empty;
        c.ui(U_RewardButton,70,y,180,29,(claimed||!available)?110:255);
        if(a.cursor==i&&!claimed&&available)c.box(68,y-1,184,31,Gold);
        if(i==0)c.ui(U_RewardGold,76,y+3,23,23);
        if(i==1)c.ui(U_RewardCard,76,y+3,23,23);
        if(i==2&&available)c.picture(a.content.potion_image(s.reward_potion),78,y+2,20,25);
        char text[96]={};TextBuffer t{text,sizeof text,0};
        if(claimed)t.text("已领取");
        else if(i==0){t.number(s.last_gold);t.text(" 金币");}
        else if(i==1)t.text("选择一张牌");
        else if(available)t.text(potion_definition(s.reward_potion)->name);
        else t.text("没有药水掉落");
        c.text(text,107,y+6,12,claimed?Muted:White,137);
        const UiAction actions[]={UiAction::RewardGold,UiAction::RewardCards,UiAction::RewardPotion};
        c.action(68,y-1,184,31,actions[i],!claimed&&available);
    }
    c.ui(U_RewardSkip,213,211,99,25);c.centered("前进 ›",213,214,99,12,a.cursor==3?Gold:White);c.action(209,208,107,30,UiAction::LeaveRewards);
}

static void camp_screen(Canvas&c){
    auto&a=c.a;c.picture(A_CampBackground,0,22,320,218);c.clip(a.game.character==Character::Silent?AnimSilentCamp:AnimCampHero,188,61,a.animation*33,false);
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
    c.dim(0,219,320,21,200);char info[128]={};TextBuffer t{info,sizeof info,0};
    if(a.cursor==0){t.text("回复 ");t.number(rest_heal_amount(a.game));t.text((a.game.relics&RegalPillow)?" 点生命（包含皇家枕头）":" 点生命（最大生命的 30%）");}
    else t.text("升级牌组中的一张牌。");
    c.centered(info,0,221,320,10,White);
}

static void panels(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.hit_count=0;if(a.panel==3){map_screen(c,true);c.close_button();return;}
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
        for(int i=a.scroll;i<mn(n,a.scroll+10);i++){
            Card card=a.pile_tab==0?s.deck[ids[i]]:s.cards[ids[i]];
            int x=24+(i-a.scroll)%5*54,y=52+(i-a.scroll)/5*74;
            bool hovered=a.pointer.visible&&a.pointer.x>=x-2&&a.pointer.x<x+50&&a.pointer.y>=y-2&&a.pointer.y<y+68;
            small_card(c,card,x,y,hovered||(!a.pointer.visible&&i==a.inspect_cursor),true,0,a.pile_tab==0?cost(card):cost(s,ids[i]));
            c.hit(x-2,y-2,51,70,a.pile_tab==0?HitKind::PileCard:HitKind::InstanceCard,a.pile_tab==0?card.kind+(card.upgraded<<8):ids[i]);
        }
        if(!n)c.centered("空",0,105,320,12,Muted);
        c.text("‹",14,201,16,Gold);c.hit(7,198,37,27,HitKind::Scroll,-2,a.scroll>0);
        c.text("›",296,201,16,Gold);c.hit(279,198,34,27,HitKind::Scroll,2,a.scroll+10<n);
        char page[64]={};TextBuffer b{page,sizeof page,0};b.number(n);b.text(" 张  ·  ");b.number(a.scroll/10+1);b.text(" / ");b.number(mx(1,(n+9)/10));
        c.centered(page,45,202,230,12,Gold);
        c.centered(a.pile_tab==1?"无序查看 · 点击卡牌看详情":"点击卡牌看详情 · ←→ 切换牌堆",0,221,320,10,Muted);
    }else if(a.panel==2){
        c.centered("本局记录（最近 24 条）",5,27,310,12,Gold);a.scroll=mn(a.scroll,mx(0,s.log_count-10));int first=mx(0,s.log_count-10-a.scroll);
        for(int i=first;i<mn(s.log_count,first+10);i++){char b[160]={};log_line(a,s.logs[i],b,sizeof b);c.text(b,13,49+(i-first)*16,10,White,295);}c.text("↑↓ 滚动 · Esc 返回",15,216,10,Muted);
    }else if(a.panel==5){
        owned_relic_panel(c);
    }else if(a.panel==4){
        c.centered("药水",0,30,320,16,Gold);
        for(int i=0;i<2;i++){
            int x=43+i*133;c.hit(x,59,103,57,HitKind::Potion,i);if(a.scroll==i)c.box(x,59,103,57,Gold);
            int p=(int)s.potions[i];if(p)c.picture(a.content.potion_image((Potion)p),x+38,63,25,29);
            c.centered(potion_name(p),x+2,96,99,10);
        }
        c.paragraph(potion_desc((int)s.potions[a.scroll]),19,126,282,3,0,12);
        const auto*d=potion_definition(s.potions[a.scroll]);
        if(d&&d->targeted&&s.phase==Phase::Combat){
            for(int i=0;i<s.enemy_count;i++){int x=19+i*147;char label[24]={};TextBuffer t{label,sizeof label,0};t.text("敌人 ");t.number(i+1);
                c.button(label,x,176,133,a.target==i,23);c.action(x,174,133,26,i?UiAction::TargetSecond:UiAction::TargetFirst,s.enemies[i].hp>0);}
        }
        c.button("丢弃",28,205,113,false,25);c.action(25,204,119,28,UiAction::DiscardPotion,d!=nullptr);
        bool usable=can_use_potion(s,a.scroll,a.target);c.button(d&&d->automatic?"自动触发":usable?"使用":"无法使用",176,205,113,usable,25);
        c.hit(174,204,117,28,HitKind::Command,(int)Key::Confirm,usable);
    }
    c.close_button();
    if(a.panel==2){c.text("↑",299,53,12,Gold);c.hit(294,50,23,24,HitKind::Scroll,-1);c.text("↓",299,157,12,Gold);c.hit(294,153,23,24,HitKind::Scroll,1);}
}
int App::detail_energy()const{
    if(detail_card<0)return -2;
    Card card{(uint8_t)(detail_card&255),(uint8_t)(detail_card>>8)};
    int energy=detail_instance>=0?card_preview_cost(game,card,detail_instance):detail_cost_override==-2?cost(card):detail_cost_override;
    if(detail_instance>=0&&game.phase==Phase::ExtendedChoice&&game.expansion.choice_free&&cost(card)>=0){
        for(unsigned i=0;i<game.expansion.choices.count;i++)if(game.expansion.choices.ids[i]==detail_instance){energy=0;break;}
    }
    return energy;
}
static void detail_screen(Canvas&c){
    auto&a=c.a;Card card{(uint8_t)(a.detail_card&255),(uint8_t)(a.detail_card>>8)};
    a.hit_count=0;c.dim(0,0,320,240,170);c.rect(7,22,306,211,Panel);c.box(7,22,306,211,Gold);
    char title[128]={};TextBuffer name{title,sizeof title,0};name.text(a.content.card_string(card.kind));if(card.upgraded)name.text("+");
    c.centered(title,18,28,270,16,card.upgraded?Green:Gold);
    int type=(int)definitions[card.kind].type;
    c.picture(a.content.frame_image(card),18,61,94,132);
    c.picture(c.a.content.card_image(card.kind),24,90,82,60);c.picture(a.content.energy_image(card),18,60,26,26);
    c.energy_label(a.detail_energy(),27,64,12);
    c.centered(title,21,161,87,12,card.upgraded?Green:White);
    if(!a.library&&combat_phase(a.game.phase)&&a.detail_instance>=0&&a.detail_instance<a.game.card_count&&
       a.game.cards[a.detail_instance].kind==card.kind&&a.game.silent.inky[a.detail_instance])c.ui(U_Inky,56,177,18,18);
    char description[512]={};a.card_description(card,card_targets_enemy(a.game,card)?a.target:-1,description,sizeof description,a.detail_instance);
    int lines=c.paragraph(description,123,66,177,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-8));
    c.paragraph(description,123,66,177,8,a.detail_scroll);
    c.text(card_type_label((CardType)type),124,48,10,Muted);
    const bool lab=a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
    if(lab){
        c.button("返回",11,205,67,false,25);c.hit(9,202,72,31,HitKind::DetailClose);
        c.button(card.upgraded?"基础":"升级",87,205,67,false,25);c.action(84,202,74,31,UiAction::ToggleCardUpgrade);
        const bool room=a.game.card_count<MaxCards;
        c.button(room?(a.game.hand.count<MaxHand?"加入手牌":"加入弃牌堆"):"卡牌已满",168,205,135,room,25);
        c.action(163,202,143,31,UiAction::LabCard,room);
    }else{
        c.button("返回",21,204,98,false,25);c.hit(20,202,101,28,HitKind::DetailClose);
        c.button(card.upgraded?"查看基础":"预览升级",166,204,132,true,25);c.action(163,202,139,28,UiAction::ToggleCardUpgrade);
    }
    c.close_button(290,26);c.hit(288,24,25,24,HitKind::DetailClose);
    if(lines>8){c.hit(285,55,25,25,HitKind::Scroll,-1);c.hit(285,174,25,25,HitKind::Scroll,1);}
}
#include "reference_map_ui.inc"
#include "ui_frontend.inc"
#include "card_choice_ui.inc"
#include "deck_confirm_ui.inc"
static void pointer_overlay(Canvas&c){
    auto&a=c.a;auto&p=a.pointer;if(!p.visible)return;
    if(p.armed_card>=0&&p.armed_card<a.game.card_count){
        if(a.game.zones[p.armed_card]!=Zone::Hand||a.game.phase!=Phase::Combat)p.armed_card=-1;
        else{c.line(160,180,p.x,p.y,Blue,true);c.box(p.x-5,p.y-5,11,11,Gold);}
    }
    // Small high-contrast cursor, clipped by the same raster bounds as the UI.
    for(int y=0;y<12;y++)for(int x=0;x<=y/2;x++)c.rect(p.x+x,p.y+y,1,1,x==0||x==y/2||y==11?Ink:White);
    c.rect(p.x+4,p.y+10,2,5,Ink);c.rect(p.x+4,p.y+10,1,4,White);
}
void App::render(){
    Canvas c{*this};sync_relic_feedback();hit_count=0;c.rect(0,0,320,240,Ink);
    // Opaque screens do not redraw the hidden title/combat scene or its clips.
    if(map_preview.open){reference_map_screen(c);pointer_overlay(c);return;}
    if(library){library_screen(c);if(detail_card>=0)detail_screen(c);if(detail_item>=0)item_detail_screen(c);pointer_overlay(c);return;}
    if(character_select){character_screen(c);pointer_overlay(c);return;}
    if(title){title_screen(c);
    }else{
        switch(game.phase){
        case Phase::Map:map_screen(c);break;
        case Phase::Shop:shop_screen(c);break;
        case Phase::Event:event_screen(c);break;
        case Phase::Camp:camp_screen(c);break;
        case Phase::Upgrade:case Phase::Remove:case Phase::Transform:c.picture(A_Title,0,0);deck_select(c);break;
        case Phase::Reward:reward_screen(c);break;
        case Phase::Combat:case Phase::ExhaustChoice:case Phase::DiscardChoice:case Phase::ExhaustManyChoice:case Phase::ReturnChoice:case Phase::ExtendedChoice:combat_screen(c);break;
        case Phase::Treasure:treasure_screen(c);break;
        default:c.picture(A_Title,0,0);c.dim(0,20,320,220,120);c.centered(game.phase==Phase::Complete?"远征完成":game.phase==Phase::Dead?"倒在了高塔下":"达到安全容量上限",0,78,320,20,Gold);c.centered("Enter 返回标题",0,150,320,12);c.hit(0,130,320,65,HitKind::Command,(int)Key::Confirm);c.centered("原型路线结束，并非完整原版通关。",0,210,320,10,Muted);break;
        }
        if(panel)panels(c);hud(c);
        if(!panel&&!library&&choice_phase(game.phase))card_choice_screen(c);
        if(paused){hit_count=0;c.dim(0,0,320,240,185);c.centered("暂停",0,22,320,20,Gold);
            const char*items[]={"继续游戏","保存游戏","读取存档","操作说明","返回主菜单","退出游戏"};
            for(int i=0;i<6;i++){int y=60+i*28;c.ui(U_RewardButton,63,y,194,25);if(menu==i)c.box(62,y-1,196,27,Gold);
                c.centered(items[i],65,y+3,190,12,menu==i?Gold:White);c.hit(60,y-1,200,27,HitKind::Menu,i);}}

    }
    if(help){
        hit_count=0;c.hit(6,7,308,226,HitKind::Close);c.rect(6,7,308,226,Panel);c.box(6,7,308,226,Gold);c.centered("操作说明",6,13,308,16,Gold);
        const char*lines[]={"方向键 / 4、6、8、2：导航","Enter / 5：出牌、选路或确认","空格：结束回合","Tab：牌组与牌堆    L：记录","M：地图  P：药水  R：遗物  I：详情","S：保存  O：载入  C：图鉴  V：状态","Esc：返回或打开菜单","事件中：↑↓ 阅读，←→ 选项","12 层分岔路线，含商店、事件和营地。","触摸板：轻触/按下点击，按住可拖牌。"};
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
    owned_relic_tooltip(c);
    pointer_overlay(c);
}
} // namespace spire
