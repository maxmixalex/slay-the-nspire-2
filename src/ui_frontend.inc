// Original scene textures and interaction flow, reflowed for 320x240.
static void title_screen(Canvas&c){
    auto&a=c.a;c.ui(U_TitleBackground,0,0);c.picture(A_Logo,3,5);
    const char*labels[]={"开始游戏","继续游戏","图鉴","组合练习","参考地图","操作说明","退出游戏"};
    for(int i=0;i<7;i++){
        int y=84+i*20;bool selected=a.menu==i;
        if(selected)c.dim(6,y,146,20,100);
        if(selected){c.rect(9,y+5,2,13,Gold);c.text("›",19,y,12,Gold);}
        c.text(labels[i],35,y+1,12,Ink);c.text(labels[i],34,y,12,selected?Gold:White);
        c.hit(7,y,147,20,HitKind::Menu,i);
    }
    c.text("nSpire II 0.15",218,219,10,White,100);
}
static void character_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;bool silent=a.selected_character==Character::Silent;
    const auto& definition=character_definitions[(unsigned)a.selected_character];
    c.ui(silent?U_SilentBackground:U_CharacterBackground,0,0);c.dim(0,0,150,208,140);
    c.text(definition.name,13,17,20,Gold);
    c.picture(A_Heart,15,54,15,15);char hp[32]={};TextBuffer h{hp,sizeof hp,0};h.number(definition.starting_hp);h.text(" / ");h.number(definition.starting_hp);c.text(hp,37,51,12,White);
    c.paragraph(silent?"一位来自尖塔之外的女猎手。\n随时准备刀刺与毒杀任何拦路者。":"铁甲军团最后的士兵。\n并非出自自身的意愿，用刀剑与烈焰击溃敌人们。",14,78,127,4,0,12,White);
    c.picture(silent?a.content.ui_image(U_SnakeRing):A_BurningBlood,14,145,25,25);
    c.text(silent?"蛇之戒":"燃烧之血",46,148,12,Gold);
    c.paragraph(silent?"每场战斗的首个回合，多抽 [green]2[/green] 张牌。":"每场战斗结束时，回复 [green]6[/green] 点生命。",14,177,130,2,0,10,White);
    c.dim(0,209,320,31,130);
    c.button("返回",3,216,82,false,23);c.action(1,209,86,31,UiAction::Back);
    for(int i=0;i<2;i++){
        int x=115+i*43;bool selected=i==(int)a.selected_character;
        if(selected){c.rect(x-3,203,39,37,rgb(87,68,38));c.box(x-3,203,39,37,Gold);}
        c.ui(i?U_SilentPortrait:U_CharPortrait,x,203,32,36,selected?255:145);
        c.action(x-3,202,39,38,i?UiAction::SelectSilent:UiAction::SelectIronclad);
    }
    c.ui(U_RewardSkip,218,211,99,27);c.centered(a.practice_entry?"练习 ›":"启程 ›",218,214,99,12,Gold);c.action(213,207,107,33,UiAction::StartCharacter);
}
static void library_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.ui(U_TitleBackground,0,0);c.dim(0,0,320,240,225);
    c.centered("图鉴",0,1,320,16,Gold);c.close_button(294,3);
    const char* tabs[]={"卡牌","遗物","药水"};
    const int totals[]={(int)a.library_items(),RelicKinds,PotionKinds};
    for(int i=0;i<3;i++){
        int x=5+i*104;char label[32]={};TextBuffer b{label,sizeof label,0};b.text(tabs[i]);b.ch(' ');b.number(i==0?CardKinds:totals[i]);
        c.ui(U_LibraryCards+i,x+6,24,18,18);c.text(label,x+29,25,10,i==a.library_tab?Gold:Muted);
        if(i==a.library_tab)c.rect(x+6,43,91,1,Gold);
        c.action(x,22,104,23,(UiAction)((int)UiAction::LibraryCards+i));
    }
    if(a.library_tab==0){
        const char* filters[]={"全部","铁甲","猎手","无色"};
        for(int i=0;i<4;i++){int x=4+i*48;if(a.library_filter==i)c.rect(x,46,46,18,rgb(67,54,38));
            c.centered(filters[i],x,46,46,10,a.library_filter==i?Gold:Muted);c.action(x,45,47,19,(UiAction)((int)UiAction::FilterAll+i));}
        const char* sorts[]={"默认排序","按费用","按类型"};c.text(sorts[a.library_sort],230,46,10,Gold,85);c.action(220,45,100,19,UiAction::CycleSort);
        uint8_t ids[CardKinds];const int count=(int)a.library_items(ids),last=mx(0,(count-1)/8);
        a.library_page=mn(mx(0,a.library_page),last);a.library_selected=mn(mx(0,a.library_selected),count-1);
        for(int i=a.library_page*8;i<mn(count,a.library_page*8+8);i++){
            int k=i-a.library_page*8,x=29+(k%4)*69,y=68+(k/4)*71;
            small_card(c,{ids[i],(uint8_t)a.library_upgraded},x,y,i==a.library_selected,true,0);
            c.hit(x-3,y-2,55,70,HitKind::LibraryItem,i);
        }
        c.action(108,211,116,29,UiAction::ToggleUpgrades);c.centered(a.library_upgraded?"升级版本 +":"基础版本",108,215,116,12,Green);
        c.text("‹",12,214,16,Gold);c.action(1,211,43,29,UiAction::PreviousPage,a.library_page>0);
        c.text("›",296,214,16,Gold);c.action(277,211,43,29,UiAction::NextPage,a.library_page<last);
        char page[32]={};TextBuffer b{page,sizeof page,0};b.number(a.library_page+1);b.text(" / ");b.number(last+1);c.text(page,51,218,10,Muted);
    }else{
        const int count=totals[a.library_tab];
        const int last=(count-1)/18;a.library_page=mn(mx(0,a.library_page),last);int first=a.library_page*18;
        for(int i=first;i<mn(count,first+18);i++){
            int j=i-first;int x=22+(j%6)*49,y=53+(j/6)*35;bool selected=i==a.library_selected;
            if(selected){c.rect(x-4,y-2,38,34,rgb(75,58,37));c.box(x-4,y-2,38,34,Gold);}
            int image=a.library_tab==1?a.content.relic_image(i):a.content.potion_image((Potion)(i+1));
            c.picture(image,x+2,y,25,29);c.hit(x-5,y-3,47,35,HitKind::LibraryItem,i);
        }
        int index=mn(count-1,mx(0,a.library_selected));const char*name;const char*desc;
        if(a.library_tab==1){name=relic_definitions[index].name;desc=relic_definitions[index].description;}
        else{name=potion_definitions[index].name;desc=potion_definitions[index].description;}
        c.centered(name,10,163,300,16,Gold);c.paragraph(desc,20,189,282,2,0,12,White);
        c.text("‹",12,215,16,Gold);c.action(0,211,44,29,UiAction::PreviousPage,a.library_page>0);
        c.text("›",296,215,16,Gold);c.action(277,211,43,29,UiAction::NextPage,a.library_page<last);
        char page[64]={};TextBuffer b{page,sizeof page,0};b.number(a.library_page+1);b.text(" / ");b.number(last+1);b.text(" · 点击查看完整详情");c.centered(page,40,222,236,10,Muted);
    }
}

