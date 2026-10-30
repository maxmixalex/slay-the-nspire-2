// Prepare the entered room's real controls and current cards before revealing
// it. This path visits fitted text on a zero-sized surface, never a hidden
// combat render: no hitboxes, discoveries, holder tweens or label clocks run.
static void plan_combat_frontend(App&a){
    auto&p=a.combat_preparation;const auto&s=a.game;
    const auto add=[&](int image){
        if(image<0||a.content.image_cached(image))return;
        for(unsigned i=0;i<p.ui_count;++i)if(a.content.image_alias(p.ui_images[i],image))return;
        if(p.ui_count<CombatPreparation::UiCapacity)p.ui_images[p.ui_count++]=uint16_t(image);
        else p.bounded=true;
    };
    const auto ui=[&](int id){add(a.content.ui_image(id));};
    ui(U_TopBar);ui(U_TopBarBackdrop);ui(U_TopFloor);ui(U_TopMap);ui(U_TopDeck);ui(U_TopSettings);
    int portrait=a.content.ui_image(U_TopBarIronclad+int(s.character));
    add(portrait>=0?portrait:a.content.ui_image(character_portrait(s.character)));
    if(world_ascension(s))ui(U_TopBarAscension);
    add(A_Heart);add(A_Coin);ui(U_PotionPlaceholder);if(a.run_timer_visible())ui(I9_RunTimerIcon);
    const int first=int(a.potion_page)*hud_potion_layout::Visible;
    for(int i=first;i<mn(int(potion_slot_count(s)),first+hud_potion_layout::Visible);++i)
        if(potion_at(s,i)!=Potion::Empty)add(a.content.potion_image(potion_at(s,i)));
    const auto room=hud_room_info(s);
    const auto room_icon=[&](const char*key){if(const auto*icon=key?hud_ui7_icon(key):nullptr){ui(icon->icon);ui(icon->outline);}};
    if(room.room_enabled)room_icon(room.room_key);
    if(room.boss_enabled){if(const auto*d=encounter_definition(room.boss))room_icon(d->key);
        if(const auto*d=encounter_definition(room.second_boss))room_icon(d->key);}
    for(unsigned i=0;i<ui_owned_relic_count(s);++i){
        if(hud_relic_layout::y(i)>=ScreenH)break;
        const RelicId id=ui_owned_relic_id(s,i);add(a.relic_face_image(id,int(s.character)));
        if(id<WorldRelicKinds)ui(id==WorldRelicYummyCookie?collection_assets::CookieOutline[unsigned(s.character)<5?unsigned(s.character):0]:collection_assets::RelicOutline[id]);
    }
    static const char*const health_assets[]={"HEALTH_BAR_BG","HEALTH_BAR_MASK","HEALTH_BAR_FILL"};
    for(const char*key:health_assets)
        add(a.content.world_image("common",key));
    for(int actor=0;actor<=s.enemy_count;++actor){
        const auto health=a.combat_health(actor);
        if(health.block>0){add(a.content.world_image("common","HEALTH_BAR_STROKE"));add(a.content.world_image("common","HEALTH_BAR_BLOCK"));}
        if(health.infinite&&health.hp>0)add(a.content.world_image("common","HEALTH_BAR_INFINITY"));
        uint16_t tokens[MaxPlayerPowerEntries];const unsigned count=a.combat_power_entries(actor,tokens,MaxPlayerPowerEntries);
        for(unsigned i=0;i<count;++i)if(CombatStatus::PowerFlags[power_id(tokens[i])]&1)add(ui_status_image(a,int(power_id(tokens[i]))));
        if(actor&&s.enemy_world.active){const auto intent=a.combat_intent(actor-1);
            for(const auto&part:ui_intent_parts)if(intent.flags&part.flag)add(ui_intent_image(a,part,intent));}
    }
    if(!s.enemy_world.active){add(A_AttackIntent);add(A_BuffIntent);}
    const int energy=U_EnergyLayers+int(s.character)*5;
    if(a.content.ui_image(energy)>=0)for(int i=0;i<(s.character==Character::Necrobinder?3:5);++i)ui(energy+i);
    else add(character_energy_image(a.content,s.character));
    if(s.character==Character::Regent||a.combat_status_amount(0,44)){
        if(a.content.version()>=FrontendContentVersion)for(int i=0;i<3;++i)ui(F_StarCounterBase+i);else ui(U_StarCounter);
    }
    for(int i=0;i<a.combat_status_fact(CombatStatus::OrbSlots);++i)
        ui(i<a.combat_status_fact(CombatStatus::OrbCount)?orb_image(a.combat_status_fact(CombatStatus::OrbKind,i)):I8_EmptyOrb);
    const int end=a.content.ui_image(U_EndTurnNative);add(end>=0?end:A_EndTurn);ui(U_EndTurnHover);ui(U_EndTurnGlow);
    const auto piles=a.combat_pile_counts();add(A_DrawPile);add(A_DiscardPile);if(piles.exhaust)add(A_ExhaustPile);ui(I12_PileCount);
    const auto hand=a.combat_hand();
    for(unsigned i=0;i<hand.count;++i){
        const int instance=hand.ids[i];const Card card=s.cards[instance];const auto*facts=event_card_facts(s,card,instance,-1);
        const auto type=a.card_face_type(card,instance,-1,facts);const unsigned group=card_face::material_group(unsigned(card_rarity(card.kind)));
        const int base=U_CardBorderCommonAttack+int(group)*5;
        add(a.card_face_image(card,instance,-1,facts));add(a.card_face_frame(card,instance,-1,facts));
        ui(base+(type==CardType::Attack?0:type==CardType::Power?2:1));ui(base+3);ui(base+4);add(a.content.energy_image(card));
        if(card_star_cost(s,card,instance))ui(U_Star);
        if(card_face_resource_cross(a,card,cost(s,instance),card_star_cost(s,card,instance),instance))ui(I9_CardUnplayable);
        if(const unsigned enchantment=card_face_enchantment_key(a,facts,instance)){
            ui(I13_CardEnchantment);const unsigned kind=enchantment&255;
            add(a.content.world_image("enchantments",kind==EventEnchantmentKinds?"INKY":event_enchantment_key(EventEnchantment(kind))));
        }
        if(card_rarity(card.kind)==5){ui(type==CardType::Attack?I14_AncientTextAttack:type==CardType::Power?I14_AncientTextPower:I14_AncientTextSkill);
            ui(I14_AncientBanner);for(unsigned j=0;j<I14_AncientFlameCount;++j)ui(I14_AncientFlame+j);
        }else if(const unsigned overlay=card_face_overlay(a,card,instance)){
            const unsigned count=overlay==8?(a.phobia_mode?1:I14_InfectionCount):I14_AfflictionFrames;
            for(unsigned j=0;j<count;++j)ui(overlay==8?I14_Infection+j:I14_Afflictions+(overlay-1)*I14_AfflictionFrames+j);
        }
    }
    if(hand.count&&a.content.has_selection_assets())ui(I9_CardHighlightSdf);
    for(unsigned i=1;i<p.ui_count;++i){const auto id=p.ui_images[i];unsigned j=i;
        while(j&&p.ui_images[j-1]>id){p.ui_images[j]=p.ui_images[j-1];--j;}p.ui_images[j]=id;}
    p.ui_stage=1;
}
static void prepare_combat_labels(Canvas&c){
    auto&a=c.a;const auto&s=a.game;
    const auto number=[&](int value,int size,unsigned face=0){char text[24]{};TextBuffer b{text,sizeof text,0};b.number(value);
        Canvas label=c;label.font_face=uint8_t(face);label.text(text,0,0,size);};
    const unsigned face=source_label_face(a.content.original_label_fonts(),5);
    char text[64]{};TextBuffer b{text,sizeof text,0};
    const auto health=a.combat_health(0);b.number(health.hp);b.ch('/');b.number(health.max_hp);c.text_fitted(text,0,0,40,10,White);
    text[0]=0;b.n=0;b.number(a.reward_gold());c.text_fitted(text,0,0,24,10,White);
    text[0]=0;b.n=0;b.number(s.floor);const auto room=hud_room_geometry(s);c.text_fitted(text,0,0,mn(12,room.boss-room.floor-11),10,White,6);
    number(s.deck_count,8);if(world_ascension(s))number(int(world_ascension(s)),8);
    const bool pet=s.character==Character::Necrobinder||s.expansion.osty_max_hp>0;
    for(int actor=0;actor<=s.enemy_count+(pet?1:0);++actor){
        const int slot=actor>s.enemy_count?MaxEnemies+1:actor;const auto value=a.combat_health(slot);
        const auto pose=slot==MaxEnemies+1?a.combat_player_pose(true):slot?a.combat_enemy_pose(slot-1):a.combat_player_pose();
        text[0]=0;b.n=0;
        if(value.hp<=0){const char*dead=a.content.world_string("gameplay_ui","HEALTH_BAR.DEAD");b.text(*dead?dead:"已击败");}
        else {b.number(value.hp);b.ch('/');b.number(value.max_hp);}
        const int size=c.width(text,10)<=pose.width-2?10:8;
        if(value.hp>0&&c.width(text,size)>pose.width-2){text[0]=0;b.n=0;health_amount(b,value.hp);b.ch('/');health_amount(b,value.max_hp);}
        c.text(text,0,0,size);
        if(value.block>0){text[0]=0;b.n=0;b.number(value.block);if(c.width(text,8)>16){text[0]=0;b.n=0;health_amount(b,value.block);}c.text(text,0,0,8);}
        if(slot<=s.enemy_count){uint16_t tokens[MaxPlayerPowerEntries];const unsigned count=a.combat_power_entries(slot,tokens,MaxPlayerPowerEntries);
            for(unsigned i=0;i<count;++i)if((CombatStatus::PowerFlags[power_id(tokens[i])]&1)&&a.combat_power_counter(tokens[i]))number(a.combat_power_amount(tokens[i],true),8);}
        if(slot>0&&slot<=s.enemy_count&&s.enemy_world.active){const auto intent=a.combat_intent(slot-1);
            for(const auto&part:ui_intent_parts)if(intent.flags&part.flag){const auto label=ui_intent_value(a,part,intent);Canvas intent_label=c;intent_label.font_face=uint8_t(ui_intent_face(a));
                intent_label.text(label.value,0,0,ui_intent_size(a));intent_label.text(label.repeat,0,0,ui_intent_size(a,true));}}
    }
    text[0]=0;b.n=0;b.number(a.combat_energy());b.ch('/');b.number(a.combat_energy(true));
    Canvas resource=c;resource.font_face=uint8_t(face);resource.text(text,0,0,12);resource.centered("结束回合",0,0,combat_controls::EndW-2,10);
    const auto piles=a.combat_pile_counts();number(piles.draw,8,face);number(piles.discard,8,face);if(piles.exhaust)number(piles.exhaust,10,face);
    if(s.character==Character::Regent||a.combat_status_amount(0,44))number(a.combat_status_amount(0,44),12,face);
    for(int i=0;i<a.combat_status_fact(CombatStatus::OrbCount);++i){number(a.combat_status_fact(CombatStatus::OrbPassive,i),8);
        if(a.combat_status_fact(CombatStatus::OrbKind,i)==unsigned(DOrb::Dark))number(a.combat_status_fact(CombatStatus::OrbEvoke,i),8);}
    for(unsigned i=0;i<ui_owned_relic_count(s);++i){int value=0;const RelicId id=ui_owned_relic_id(s,i);
        bool counter=s.world_relics.active?world_relic_display(s,i,value):false;
        if(!s.world_relics.active&&id<RelicKinds){const auto legacy=relic_definitions[id].id;counter=relic_counter_limit(legacy)&&(!relic_counter_is_turn_scoped(legacy)||combat_phase(s.phase));value=int(relic_counter(s,legacy));}
        if(counter)number(value,8);}
    if(a.show_card_indices){const auto hand=a.combat_hand();for(unsigned i=0;i<hand.count;++i)number(int(i+1),10);}
}
static PreparationResult prepare_combat_frontend(App&a,GlyphPreparationBudget&budget){
    auto&p=a.combat_preparation;
    if(p.ui_stage==0){plan_combat_frontend(a);return PreparationResult::Pending;}
    if(p.ui_stage==1){
        while(p.ui_cursor<p.ui_count&&a.content.image_cached(p.ui_images[p.ui_cursor]))++p.ui_cursor;
        if(p.ui_cursor<p.ui_count){const auto result=a.content.preload_image_step(p.ui_images[p.ui_cursor]);
            if(result==PreparationResult::Ready){++p.ui_cursor;++p.cold_images;return PreparationResult::Pending;}return result;}
        p.ui_stage=2;return PreparationResult::Pending;
    }
    Canvas hidden{a};hidden.clip_w=hidden.clip_h=0;hidden.glyph_preparation=&budget;
    if(p.ui_stage==2){
        const auto hand=a.combat_hand();if(p.card>=hand.count){p.ui_stage=3;return PreparationResult::Pending;}
        const int instance=hand.ids[p.card];const Card card=a.game.cards[instance];const auto*facts=event_card_facts(a.game,card,instance,-1);
        char description[512]{};a.card_description(card,a.card_preview_target(instance),description,sizeof description,instance,-1,facts);
        if(!p.text_ready){a.content.card_string(card.kind);card_type_label(a.card_face_type(card,instance,-1,facts),&a.content);p.text_ready=true;return PreparationResult::Pending;}
        render_card_face(hidden,card,HandFaceX,HandFaceY,HandFaceW,HandFaceH,cost(a.game,instance),card_star_cost(a.game,card,instance),instance,-1,facts,description);
        if(!budget.pending){++p.card;p.text_ready=false;}
        return a.content.healthy()?PreparationResult::Pending:PreparationResult::Failed;
    }
    if(p.ui_stage==3){prepare_combat_labels(hidden);if(!a.content.healthy())return PreparationResult::Failed;
        if(budget.pending)return PreparationResult::Pending;p.ui_stage=4;}
    return PreparationResult::Ready;
}
