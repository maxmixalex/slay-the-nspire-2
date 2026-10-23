// Original power icons and textured panels, reflowed for a 320x240 display.
#include "world_status_ui.inc"
static void status_strip(Canvas&c,int actor,int x,int y,int width){
    int ids[StatusKinds],count=c.a.combat_statuses(actor,ids);
    if(!count)return;
    // NPowerContainer keeps every icon visible. Wrap at the creature's
    // display width; the amount overlaps the icon's lower-right corner.
    // Measure actual amounts, including negative and three-digit stacks.
    int widths[StatusKinds],rows=1,used=0;
    for(int i=0;i<count;i++){
        char amount[16]{};TextBuffer number{amount,sizeof amount,0,&c.a.content};number.number(c.a.combat_status_amount(actor,ids[i],true));
        widths[i]=mx(18,11+c.width(amount,8));
        if(used&&used+widths[i]>width){++rows;used=0;}used+=widths[i];
    }
    // Grow toward the actors when extra rows would enter the resting hand.
    const int top=mx(24,mn(y,174-rows*12));
    c.hit(x-2,top-2,width+4,rows*12+3,HitKind::StatusActor,actor);
    used=0;int row=0;
    for(int i=0;i<count;i++){
        if(used&&used+widths[i]>width){++row;used=0;}
        const int xx=x+used,yy=top+row*12,value=c.a.combat_status_amount(actor,ids[i],true);
        c.picture(ui_status_image(c.a,ids[i]),xx,yy,11,11);
        char amount[16]{};TextBuffer number{amount,sizeof amount,0,&c.a.content};number.number(value);
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.text(amount,xx+9+dx,yy+3+dy,8,Ink);
        c.text(amount,xx+9,yy+3,8,value<0?rgb(255,102,102):White);
        c.hit(xx-1,yy-1,widths[i],12,HitKind::BattleStatus,actor*StatusKinds+ids[i]);
        used+=widths[i];
    }
}
static void battle_status_tooltip(Canvas&c){
    auto&a=c.a;
    if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.title||a.library||a.paused||a.help||a.panel||
       a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.settings_open||a.frontend_confirm||(!combat_phase(a.game.phase)&&!a.combat_feedback.active))return;
    const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||hit->kind!=HitKind::BattleStatus)return;
    const int actor=hit->value/StatusKinds,id=hit->value%StatusKinds;
    if(actor<0||actor>a.game.enemy_count||id<0||id>=StatusKinds)return;
    char body[8192]{};ui_status_description(a,actor,id,body,sizeof body);
    const int lines=c.paragraph(body,0,0,158,0,0,10),shown=mn(lines,9),height=29+shown*13;
    const int x=a.pointer.x>=160?7:143,y=mx(25,mn(218-height,hit->y-height-5));
    const int skin=a.content.world_image("common","HOVER_TIP");
    if(skin>=0){Canvas shadow=c;shadow.tint_red=shadow.tint_green=shadow.tint_blue=0;shadow.nine_image(skin,x+2,y+2,170,height,28,22,46,16,64);c.nine_image(skin,x,y,170,height,28,22,46,16);}
    else{c.rect(x,y,170,height,Panel);c.box(x,y,170,height,Gold);}
    c.picture(ui_status_image(a,id),x+6,y+5,13,13);
    c.text(ui_status_title(a,id),x+23,y+4,10,Gold,122);
    c.num(a.combat_status_amount(actor,id,true),x+145,y+4,8,White);
    c.paragraph(body,x+6,y+23,158,shown,0,10,White);
    if(lines>shown)c.text("…",x+151,y+height-13,10,Gold);
}
static void orb_status_detail(Canvas&c){
    const auto&a=c.a;
    char heading[64]={};TextBuffer t{heading,sizeof heading,0,&c.a.content};t.text("槽位 ");t.number(a.combat_status_fact(CombatStatus::OrbCount));t.ch('/');t.number(a.combat_status_fact(CombatStatus::OrbSlots));t.text(" · 被动/激发");
    c.text(heading,145,115,10,White,155);
    c.a.status_scroll_limit=a.combat_status_fact(CombatStatus::OrbSlots)>5?5:0;
    c.a.status_scroll=c.a.status_scroll>=5&&c.a.status_scroll_limit?5:0;
    const int first=c.a.status_scroll;
    for(int i=first;i<mn(a.combat_status_fact(CombatStatus::OrbSlots),first+5);i++){
        const int y=135+(i-first)*14;
        if(i<a.combat_status_fact(CombatStatus::OrbCount)){
            const int passive=a.combat_status_fact(CombatStatus::OrbPassive,i),evoke=a.combat_status_fact(CombatStatus::OrbEvoke,i);
            c.ui(orb_image(a.combat_status_fact(CombatStatus::OrbKind,i)),145,y,11,11);
            c.text(orb_name(a.combat_status_fact(CombatStatus::OrbKind,i)),160,y-1,10,Muted,52);
            char values[32]={};TextBuffer b{values,sizeof values,0,&c.a.content};b.number(passive);b.ch('/');b.number(evoke);c.text(values,215,y-1,10,White,88);
        }else{c.box(146,y+2,8,8,Muted);c.text("空槽位",160,y-1,10,Muted,142);}
    }
    c.text(first?"较新的左侧充能球":"从最右侧依次查看",145,207,10,Muted,155);
    if(c.a.status_scroll_limit){c.text(first?"‹ 前五个":"后五个 ›",250,207,10,Gold,54);c.action(246,204,60,17,UiAction::MoreStatus);}
}
static void status_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.hit_count=0;
    c.dim(0,0,320,240,215);c.ui(U_RewardPanel,4,5,312,233);
    char heading[160]{};TextBuffer hb{heading,sizeof heading,0,&c.a.content};
    if(a.status_actor>0){hb.text(ui_enemy_name(a,a.status_actor-1));hb.text(" · ");const auto health=a.combat_health(a.status_actor);hb.number(health.hp);hb.ch('/');hb.number(health.max_hp);}else hb.text("能力与状态");
    c.centered(heading,7,9,282,12,Gold);c.close_button(289,10);
    for(int actor=0;actor<=s.enemy_count;actor++){
        int width=300/(s.enemy_count+1),x=10+actor*width;
        char label[48]={};TextBuffer t{label,sizeof label,0,&c.a.content};
        if(actor){if(s.enemy_count<=4)t.text("敌人 ");t.number(actor);if(!a.combat_health(actor).hp)t.text(" ×");}
        else t.text(s.enemy_count>4?"我方":character_definitions[(int)s.character].name);
        if(a.status_actor==actor)c.rect(x,54,width-5,1,Gold);
        c.centered(label,x,35,width-5,10,a.status_actor==actor?Gold:Muted);
        c.hit(x,33,width-5,25,HitKind::StatusActor,actor);
    }
    int ids[StatusKinds],count=a.combat_statuses(a.status_actor,ids);
    a.status_selected=mn(mx(0,a.status_selected),mx(0,count-1));
    if(!count){c.centered("当前没有能力或状态",0,113,320,12,Muted);}
    else{
        int first=a.status_selected/6*6;
        for(int i=first;i<mn(count,first+6);i++){
            int y=66+(i-first)*23,id=ids[i];bool selected=i==a.status_selected;
            if(selected){c.rect(12,y,116,22,rgb(74,58,39));c.rect(12,y,2,22,Gold);}
            c.picture(ui_status_image(a,id),17,y+3,15,15);
            c.text(ui_status_title(a,id),37,y+2,10,selected?Gold:White,87);
            c.hit(12,y,116,22,HitKind::StatusItem,i);
        }
        int id=ids[a.status_selected];
        c.picture(ui_status_image(a,id),145,65,25,25);c.num(a.combat_status_amount(a.status_actor,id,true),181,66,16,Gold);
        c.text(ui_status_title(a,id),145,95,12,Gold,155);
        char description[8192]={};ui_status_description(a,a.status_actor,id,description,sizeof description);TextBuffer t{description,sizeof description,0,&c.a.content};while(description[t.n])++t.n;
        if(id==44){
            t.text("\n辉星跨回合保留。\n本回合获得：");t.number(a.combat_status_fact(CombatStatus::StarsGained));
        }else if(id==45){
            t.text("\n最大生命：");t.number(a.combat_status_fact(CombatStatus::OstyMax));
            t.text("\n本回合攻击：");t.number(a.combat_status_fact(CombatStatus::OstyAttacks));
            t.text("\n召唤可增加生命；倒下后能被重新召唤。");
        }else if(id==DefectStatusStart+(int)DPower::EchoForm){
            t.text("\n本回合已打出：");t.number(a.combat_status_fact(CombatStatus::PhysicalStarted));
        }else if(id==DefectStatusStart+(int)DPower::Feral){
            t.text("\n本回合已触发：");t.number(a.combat_status_fact(CombatStatus::FeralUsed));
            t.text("\n本回合剩余：");t.number(mx(0,a.combat_status_amount(0,DefectStatusStart+(int)DPower::Feral)-(int)a.combat_status_fact(CombatStatus::FeralUsed)));
        }else if(id==RegentStatusStart+(int)RPower::PaleBlueDot){
            t.text("\n本回合打出：");t.number(a.combat_status_fact(CombatStatus::RegentCards));t.text(a.combat_status_fact(CombatStatus::PaleActivated)?" · 已达成":" / 5");
        }else if(id==RegentStatusStart+(int)RPower::VoidForm){
            t.text("\n本回合剩余免费次数：");t.number(mx(0,a.combat_status_amount(0,RegentStatusStart+(int)RPower::VoidForm)-(int)a.combat_status_fact(CombatStatus::VoidPlayed)));
        }else if(id==RegentStatusStart+(int)RPower::Orbit){
            t.text("\n独立进度（已花费 / 4）：");
            for(int i=0;i<MaxPowerInstances;i++)if(a.combat_status_fact(CombatStatus::Orbit,i)){t.text("\n");t.number(a.combat_status_fact(CombatStatus::OrbitRemainder,i));t.text(" / 4 → ");t.number(a.combat_status_fact(CombatStatus::Orbit,i));t.text(" 能量");}
        }else if(id==NecroStatusStart+(int)NPower::SpiritOfAsh||id==NecroStatusStart+(int)NPower::Pagestorm){
            t.text("\n本场已打出虚无牌：");t.number(a.combat_status_fact(CombatStatus::EtherealPlayed));
            t.text("\n本回合已抽牌：");t.number(a.combat_status_fact(CombatStatus::Drawn));
        }
        if(id==46&&a.status_actor==0)orb_status_detail(c);
        else{
            const int lines=c.paragraph(description,145,118,156,0,0,12,White);
            a.status_scroll_limit=mx(0,lines-6);a.status_scroll=mn(mx(0,a.status_scroll),a.status_scroll_limit);
            c.paragraph(description,145,118,156,6,a.status_scroll,12,White);
            if(a.status_scroll_limit){c.text(a.status_scroll>=a.status_scroll_limit?"回到开头":"更多说明 ›",225,207,10,Gold,79);c.action(222,204,84,17,UiAction::MoreStatus);}
        }
        if(count>6){char label[24]={};TextBuffer t{label,sizeof label,0,&c.a.content};t.number(first/6+1);t.text(" / ");t.number((count+5)/6);c.centered(label,37,208,66,10,Muted);
            c.text("‹",15,205,12,Gold);c.hit(11,204,25,18,HitKind::Scroll,-6,first>0);
            c.text("›",111,205,12,Gold);c.hit(105,204,25,18,HitKind::Scroll,6,first+6<count);}
    }
    c.centered("←→ 切换 · ↑↓ 状态 · Enter 翻说明 · V 返回",4,220,312,10,Muted);
}
