// A new power icon can invalidate the already prepared battle resource set.
// Finish that explicit preparation phase before querying the normal hit list.
static void prepare_power_hover_scene(){
    snapshot();a.reset_presentation();const unsigned animation=a.animation;a.render();unsigned steps=0;
    while(a.combat_preparation.active&&steps++<1024){a.advance(33);a.render();CHECK(a.animation==animation);}
    CHECK(!a.combat_preparation.active&&a.content.healthy());unchanged();
}
static void power_hover_catalog_and_input(){
    setup();a.game.pain=2;a.game.infinite_blades=1;a.game.enemies[0].weak=2;
    power_order_set(a.game,0,7,true);power_order_set(a.game,0,14,true);power_order_set(a.game,1,4,true);
    prepare_power_hover_scene();
    auto collect=[&](uint16_t token){HoverTipList list{};collect_power_hover_tips(a,token,list);CHECK(!list.overflow);return list;};
    auto list=collect(power_token(0,14));CHECK(list.count==2&&list.items[0].smart&&list.items[0].instanced);
    CHECK(list.items[1].kind==HoverTipKind::Card&&list.items[1].card.kind==Shiv);
    list=collect(power_token(0,7));CHECK(list.count==2&&list.find(HoverTipKind::Static,"BLOCK")==1);
    list=collect(power_token(1,4));CHECK(list.count==1&&list.items[0].debuff&&list.items[0].actor==1);
    list=collect(power_token(1,FiveCharacterExtraStatusStart));
    CHECK(list.find(HoverTipKind::Card,"SOVEREIGN_BLADE")>=0&&list.find(HoverTipKind::Keyword,"RETAIN")>=0);
    for(const unsigned id:{44u,45u,46u,113u}){list=collect(power_token(0,id));CHECK(!list.count);}
    for(const auto token:{power_token(0,14),power_token(1,4)}){
        const unsigned actor=power_actor(token),id=power_id(token);const auto region=hit(HitKind::BattleStatus,int(actor)*StatusKinds+int(id));
        a.pointer_move(region.x+region.w/2,region.y+region.h/2);a.render();CardHoverTarget owner{};
        CHECK(power_hover_target(a,owner)&&owner.power==token);
        const auto pose=actor?a.combat_enemy_pose(int(actor)-1):a.combat_player_pose();
        CHECK(owner.owner.x==pose.body_left&&owner.owner.y==pose.body_top&&owner.owner.w==pose.body_width&&owner.owner.h==pose.body_height);
        CHECK(owner.power_left==(pose.x>=160));list=collect(token);
        const auto pages=card_hover_page(a,owner,list);CHECK(pages.count&&pages.pages>0);
        if(!actor){
            CHECK(pages.count==2);const auto&text=pages.pieces[0].box;const auto&card=pages.pieces[1].box;
            if(owner.owner.x<50)CHECK(card.x==owner.owner.x+owner.owner.w+1&&text.x==card.x+card.w+1);
            // Mirror the real owner to the right screen edge. The source's
            // overflow correction moves text left and keeps the card adjacent.
            CardHoverTarget right=owner;right.owner={285,owner.owner.y,32,owner.owner.h};right.power_left=true;
            const auto mirrored=card_hover_page(a,right,list);CHECK(mirrored.count==2);
            const auto&text_right=mirrored.pieces[0].box;const auto&card_right=mirrored.pieces[1].box;
            CHECK(text_right.x+text_right.w+1==card_right.x&&card_right.x+card_right.w<=317);
        }
        int visited[MaxCardHoverTips]{};
        for(int n=0;n<pages.pages;++n){const auto page=card_hover_page(a,owner,list,n);
            for(unsigned i=0;i<page.count;++i){const auto&p=page.pieces[i];const auto&r=p.box;
                CHECK(p.tip<list.count&&p.skip==visited[p.tip]);
                if(list.items[p.tip].kind==HoverTipKind::Card){CHECK(r.w==46&&r.h==62);++visited[p.tip];}
                else{CHECK(r.w==104&&p.lines>0);visited[p.tip]+=p.lines;}
                CHECK(r.x>=3&&r.y>=24&&r.x+r.w<=317&&r.y+r.h<=218);
                for(unsigned j=0;j<i;++j){const auto&o=page.pieces[j].box;CHECK(r.x+r.w<=o.x||o.x+o.w<=r.x||r.y+r.h<=o.y||o.y+o.h<=r.y);}
            }}
        for(unsigned i=0;i<list.count;++i){char title[160]{},body[2048]{};
            format_card_hover_tip(a,owner,list.items[i],title,sizeof title,body,sizeof body);
            CHECK(visited[i]>0&&*title&&*body&&!std::strchr(body,'{'));
        }
        capture(actor?"power-tooltip-original-debuff":"power-tooltip-original-associated-shiv");unchanged();
        a.pointer_button(true);CHECK(!power_hover_target(a,owner));a.pointer_cancel();a.render();
        a.keyboard_card=0;CHECK(!power_hover_target(a,owner));a.keyboard_card=-1;
        a.paused=true;CHECK(!power_hover_target(a,owner));a.paused=false;unchanged();
    }
    // Original independent Panache instances retain their own smart progress;
    // moving between equal Power ids must change ownership and description.
    setup();queue_extension(a.game,XOp::Power,int(XPower::Panache),10);resolve_pending(a.game);
    queue_extension(a.game,XOp::Power,int(XPower::Panache),20);resolve_pending(a.game);
    a.game.expansion.panache_count[0]=1;a.game.expansion.panache_count[1]=3;CHECK(validate(a.game));
    const unsigned id=18+unsigned(XPower::Panache);power_order_set(a.game,0,id,true,0);power_order_set(a.game,0,id,true,1);
    prepare_power_hover_scene();uint32_t identities[2]{};std::string descriptions[2];
    for(unsigned slot=0;slot<2;++slot){const auto token=power_token(0,id,slot);const auto region=hit(HitKind::PowerInstance,token);
        a.pointer_move(region.x+region.w/2,region.y+region.h/2);a.render();CardHoverTarget owner{};
        CHECK(power_hover_target(a,owner)&&owner.power==token);identities[slot]=owner.identity;list=collect(token);
        char title[160]{},body[2048]{};format_card_hover_tip(a,owner,list.items[0],title,sizeof title,body,sizeof body);
        CHECK(*title&&*body&&!std::strchr(body,'{'));descriptions[slot]=body;unchanged();
    }
    CHECK(identities[0]!=identities[1]);if(actual)CHECK(descriptions[0]!=descriptions[1]);
    capture("power-tooltip-original-independent-instance");setup();
}
