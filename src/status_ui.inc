// Original power icons and textured panels, reflowed for a 320x240 display.
#include "world_status_ui.inc"
static void status_strip(Canvas&c,int actor,int x,int y,int width){
    uint16_t tokens[MaxPlayerPowerEntries]{};int count=0;
    const unsigned all=c.a.combat_power_entries(actor,tokens,MaxPlayerPowerEntries);
    // NPowerContainer.Add omits IsVisible=false models. Resources have
    // their own original controls; the details browser can still expose
    // their auxiliary facts without inventing duplicate Power icons.
    for(unsigned i=0;i<all;++i)if(CombatStatus::PowerFlags[power_id(tokens[i])]&1)tokens[count++]=tokens[i];
    if(!count)return;
    const auto&a=c.a;
    const auto scene=a.creature_layout.right?a.creature_layout:combat_layout_for(a.game);
    const int cell=a.game.enemy_world.active?mx(1,combat_project_extent(encounter_camera(a.game.enemy_world.encounter),scene,48)):12;
    const auto layout=PowerStripLayout::make(x,y,width,count,cell);
    const int icon=mx(1,(cell*40+24)/48),label_right=(cell*44+24)/48,label_y=cell*21/48;
    c.hit(layout.left-1,layout.top-1,layout.width+2,layout.rows*cell+2,HitKind::StatusActor,actor);
    for(int i=0;i<count;i++){
        const uint16_t token=tokens[i];const unsigned id=power_id(token);
        const int xx=layout.x(i),yy=layout.y(i),value=c.a.combat_power_amount(token,true);
        c.picture(ui_status_image(c.a,int(id)),xx,yy,icon,icon);
        if(c.a.combat_power_counter(token)){
        char amount[16]{};TextBuffer number{amount,sizeof amount,0,&c.a.content};number.number(value);
        const int tx=xx+label_right-c.width(amount,8),ty=yy+label_y;
        for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(dx||dy)c.text(amount,tx+dx,ty+dy,8,Ink);
        c.text(amount,tx,ty,8,value<0?rgb(255,102,102):White);}
        c.hit(xx-1,yy-1,cell,cell,power_instanced(id)?HitKind::PowerInstance:HitKind::BattleStatus,power_instanced(id)?token:actor*StatusKinds+int(id));
    }
}
bool power_hover_target(const App&a,CardHoverTarget&target){
    if(!a.pointer.visible||a.pointer.held||a.pointer.armed_card>=0||a.title||a.library||a.paused||a.help||a.panel||
       a.keyboard_card>=0||a.detail_card>=0||a.detail_item>=0||a.status_actor>=0||a.settings_open||a.frontend_confirm||(!combat_phase(a.game.phase)&&!a.combat_feedback.active&&!a.choice_peeking))return false;
    const auto*hit=a.hit_at(a.pointer.x,a.pointer.y);if(!hit||(hit->kind!=HitKind::BattleStatus&&hit->kind!=HitKind::PowerInstance))return false;
    const bool instanced=hit->kind==HitKind::PowerInstance;
    const int actor=instanced?0:hit->value/StatusKinds,id=instanced?(hit->value&255):hit->value%StatusKinds;
    if(actor<0||actor>a.game.enemy_count||id<0||id>=StatusKinds||!(CombatStatus::PowerFlags[id]&1))return false;
    const uint16_t token=instanced?uint16_t(hit->value):power_token(unsigned(actor),unsigned(id));
    // NPower delegates to its NCreature owner. The actual source Hitbox,
    // not the small power icon or pointer, owns the complete hover-tip set.
    const auto pose=actor?a.combat_enemy_pose(actor-1):a.combat_player_pose();
    target={};target.owner={pose.body_left,pose.body_top,pose.body_width,pose.body_height};
    target.power=int(token);target.power_left=pose.x>=160;target.identity=0x73000000u|token;
    static constexpr CardKind cards[]={Strike,StrikeSilent,DefectStrike,RegentStrike,NecroStrikeNecrobinder};
    target.card={cards[unsigned(a.game.character)<5?unsigned(a.game.character):0],0};return true;
}
static void orb_status_detail(Canvas&c){
    const auto&a=c.a;
    char heading[64]={};TextBuffer t{heading,sizeof heading,0,&c.a.content};t.text("槽位 ");t.number(a.combat_status_fact(CombatStatus::OrbCount));t.ch('/');t.number(a.combat_status_fact(CombatStatus::OrbSlots));t.text("  被动/激发");
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
    if(a.status_actor>0){hb.text(ui_enemy_name(a,a.status_actor-1));hb.text("  ");const auto health=a.combat_health(a.status_actor);hb.number(health.hp);hb.ch('/');hb.number(health.max_hp);}else hb.text("能力与状态");
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
        c.picture(ui_status_image(a,id),145,65,25,25);if(a.combat_status_counter(a.status_actor,id))c.num(a.combat_status_amount(a.status_actor,id,true),181,66,16,Gold);
        c.text(ui_status_title(a,id),145,95,12,Gold,155);
        ForegroundScratch<char,8192> storage;if(!storage)return;auto&description=storage.array();description[0]=0;
        ui_status_description(a,a.status_actor,id,description,sizeof description);TextBuffer t{description,sizeof description,0,&c.a.content};while(description[t.n])++t.n;
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
            t.text("\n本回合打出：");t.number(a.combat_status_fact(CombatStatus::RegentCards));t.text(a.combat_status_fact(CombatStatus::PaleActivated)?"  已达成":" / 5");
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

}
