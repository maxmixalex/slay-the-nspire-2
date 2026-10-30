static void power_hover_card(const App&a,HoverTipList&out,Card card,int instance=-1,const EventCardFacts*facts=nullptr,bool extras=false){
    if(card.kind>=CardKinds)return;HoverTip tip{};tip.kind=HoverTipKind::Card;tip.key=definitions[card.kind].key;
    tip.card=card;tip.smart=true;tip.amount=int16_t(instance+1);if(facts){tip.has_facts=true;tip.facts=*facts;}out.add(tip);
    if(extras){const EventCardFacts canonical{};hover_collect(a,card,out,-1,-1,facts?facts:&canonical,1);}
}
static void power_hover_origin(const App&a,unsigned id,HoverTipList&out){
    CardKind card=CardKinds;
    if(id==CharacterExtraStatusStart+4)card=SetupStrike;
    else if(id==DefectStatusStart+unsigned(DPower::FocusedStrikeFocus))card=DefectFocusedStrike;
    else if(id==DefectStatusStart+unsigned(DPower::HotfixFocus))card=DefectHotfix;
    else if(id==DefectStatusStart+unsigned(DPower::SynchronizeFocus))card=DefectSynchronize;
    if(card>=CardKinds)return;
    // These existing independent status channels retain their exact origin
    // model. Do not invent an upgrade when mixed copies make it unknowable.
    int upgraded=-1;for(unsigned i=0;i<a.game.card_count;++i)if(a.game.cards[i].kind==card){
        if(upgraded>=0&&upgraded!=a.game.cards[i].upgraded)return;upgraded=a.game.cards[i].upgraded;}
    if(upgraded>=0)power_hover_card(a,out,{card,uint8_t(upgraded)});
}
void collect_power_hover_tips(const App&a,uint16_t token,HoverTipList&out){
    out.clear();const unsigned id=power_id(token),actor=power_actor(token);
    if(id>=StatusKinds||actor>a.game.enemy_count||!(CombatStatus::PowerFlags[id]&1))return;
    const char*key=ui_status_key(int(id));if(!key)return;
    HoverTip main{};main.kind=HoverTipKind::Power;main.key=key;main.actor=int16_t(actor);
    main.amount=int16_t(a.combat_power_amount(token));main.smart=true;main.instanced=true;
    main.power=int(token);
    main.debuff=(CombatStatus::PowerFlags[id]&2)!=0;out.add(main);
    const PowerHoverCatalog*row=nullptr;for(const auto&entry:power_hover_catalog)if(hover_key_equal(entry.key,key)){row=&entry;break;}
    if(!row)return;
    for(unsigned i=0;i<row->count;++i){const auto&spec=power_hover_specs[row->first+i];
        if(spec.kind==HoverTipKind::Special){
            if(hover_key_equal(spec.key,"POWER_ORIGIN"))power_hover_origin(a,id,out);
            else if(hover_key_equal(spec.key,"POWER_CAPTURED_CARD")&&actor){
                const int value=a.combat_status_fact(CombatStatus::CapturedCard,int(actor)-1);
                if(value>=0){const auto&m=a.game.enemy_world.models[actor-1];
                    power_hover_card(a,out,card_from_token(value),m.captured_card?int(m.captured_card)-1:-1,&m.captured_deck.facts);}
            }else if(hover_key_equal(spec.key,"POWER_WITHER")){
                int instance=-1;const int level=actor&&a.game.enemy_world.models[actor-1].kind==EnemyId::Aeonglass?a.game.enemy_world.models[actor-1].data[1]:0;
                for(unsigned index=0;index<a.game.card_count;++index)if(a.game.cards[index].kind==EventCardWither&&a.game.enemy_world.card_value[index]==level){instance=int(index);break;}
                const EventCardFacts canonical{};power_hover_card(a,out,{EventCardWither,0},instance,&canonical,true);
            }
        }else if(spec.kind==HoverTipKind::Card){
            power_hover_card(a,out,{world_card_id(spec.key),0},-1,nullptr,(spec.flags&2)!=0);
        }else if(spec.kind==HoverTipKind::Affliction){
            hover_add(out,spec.kind,spec.key,spec.flags&4?main.amount:0);
            if(hover_key_equal(spec.key,"HEXED"))hover_word(out,"ETHEREAL");
            if(hover_key_equal(spec.key,"TAINTED"))hover_add(out,HoverTipKind::Power,"TAINTED_POWER");
        }else hover_add(out,spec.kind,spec.key);
    }
    out.finish();
}
