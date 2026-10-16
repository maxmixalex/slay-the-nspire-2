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
        char description[512]={};TextBuffer t{description,sizeof description,0};
        for(const char*p=d.description;*p;){
            if(p[0]=='{'&&p[1]=='n'&&p[2]=='}'){t.number(status_amount(s,a.status_actor,id));p+=3;}
            else t.ch(*p++);
        }
        if(id==43){
            t.text("\n");for(int i=0;i<MaxBombs;i++)if(s.expansion.bomb_turns[i]){t.number(s.expansion.bomb_turns[i]);t.text(" 回合 / ");t.number(s.expansion.bomb_damage[i]);t.text(" 伤害\n");}
        }else if(id==46){
            t.text("\n蓄能：");for(int i=0;i<s.expansion.orb_count;i++){if(i)t.text(" / ");t.number(s.expansion.dark[i]);}
        }
        c.paragraph(description,145,118,156,6,0,12,White);
        if(count>6){char label[24]={};TextBuffer t{label,sizeof label,0};t.number(first/6+1);t.text(" / ");t.number((count+5)/6);c.centered(label,37,208,66,10,Muted);
            c.text("‹",15,205,12,Gold);c.hit(11,204,25,18,HitKind::Scroll,-6,first>0);
            c.text("›",111,205,12,Gold);c.hit(105,204,25,18,HitKind::Scroll,6,first+6<count);}
    }
    c.centered("←→ 切换角色 · ↑↓ 查看 · V / Esc 返回",4,220,312,10,Muted);
}
