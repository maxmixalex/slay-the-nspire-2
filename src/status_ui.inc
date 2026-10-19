// Original power icons and textured panels, reflowed for a 320x240 display.
static void status_strip(Canvas&c,int actor,int x,int y,int width){
    int ids[StatusKinds],count=collect_statuses(c.a.game,actor,ids);
    if(!count)return;
    int slots=mx(1,width/30),visible=mn(count,slots);
    if(count>slots)--visible;
    for(int i=0;i<visible;i++){
        int xx=x+i*30,value=status_amount(c.a.game,actor,ids[i]);
        c.ui(status_definitions[ids[i]].image,xx,y,13,13);
        if(value<100)c.num(value,xx+14,y-1,10,White);else c.text("99+",xx+13,y-1,10,White);
    }
    if(count>visible){char b[16]={};TextBuffer t{b,sizeof b,0};t.ch('+');t.number(count-visible);c.text(b,x+visible*30,y-1,10,Gold);}
    c.hit(x-2,y-2,width+4,19,HitKind::StatusActor,actor);
}
static void orb_status_detail(Canvas&c){
    const auto&s=c.a.game;
    char heading[64]={};TextBuffer t{heading,sizeof heading,0};t.text("槽位 ");t.number(s.expansion.orb_count);t.ch('/');t.number(s.expansion.orb_slots);t.text(" · 被动/激发");
    c.text(heading,145,115,10,White,155);
    c.a.status_scroll_limit=s.expansion.orb_slots>5?5:0;
    c.a.status_scroll=c.a.status_scroll>=5&&c.a.status_scroll_limit?5:0;
    const int first=c.a.status_scroll;
    for(int i=first;i<mn(s.expansion.orb_slots,first+5);i++){
        const int y=135+(i-first)*14;
        if(i<s.expansion.orb_count){
            int passive,evoke;orb_values(s,i,passive,evoke);
            c.ui(orb_image(s.defect.orb_kind[i]),145,y,11,11);
            c.text(orb_name(s.defect.orb_kind[i]),160,y-1,10,Muted,52);
            char values[32]={};TextBuffer b{values,sizeof values,0};b.number(passive);b.ch('/');b.number(evoke);c.text(values,215,y-1,10,White,88);
        }else{c.box(146,y+2,8,8,Muted);c.text("空槽位",160,y-1,10,Muted,142);}
    }
    c.text(first?"较新的左侧充能球":"从最右侧依次查看",145,207,10,Muted,155);
    if(c.a.status_scroll_limit){c.text(first?"‹ 前五个":"后五个 ›",250,207,10,Gold,54);c.action(246,204,60,17,UiAction::MoreStatus);}
}
static void status_screen(Canvas&c){
    auto&a=c.a;auto&s=a.game;a.hit_count=0;
    c.dim(0,0,320,240,215);c.ui(U_RewardPanel,4,5,312,233);
    c.centered("能力与状态",0,9,320,16,Gold);c.close_button(289,10);
    for(int actor=0;actor<=s.enemy_count;actor++){
        int width=300/(s.enemy_count+1),x=10+actor*width;
        char label[48]={};TextBuffer t{label,sizeof label,0};
        if(actor){t.text("敌人 ");t.number(actor);if(!s.enemies[actor-1].hp)t.text(" · 已击败");}
        else t.text(character_definitions[(int)s.character].name);
        if(a.status_actor==actor)c.rect(x,54,width-5,1,Gold);
        c.centered(label,x,35,width-5,10,a.status_actor==actor?Gold:Muted);
        c.hit(x,33,width-5,25,HitKind::StatusActor,actor);
    }
    int ids[StatusKinds],count=collect_statuses(s,a.status_actor,ids);
    a.status_selected=mn(mx(0,a.status_selected),mx(0,count-1));
    if(!count){c.centered("当前没有能力或状态",0,113,320,12,Muted);}
    else{
        int first=a.status_selected/6*6;
        for(int i=first;i<mn(count,first+6);i++){
            int y=66+(i-first)*23,id=ids[i];bool selected=i==a.status_selected;
            if(selected){c.rect(12,y,116,22,rgb(74,58,39));c.rect(12,y,2,22,Gold);}
            c.ui(status_definitions[id].image,17,y+3,15,15);
            c.text(status_definitions[id].name,37,y+2,10,selected?Gold:White,87);
            c.hit(12,y,116,22,HitKind::StatusItem,i);
        }
        int id=ids[a.status_selected];const auto&d=status_definitions[id];
        c.ui(d.image,145,65,25,25);c.num(status_amount(s,a.status_actor,id),181,66,16,Gold);
        c.text(d.name,145,95,12,Gold,155);
        char description[768]={};TextBuffer t{description,sizeof description,0};
        for(const char*p=d.description;*p;){
            if(p[0]=='{'&&p[1]=='n'&&p[2]=='}'){t.number(status_amount(s,a.status_actor,id));p+=3;}
            else t.ch(*p++);
        }
        if(id==43){
            t.text("\n");for(int i=0;i<MaxBombs;i++)if(s.expansion.bomb_turns[i]){t.number(s.expansion.bomb_turns[i]);t.text(" 回合 / ");t.number(s.expansion.bomb_damage[i]);t.text(" 伤害\n");}
        }else if(id==44){
            t.text("\n星星跨回合保留。\n本回合获得：");t.number(s.regent.stars_gained_turn);
        }else if(id==45){
            t.text("\n最大生命：");t.number(s.expansion.osty_max_hp);
            t.text("\n本回合攻击：");t.number(s.necro.osty_attacks_turn);
            t.text("\n召唤可增加生命；倒下后能被重新召唤。");
        }else if(id==DefectStatusStart+(int)DPower::EchoForm){
            t.text("\n本回合已打出：");t.number(s.defect.physical_started);
        }else if(id==DefectStatusStart+(int)DPower::Feral){
            t.text("\n本回合已触发：");t.number(s.defect.feral_used);
            t.text("\n本回合剩余：");t.number(mx(0,s.defect.powers[(int)DPower::Feral]-(int)s.defect.feral_used));
        }else if(id==RegentStatusStart+(int)RPower::PaleBlueDot){
            t.text("\n本回合打出：");t.number(s.regent.cards_finished_turn);t.text(s.regent.pale_activated?" · 已达成":" / 5");
        }else if(id==RegentStatusStart+(int)RPower::VoidForm){
            t.text("\n本回合剩余免费次数：");t.number(mx(0,s.regent.powers[(int)RPower::VoidForm]-(int)s.regent.void_played_turn));
        }else if(id==RegentStatusStart+(int)RPower::Orbit){
            t.text("\n独立进度（已花费 / 4）：");
            for(int i=0;i<MaxPowerInstances;i++)if(s.regent.orbit[i]){t.text("\n");t.number(s.regent.orbit_remainder[i]);t.text(" / 4 → ");t.number(s.regent.orbit[i]);t.text(" 能量");}
        }else if(id==RegentStatusStart+(int)RPower::Monologue){
            int strength=0;for(auto n:s.regent.monologue_applied)strength+=n;t.text("\n本回合临时力量：");t.number(strength);
        }else if(id==NecroStatusStart+(int)NPower::SpiritOfAsh||id==NecroStatusStart+(int)NPower::Pagestorm){
            t.text("\n本场已打出虚无牌：");t.number(s.necro.ethereal_played);
            t.text("\n本回合已抽牌：");t.number(s.necro.drawn_turn);
        }
        if(id==46&&a.status_actor==0)orb_status_detail(c);
        else{
            const int lines=c.paragraph(description,145,118,156,0,0,12,White);
            a.status_scroll_limit=mx(0,lines-6);a.status_scroll=mn(mx(0,a.status_scroll),a.status_scroll_limit);
            c.paragraph(description,145,118,156,6,a.status_scroll,12,White);
            if(a.status_scroll_limit){c.text(a.status_scroll>=a.status_scroll_limit?"回到开头":"更多说明 ›",225,207,10,Gold,79);c.action(222,204,84,17,UiAction::MoreStatus);}
        }
        if(count>6){char label[24]={};TextBuffer t{label,sizeof label,0};t.number(first/6+1);t.text(" / ");t.number((count+5)/6);c.centered(label,37,208,66,10,Muted);
            c.text("‹",15,205,12,Gold);c.hit(11,204,25,18,HitKind::Scroll,-6,first>0);
            c.text("›",111,205,12,Gold);c.hit(105,204,25,18,HitKind::Scroll,6,first+6<count);}
    }
    c.centered("←→ 切换 · ↑↓ 状态 · Enter 翻说明 · V 返回",4,220,312,10,Muted);
}
