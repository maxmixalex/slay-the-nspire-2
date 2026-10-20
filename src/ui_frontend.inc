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
    c.text("nSpire II 0.17",218,219,10,White,100);
}
static void character_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;
    const auto& definition=character_definitions[(unsigned)a.selected_character];
    c.ui(character_background(a.selected_character),0,0);c.dim(0,0,150,208,140);
    c.text(definition.name,13,17,20,Gold);
    c.picture(A_Heart,15,54,15,15);char hp[32]={};TextBuffer h{hp,sizeof hp,0};h.number(definition.starting_hp);h.text(" / ");h.number(definition.starting_hp);c.text(hp,37,51,12,White);
    c.paragraph(character_summary(a.selected_character),14,78,127,4,0,12,White);
    if(const auto*relic=relic_definition(definition.starting_relic)){
        c.picture(a.content.relic_image((int)(relic-relic_definitions)),14,142,25,25);
        c.text(relic->name,46,145,12,Gold,99);
        c.paragraph(relic->description,14,175,130,2,0,10,White);
    }
    c.dim(0,209,320,31,130);
    c.button("返回",3,216,82,false,23);c.action(1,209,86,31,UiAction::Back);
    const UiAction actions[]={UiAction::SelectIronclad,UiAction::SelectSilent,UiAction::SelectDefect,UiAction::SelectRegent,UiAction::SelectNecrobinder};
    for(int i=0;i<CharacterKinds;i++){
        int x=159+i*31;bool selected=i==(int)a.selected_character;
        if(selected){c.rect(x-2,173,31,35,rgb(87,68,38));c.box(x-2,173,31,35,Gold);}
        c.ui(character_portrait((Character)i),x,175,27,30,selected?255:145);
        c.action(x-2,172,31,36,actions[i]);
    }
    c.ui(U_RewardSkip,218,211,99,27);c.centered(a.practice_entry?"练习 ›":"启程 ›",218,214,99,12,Gold);c.action(213,207,107,33,UiAction::StartCharacter);
}
static void library_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.ui(U_TitleBackground,0,0);c.dim(0,0,320,240,225);
    c.centered("图鉴",0,1,320,16,Gold);c.close_button(294,3);
    const char* tabs[]={"卡牌","遗物","药水"};
    const int totals[]={(int)a.library_items(),WorldRelicKinds,PotionKinds};
    for(int i=0;i<3;i++){
        int x=5+i*104;char label[32]={};TextBuffer b{label,sizeof label,0};b.text(tabs[i]);b.ch(' ');b.number(i==0?CardKinds:totals[i]);
        c.ui(U_LibraryCards+i,x+6,24,18,18);c.text(label,x+29,25,10,i==a.library_tab?Gold:Muted);
        if(i==a.library_tab)c.rect(x+6,43,91,1,Gold);
        c.action(x,22,104,23,(UiAction)((int)UiAction::LibraryCards+i));
    }
    if(a.library_tab==0){
        const char* filters[]={"全部","铁甲","猎手","故障","储君","亡灵","无色"};
        const int filter_ids[]={0,1,2,4,5,6,3};
        const UiAction filter_actions[]={UiAction::FilterAll,UiAction::FilterIronclad,UiAction::FilterSilent,UiAction::FilterDefect,UiAction::FilterRegent,UiAction::FilterNecrobinder,UiAction::FilterColorless};
        for(int i=0;i<7;i++){int x=4+i*33;bool selected=a.library_filter==filter_ids[i];if(selected)c.rect(x,46,32,18,rgb(67,54,38));
            c.centered(filters[i],x,46,32,10,selected?Gold:Muted);c.action(x,45,33,19,filter_actions[i]);}
        const char* sorts[]={"默认排序","按费用","按类型"};c.centered(sorts[a.library_sort],238,46,79,10,Gold);c.action(237,45,83,19,UiAction::CycleSort);
        CardKind ids[CardKinds];const int count=(int)a.library_items(ids),last=mx(0,(count-1)/8);
        a.library_page=mn(mx(0,a.library_page),last);a.library_selected=mn(mx(0,a.library_selected),count-1);
        for(int i=a.library_page*8;i<mn(count,a.library_page*8+8);i++){
            int k=i-a.library_page*8,x=29+(k%4)*69,y=68+(k/4)*71;
            small_card(c,{ids[i],(uint8_t)(a.library_upgraded&&card_upgradeable({ids[i],0}))},x,y,i==a.library_selected,true,0);
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
        int index=mn(count-1,mx(0,a.library_selected));const char*name;const char*desc;char description[2048]{};
        if(a.library_tab==1){name=a.relic_title(RelicId(index));a.relic_description(RelicId(index),description,sizeof description);desc=description;}
        else{name=potion_definitions[index].name;desc=potion_definitions[index].description;}
        c.centered(name,10,163,300,16,Gold);c.paragraph(desc,20,189,282,2,0,12,White);
        c.text("‹",12,215,16,Gold);c.action(0,211,44,29,UiAction::PreviousPage,a.library_page>0);
        c.text("›",296,215,16,Gold);c.action(277,211,43,29,UiAction::NextPage,a.library_page<last);
        char page[64]={};TextBuffer b{page,sizeof page,0};b.number(a.library_page+1);b.text(" / ");b.number(last+1);b.text(" · 点击查看完整详情");c.centered(page,40,222,236,10,Muted);
    }
}

