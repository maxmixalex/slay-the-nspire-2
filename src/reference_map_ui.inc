// Included inside app.cpp so map controls use the same Canvas and hit registry.
static void seed_editor(Canvas& c) {
    auto& a=c.a;auto& view=a.map_preview;a.hit_count=0;
    c.rect(0,0,320,240,Panel);c.centered("输入种子",0,3,320,16,Gold);c.close_button(294,3);
    c.rect(8,29,304,39,Ink);c.box(8,29,304,39,Gold);
    c.text(view.draft,14,32,10,White,290,2);
    c.text(*view.error?view.error:"输入后选择生成；I / O 自动转为 1 / 0",9,71,10,*view.error?Red:Muted,302);
    for(int i=0;i<40;++i) {
        int x=8+(i%10)*31,y=93+(i/10)*26;
        int code=i<SeedKeyCount?SeedKeys[i]:i==SeedKeyCount?8:-1;
        bool hover=a.pointer.visible&&a.pointer.x>=x&&a.pointer.x<x+28&&a.pointer.y>=y&&a.pointer.y<y+23;
        bool selected=hover||(!a.pointer.visible&&view.editor_selection==i);
        c.rect(x,y,28,23,selected?rgb(68,53,33):Ink);c.box(x,y,28,23,selected?Gold:rgb(78,75,62));
        char s[2]={char(code),0};const char* label=i<SeedKeyCount?s:i==SeedKeyCount?"删":"清";
        c.centered(label,x,y+2,28,12,selected?Gold:White);c.hit(x,y,28,23,HitKind::SeedCharacter,code);
    }
    c.button("返回",24,202,119,view.editor_selection==SeedKeyCount+2,28);
    c.hit(24,202,119,28,HitKind::SeedCharacter,27);
    c.button("生成地图",176,202,119,view.editor_selection==SeedKeyCount+3,28);
    c.hit(176,202,119,28,HitKind::SeedCharacter,13);
}
static void reference_map_screen(Canvas& c) {
    auto& a=c.a;auto& view=a.map_preview;const auto& map=view.map;
    if(view.editing){seed_editor(c);return;}
    a.hit_count=0;c.rect(0,0,320,240,Ink);c.picture(A_MapBackground,0,0);c.rect(0,0,320,52,Panel);
    c.text("参考地图",9,1,16,Gold);
    if(view.selected>=0&&view.selected<map.count){
        const auto& point=map.points[view.selected];
        const char* names[]={"未分配","事件","商店","宝藏","休息处","战斗","精英","首领","起点"};
        char detail[80]={};TextBuffer t{detail,sizeof detail,0};t.text(names[unsigned(point.type)]);
        t.text("  (");t.number(point.col);t.ch(',');t.number(point.row);t.ch(')');
        c.text(detail,119,6,10,Gold,168);
    }else c.text("配额已按种子生成",119,6,10,Muted);
    c.close_button(294,3);
    const char* acts[]={"Overgrowth","Underdocks","Hive","Glory"};
    char label[64]={};TextBuffer b{label,sizeof label,0};b.text(acts[unsigned(view.settings.act)]);b.text("  ›");
    c.button(label,5,26,173,true,23);c.hit(5,26,173,23,HitKind::ReferenceAction,1);
    c.button(view.settings.increased_elites?"精英配额 8":"精英配额 5",182,26,111,false,23);
    c.hit(182,26,111,23,HitKind::ReferenceAction,2);
    if(!view.generated) {
        c.centered(*view.error?view.error:"地图尚未生成",4,93,310,12,Red);
    } else {
        const int step=33,base=view.first_row;
        for(unsigned i=0;i<map.count;++i) {
            const auto& p=map.points[i];if(!p.active||p.row<base||p.row>base+3)continue;
            int x=29+p.col*42,y=162-(int(p.row)-base)*step;
            for(unsigned j=0;j<p.children.used;++j) {
                uint8_t id=p.children.slots[j];if(id==reference::NoPoint)continue;
                const auto& q=map.points[id];if(q.row>base+3)continue;
                c.line(x,y-7,29+q.col*42,y-step+7,rgb(91,69,47),true);
            }
        }
        for(unsigned i=0;i<map.count;++i) {
            const auto& p=map.points[i];if(!p.active||p.row<base||p.row>base+3)continue;
            int x=29+p.col*42,y=162-(int(p.row)-base)*step,image=A_MapCombat;
            switch(p.type) {
            case reference::PointType::Unknown:image=A_MapEvent;break;
            case reference::PointType::Shop:image=A_MapShop;break;
            case reference::PointType::Treasure:image=A_MapTreasure;break;
            case reference::PointType::Rest:image=A_MapCamp;break;
            case reference::PointType::Elite:image=A_MapFinal;break;
            case reference::PointType::Start:image=A_MapMarker;break;
            case reference::PointType::Boss:image=-1;break;
            default:break;
            }
            if(view.selected==int(i))c.picture(A_MapHalo,x-15,y-15,30,30);
            if(image>=0)c.picture(image,x-9,y-9,18,18);else c.centered("BOSS",x-20,y-9,40,10,Ink);
            c.hit(x-14,y-14,28,28,HitKind::ReferenceNode,int(i));
        }
        for(int row=base;row<=base+3;++row)c.num(row,1,156-(row-base)*step,10,Gold);
    }
    c.rect(296,52,24,24,Panel);c.centered("↑",296,54,24,12,Gold);c.hit(296,52,24,24,HitKind::Scroll,-1);
    c.rect(296,149,24,25,Panel);c.centered("↓",296,151,24,12,Gold);c.hit(296,149,24,25,HitKind::Scroll,1);
    c.rect(0,179,320,61,Panel);
    char quotas[100]={};TextBuffer q{quotas,sizeof quotas,0};
    q.text("休息 ");q.number(map.config.rests);q.text(" / 商店 ");q.number(map.config.shops);
    q.text(" / 精英 ");q.number(map.config.elites);q.text(" / 事件 ");q.number(map.config.unknowns);
    c.centered(quotas,0,180,320,10,Gold);
    c.button("‹",5,198,30,false,24);c.hit(5,198,30,24,HitKind::Command,int(Key::Left));
    c.box(39,198,242,24,Gold);c.text(view.seed,47,201,12,White,226);
    c.hit(39,198,242,24,HitKind::ReferenceAction,0);
    c.button("›",285,198,30,false,24);c.hit(285,198,30,24,HitKind::Command,int(Key::Right));
    c.centered("点种子编辑 · ↑↓ 浏览 · 尚未接入战斗",0,225,320,10,Muted);
}
