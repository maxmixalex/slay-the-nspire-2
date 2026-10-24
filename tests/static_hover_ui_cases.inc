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
    // Settings supplies titles as well as descriptions. Exercise scrolled
    // rows and the independent input tab using their real localized labels.
    setup(Character::Ironclad);app.title=true;app.settings_open=true;app.settings_tab=0;
    app.settings_top=0;app.settings_item=1;app.pointer.x=150;app.pointer.y=95;
    CHECK(save(app.game,before));clear();frontend_settings_screen(c);capture("settings-source-hover-shake");
    CHECK(*app.content.world_string("settings_ui","SCREENSHAKE_HEADER"));
    app.settings_top=2;app.settings_item=6;app.pointer.y=179;clear();frontend_settings_screen(c);capture("settings-source-hover-text-effects");
    app.settings_tab=1;app.settings_top=0;app.settings_item=0;app.pointer.y=70;
    clear();frontend_settings_screen(c);capture("settings-source-hover-keyboard");
    CHECK(save(app.game,after)&&before.size==after.size&&!std::memcmp(before.bytes,after.bytes,before.size));
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