static void item_detail_screen(Canvas&c){
    auto&a=c.a;a.hit_count=0;c.dim(0,0,320,240,190);c.ui(U_RewardPanel,43,26,234,206);
    bool relic=a.detail_item<WorldRelicKinds;int index=relic?a.detail_item:a.detail_item-WorldRelicKinds;
    if(index<0||index>=(relic?WorldRelicKinds:PotionKinds)){a.detail_item=-1;return;}
    const char*name=relic?a.relic_title(RelicId(index)):potion_definitions[index].name;
    char description[4096]{};if(relic)a.relic_description(RelicId(index),description,sizeof description,a.detail_owned);
    const char*desc=relic?description:potion_definitions[index].description;
    int image=relic?a.content.relic_image(index):a.content.potion_image((Potion)(index+1));
    c.picture(image,140,40,40,46);c.centered(name,58,91,204,16,Gold);
    bool owned=relic&&a.detail_owned>=0&&a.detail_owned<int(ui_owned_relic_count(a.game))&&ui_owned_relic_id(a.game,a.detail_owned)==index;
    char progress[128]{};if(owned)ui_relic_progress(a.game,a.detail_owned,progress,sizeof progress);int top=*progress?136:119;
    if(*progress)c.centered(progress,57,116,206,10,Green);
    int visible=top==136?4:5;
    int lines=c.paragraph(desc,67,top,188,0);a.detail_scroll=mn(a.detail_scroll,mx(0,lines-visible));
    c.paragraph(desc,67,top,188,visible,a.detail_scroll,12,White);
    if(lines>visible){c.text("↑",260,top,10,Gold);c.hit(251,top-2,26,20,HitKind::Scroll,-1,a.detail_scroll>0);
        c.text("↓",260,178,10,Gold);c.hit(251,175,26,21,HitKind::Scroll,1,a.detail_scroll+visible<lines);}
    const bool lab=!relic&&a.library&&!a.title&&!a.paused&&!a.help&&!a.panel&&a.game.practice&&a.game.phase==Phase::Combat;
    if(lab){
        bool room=false;for(unsigned i=0;i<potion_slot_count(a.game);i++)room|=potion_at(a.game,i)==Potion::Empty;
        c.button("返回",57,201,72,false,25);c.action(53,198,80,32,UiAction::Back);
        c.button(room?"获得药水":"药水栏已满",139,201,127,room,25);
        c.action(135,198,135,32,UiAction::LabPotion,room);
    }else{
        c.button("返回",112,201,96,true,25);c.action(107,198,106,32,UiAction::Back);
    }
}
