// Actual-locale checks for the remaining native NHoverTipSet owners. The
// independently authored 360/1680 column projects to 69px; a shadow adds 1.
static void static_hover_columns(){
    if(app.content.version()<HistoryUi8ContentVersion)return;
    Canvas c{app};static SaveData before,after;
    const auto setup=[&](Character character){
        new_run(app.game,0x98114,true,character);CHECK(encounter_setup(app.game,EncounterId::NibbitsNormal));
        app.title=app.character_select=app.library=app.paused=app.help=app.settings_open=false;
        app.frontend_confirm=0;app.panel=0;app.detail_card=app.detail_item=app.status_actor=-1;
        app.reset_presentation();app.pointer_cancel();app.pointer.visible=true;app.pointer.armed_card=-1;
    };
    const auto column=[&](int x){
        unsigned changed=0;for(int yy=0;yy<240;++yy)for(int xx=0;xx<320;++xx)
            if(app.pixels[yy*320+xx]!=0x0321){++changed;CHECK(xx>=x&&xx<x+70);}
        CHECK(changed>25);CHECK(save(app.game,after));
        CHECK(before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    };
    for(unsigned resource=0;resource<3;++resource){
        setup(resource?Character::Regent:Character::Ironclad);
        app.pointer.x=resource==2?20:25;app.pointer.y=resource==2?207:resource?161:199;
        CHECK(save(app.game,before));clear();combat_resource_tooltip(c,resource!=0);column(6);
        capture(resource==2?"source-hover-stars":resource?"source-hover-regent-energy":"source-hover-energy");
    }
    setup(Character::Defect);app.game.expansion.orb_slots=3;app.game.expansion.orb_count=1;
    app.game.defect.orb_kind[0]=unsigned(DOrb::Lightning);
    for(unsigned mode=0;mode<4;++mode){
        const int x=mode&1?260:42,index=mode<2?0:2;
        app.pointer.x=x+6;app.pointer.y=76;CHECK(save(app.game,before));clear();
        c.hit(x,70,14,14,HitKind::Orb,index);orb_hover_tip(c);column(mode&1?191:56);
        const char*names[]={"source-hover-orb-right","source-hover-orb-left","source-hover-empty-orb-right","source-hover-empty-orb-left"};capture(names[mode]);
    }
    // Pinned settings_screen.tscn: General begins at187+24, with64px rows
    // and82px pitch; Keyboard-Only starts at251. These coordinates and the
    // expected localized bindings are independent of the Settings renderer's
    // row lookup. General no longer scrolls in the implemented panel.
    setup(Character::Ironclad);app.title=true;app.settings_open=true;
    const auto project=[](int source){return (source*4+10)/21;};
    struct SettingHover {unsigned tab,row;int left,top;const char*title;const char*body;const char*capture;};
    const SettingHover settings[]={
        {0,1,334,211+82,"SCREENSHAKE_HEADER","SCREENSHAKE_DESCRIPTION","settings-source-hover-shake"},
        {0,7,334,211+82*7,"TEXT_EFFECTS","TEXT_EFFECTS_DESCRIPTION","settings-source-hover-text-effects"},
        {0,6,334,211+82*6,"LONG_PRESS_CONFIRMATION_HEADER","LONG_PRESS_CONFIRMATION_DESCRIPTION","settings-source-hover-long-press"},
        {1,0,290,251,"KEYBOARD_ONLY_MODE_HEADER","KEYBOARD_ONLY_MODE_DESCRIPTION","settings-source-hover-keyboard"},
    };
    CHECK(project(293+32)==62&&project(785+32)==156&&project(703+32)==140&&project(251+32)==54);
    CHECK(save(app.game,before));
    for(const auto&setting:settings){
        app.settings_tab=uint8_t(setting.tab);app.settings_top=0;app.settings_item=uint8_t(setting.row);
        app.pointer.x=319;app.pointer.y=239;clear();frontend_settings_screen(c);
        const std::vector<uint16_t> background(app.pixels,app.pixels+ScreenW*ScreenH);
        // Source x786 lies in the label/hover region, outside every tickbox.
        app.pointer.x=project(786);app.pointer.y=project(setting.top+32);
        clear();frontend_settings_screen(c);capture(setting.capture);
        const std::vector<uint16_t> actual(app.pixels,app.pixels+ScreenW*ScreenH);
        const std::string title=app.content.world_string("settings_ui",setting.title);
        const std::string body=app.content.world_string("settings_ui",setting.body);
        CHECK(!title.empty()&&!body.empty());
        std::memcpy(app.pixels,background.data(),background.size()*sizeof(uint16_t));
        // NSettings hover offset is the1012px column width and -60px y.
        // Reusing the common tooltip painter tests Settings ownership and
        // binding, not the independent correctness of that common painter.
        source_hover_box(c,title.c_str(),body.c_str(),project(setting.left)+project(1012),project(setting.top)-project(60));
        unsigned changed=0;
        for(unsigned i=0;i<actual.size();++i){
            if(actual[i]!=app.pixels[i])std::fprintf(stderr,"Settings hover %s mismatch at%u,%u: actual=%04x expected=%04x\n",setting.title,i%ScreenW,i/ScreenW,actual[i],app.pixels[i]);
            CHECK(actual[i]==app.pixels[i]);changed+=actual[i]!=background[i];
        }
        CHECK(changed>25);
        CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
    }
    app.settings_open=false;app.settings_tab=app.settings_top=app.settings_item=0;app.title=false;
    // The map legend tooltip stays below the whole column. Only its focus
    // icon and the 69px tooltip may change between the two paint passes.
    app.pointer.visible=false;app.map_legend=false;clear();map_legend_screen(c,map_colors(0));
    const std::vector<uint16_t> legend(app.pixels,app.pixels+320*240);
    app.map_legend=true;app.map_legend_item=1;clear();map_legend_screen(c,map_colors(0));
    unsigned changed=0;for(int y=0;y<240;++y)for(int x=0;x<320;++x)if(legend[y*320+x]!=app.pixels[y*320+x]){
        ++changed;CHECK((x>=248&&x<318)||(x>=258&&x<281&&y>=102&&y<127));
    }
    CHECK(changed>25);capture("map-legend-source-hover");app.map_legend=false;
    app.pointer.visible=false;app.reset_presentation();
}