static void item_detail_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,190);c.ui(U_RewardPanel,43,26,234,206);
    bool relic=a.detail_item<RelicKinds;int index=relic?a.detail_item:a.detail_item-RelicKinds;
    if(index<0||index>=(relic?RelicKinds:PotionKinds)){a.detail_item=-1;return;}
    const char*name=relic?relic_definitions[index].name:potion_definitions[index].name;
    const char*desc=relic?relic_definitions[index].description:potion_definitions[index].description;
    int image=relic?a.content.relic_image(index):a.content.potion_image((Potion)(index+1));
    c.picture(image,140,40,40,46);c.centered(name,58,91,204,16,Gold);
    bool owned=relic&&a.detail_owned>=0&&a.detail_owned<a.game.relic_count&&a.game.relic_order[a.detail_owned]==index;
    int top=owned&&relic_counter_limit(relic_definitions[index].id)?136:119;
    if(top==136){char progress[96]={};relic_progress(a.game,relic_definitions[index].id,progress,sizeof progress);c.centered(progress,57,116,206,10,Green);}
    int visible=top==136?4:5;
    int lines=c.paragraph(desc,67,top,188,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-visible));
    c.paragraph(desc,67,top,188,visible,a.detail_scroll,12,White);
    if(lines>visible){c.text("↑",260,top,10,Gold);c.hit(251,top-2,26,20,HitKind::Scroll,-1,a.detail_scroll>0);
        c.text("↓",260,178,10,Gold);c.hit(251,175,26,21,HitKind::Scroll,1,a.detail_scroll+visible<lines);}
    const bool lab=!relic&&a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
    if(lab){
        bool room=false;for(auto p:a.game.potions)room|=p==Potion::Empty;
        c.button("返回",57,201,72,false,25);c.action(53,198,80,32,UiAction::Back);
        c.button(room?"获得药水":"药水栏已满",139,201,127,room,25);
        c.action(135,198,135,32,UiAction::LabPotion,room);
    }else{
        c.button("返回",112,201,96,true,25);c.action(107,198,106,32,UiAction::Back);
    }
}
