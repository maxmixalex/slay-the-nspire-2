// Included by card_hud_ui_regression: actual production rules and cache paths.
static void original_upgrade_cost_preview(){
    auto saved=std::make_unique<State>(app.game);const bool title=app.title,library=app.library,history=app.history_open;
    const int panel=app.panel,detail=app.detail_card,instance=app.detail_instance,deck=app.detail_deck_index,tab=app.library_tab;
    const auto inspect=app.card_inspect;
    app.title=app.library=app.history_open=false;app.panel=0;new_run(app.game,8138,true,Character::Ironclad);
    app.game.energy=9;Canvas c{app};
    const Card changed{BodySlam,1},unchanged{Strike,1},x_cost{Whirlwind,1};
    CHECK(cost(Card{BodySlam,0})!=cost(changed));
    CHECK(card_face_resource_styles(app,changed,cost(changed),0,-1,nullptr)==0);
    CHECK((card_face_resource_styles(app,changed,cost(changed),0,-1,nullptr,true)&3)==ResourceDecreased);
    CHECK(card_face_resource_styles(app,unchanged,cost(unchanged),0,-1,nullptr,true)==0);
    CHECK(card_face_resource_styles(app,x_cost,-1,0,-1,nullptr,true)==0);
    // The same finalized upgraded card and preview copy have identical text,
    // costs and art. Their green value AND green outline must stay distinct
    // through repeated exact-cache hits, including switching back to normal.
    std::vector<uint16_t> normal,preview;
    for(bool flag:{false,true,false,true}){
        pattern();render_card_face(c,changed,90,75,46,64,-2,-2,-1,-1,nullptr,nullptr,nullptr,true,flag);
        const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
        if(flag)preview=expected;else normal=expected;
        for(unsigned repeat=0;repeat<2;++repeat){pattern();begin_card_face_frame();
            draw_card_face(c,changed,90,75,46,64,-2,-2,-1,-1,nullptr,nullptr,nullptr,true,flag);
            CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));}
    }
    CHECK(std::memcmp(normal.data(),preview.data(),sizeof app.pixels));
    CHECK(lab_card(app.game,changed));const int id=app.game.card_count-1;
    ++app.game.revision;begin_card_face_frame();invalidate_hand_tiles();
    CHECK((hand_tile(c,id).resource_styles&3)==ResourceNormal);
    // NCard's preview branch precedes the resource shortage branch. A
    // finalized upgraded hand card still retains the actual shortage color.
    CHECK(lab_card(app.game,{DarkEmbrace,0}));const int expensive=app.game.card_count-1;
    app.game.energy=0;
    CHECK((card_face_resource_styles(app,{DarkEmbrace,1},cost(Card{DarkEmbrace,1}),0,expensive,nullptr,true)&3)==ResourceDecreased);
    CHECK((card_face_resource_styles(app,{DarkEmbrace,1},cost(Card{DarkEmbrace,1}),0,expensive,nullptr,false)&3)==ResourceInsufficient);
    // Inspectors preserve the original model's upgrade state through the
    // checkbox and close fade; library previews always start from the base.
    app.detail_instance=-1;app.detail_deck_index=0;app.game.deck[0]={BodySlam,0};app.detail_card=card_token(changed);app.bind_card_inspect();
    CHECK(app.card_inspect.upgrade_preview(true)&&!app.card_inspect.upgrade_preview(false));
    app.game.deck[0]=changed;app.bind_card_inspect();CHECK(!app.card_inspect.upgrade_preview(true));
    app.library=true;app.library_tab=0;app.bind_card_inspect();CHECK(app.card_inspect.upgrade_preview(true));
    app.card_inspect.close(app.detail_card,app.map_clock_ms);CHECK(app.card_inspect.upgrade_preview(card_from_token(app.card_inspect.closed_token).upgraded));
    clear();draw_card_face(c,changed,63,64,69,97);draw_card_face(c,changed,190,64,69,97,-2,-2,-1,-1,nullptr,nullptr,nullptr,true,true);
    capture("card-cost-finalized-versus-upgrade-preview");
    app.game=*saved;app.title=title;app.library=library;app.history_open=history;app.panel=panel;
    app.detail_card=detail;app.detail_instance=instance;app.detail_deck_index=deck;app.library_tab=tab;app.card_inspect=inspect;invalidate_hand_tiles();
}
static void original_resource_crosses(){
    original_upgrade_cost_preview();
    auto original=std::make_unique<State>(app.game);
    const bool title=app.title,library=app.library,history=app.history_open;
    app.title=app.library=app.history_open=false;
    new_run(app.game,7316,true,Character::Regent);
    auto add=[&](Card card){CHECK(lab_card(app.game,card));return int(app.game.card_count)-1;};
    const int strike=add({Strike,0}),wound=add({Wound,0}),finale=add({GrandFinale,0});
    auto&s=app.game;s.energy=9;s.expansion.stars=9;Canvas c{app};
    auto cross=[&](int id){return card_face_resource_cross(app,s.cards[id],cost(s,id),card_star_cost(s,s.cards[id],id),id);};
    CHECK(can_play_card(s,strike)&&!card_has_nonresource_play_block(s,strike)&&!cross(strike));
    s.energy=0;CHECK(!can_play_card(s,strike)&&!card_has_nonresource_play_block(s,strike)&&!cross(strike));
    s.energy=9;
    CHECK(!can_play_card(s,wound)&&card_has_nonresource_play_block(s,wound)&&cross(wound));
    // Its negative-energy parent is hidden in the original, so an Unplayable
    // keyword does not invent an energy badge on a Wound.
    CHECK(app.content.energy_image(s.cards[wound])<0);
    CHECK(s.draw.count>0&&!can_play_card(s,finale)&&card_has_nonresource_play_block(s,finale)&&cross(finale));
    int index=-1;for(unsigned i=0;i<s.hand.count;++i)if(s.hand.ids[i]==finale)index=int(i);CHECK(index>=0);
    std::vector<uint8_t> canonical(sizeof(State));std::memcpy(canonical.data(),&s,sizeof s);
    CHECK(!play(s,index,0)&&!std::memcmp(canonical.data(),&s,sizeof s));
    app.library=true;CHECK(!cross(finale));app.library=false;

    // Tangled in the pinned sequel adds cost to Entangled attacks; unlike
    // Smog it is not an independent prohibition. Test both actual rule paths.
    s.enemy_world.active=1;s.enemy_world.affliction[strike]=unsigned(EnemyAffliction::Entangled);
    s.enemy_world.player_power[unsigned(EnemyPower::Tangled)]=2;
    CHECK(cost(s,strike)==3&&!card_has_nonresource_play_block(s,strike)&&can_play_card(s,strike)&&!cross(strike));
    s.energy=2;CHECK(!can_play_card(s,strike)&&!cross(strike));
    s.enemy_world.affliction[strike]=unsigned(EnemyAffliction::Smog);
    s.energy=9;CHECK(card_has_nonresource_play_block(s,strike)&&!can_play_card(s,strike)&&cross(strike));
    s.energy=0;CHECK(card_has_nonresource_play_block(s,strike)&&!cross(strike));
    s.energy=9;s.enemy_world.affliction[strike]=unsigned(EnemyAffliction::None);
    s.effect_count=1;CHECK(!can_play_card(s,strike)&&!card_has_nonresource_play_block(s,strike)&&!cross(strike));
    s.effect_count=0;s.playing=strike;CHECK(!can_play_card(s,strike)&&!cross(strike));s.playing=-1;
    s.enemy_world.active=0;

    // Both cached cards remain unplayable while the action queue is busy.
    // Lifting only Grand Finale's draw-pile condition must remove its cross,
    // although numeric costs, colors, description and playability stay equal.
    s.effect_count=1;++s.revision;invalidate_hand_tiles();begin_card_face_frame();
    std::memcpy(canonical.data(),&s,sizeof s);const auto&blocked=hand_tile(c,finale);
    CHECK(!blocked.playable&&blocked.resource_cross&&!std::memcmp(canonical.data(),&s,sizeof s));
    const std::vector<uint16_t> crossed(blocked.pixels,blocked.pixels+HandTileW*HandTileH);
    const unsigned styles=blocked.resource_styles;
    while(s.draw.count){const unsigned id=s.draw.ids[--s.draw.count];s.discard.ids[s.discard.count++]=uint16_t(id);s.zones[id]=Zone::Discard;}
    ++s.revision;begin_card_face_frame();std::memcpy(canonical.data(),&s,sizeof s);
    const auto&waiting=hand_tile(c,finale);
    CHECK(!waiting.playable&&!waiting.resource_cross&&waiting.resource_styles==styles);
    CHECK(!std::memcmp(canonical.data(),&s,sizeof s));
    const int image=app.content.version()>=CardResourceContentVersion?app.content.ui_image(I9_CardUnplayable):-1;
    if(image>=0)CHECK(std::memcmp(crossed.data(),waiting.pixels,sizeof waiting.pixels));
    s.effect_count=0;CHECK(can_play_card(s,finale)&&!cross(finale));

    const int stars=add({RegentCrescentSpear,0});s.enemy_world.active=1;
    s.enemy_world.affliction[stars]=unsigned(EnemyAffliction::Smog);
    CHECK(card_star_cost(s,s.cards[stars],stars)>0&&cross(stars));
    s.expansion.stars=0;CHECK(card_has_nonresource_play_block(s,stars)&&!cross(stars));
    s.expansion.stars=9;++s.revision;
    if(image>=0){
        CHECK(app.content.image(image).w==16&&app.content.image(image).h==16);
        // Original cross pixels in both projected slots, including the
        // independent small-card cache used by hand-card/pile previews.
        pattern();draw_card_face(c,s.cards[stars],90,75,46,64,cost(s,stars),card_star_cost(s,s.cards[stars],stars),stars);
        const std::vector<uint16_t> expected(app.pixels,app.pixels+320*240);
        for(unsigned i=0;i<2;++i){pattern();begin_card_face_frame();draw_card_face(c,s.cards[stars],90,75,46,64,cost(s,stars),card_star_cost(s,s.cards[stars],stars),stars);
            CHECK(!std::memcmp(expected.data(),app.pixels,sizeof app.pixels));}
        clear();draw_card_face(c,s.cards[stars],135,70,69,97,cost(s,stars),card_star_cost(s,s.cards[stars],stars),stars);
        capture("card-resource-cross-both");
    }
    app.game=*original;app.title=title;app.library=library;app.history_open=history;invalidate_hand_tiles();
}
