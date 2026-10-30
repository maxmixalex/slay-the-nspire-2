CombatSelectionKind App::combat_selection_kind()const{
    switch(game.phase){
    case Phase::ExhaustChoice:case Phase::ExhaustManyChoice:case Phase::DiscardChoice:return CombatSelectionKind::Hand;
    case Phase::ReturnChoice:return CombatSelectionKind::Pile;
    case Phase::WorldRelicChoice:
        if(game.world_relics.choice==WorldRelicChoice::Hand)return CombatSelectionKind::Hand;
        if(game.world_relics.choice==WorldRelicChoice::CombatOffer||
           (game.world_relics.choice==WorldRelicChoice::OfferCard&&!game.world_relics.choice_reward)){
            for(unsigned i=0;i<game.world_relics.count;++i){const auto&r=game.world_relics.entries[i];
                if(r.serial!=game.world_relics.choice_owner)continue;
                if(r.id==WorldRelicChoicesParadox)return CombatSelectionKind::SimpleGrid;
                if(r.id==WorldRelicToolbox||r.id==WorldRelicHeftyTablet||r.id==WorldRelicLeadPaperweight)return CombatSelectionKind::Offer;}
        }return CombatSelectionKind::None;
    case Phase::ExtendedChoice:break;
    default:return CombatSelectionKind::None;
    }
    switch(game.expansion.choice){
    case XChoice::Offer:case XChoice::EventAbundance:case XChoice::EnemyKnowledge:return CombatSelectionKind::Offer;
    case XChoice::Draw:case XChoice::Discard:case XChoice::RTransformDraw:case XChoice::RDiscardTop:
    case XChoice::RForegone:case XChoice::NCleanse:case XChoice::NDredge:case XChoice::NSeance:
    case XChoice::EventNeowsFury:case XChoice::EventWish:return CombatSelectionKind::Pile;
    case XChoice::IUpgrade:return CombatSelectionKind::HandUpgrade;
    default:return CombatSelectionKind::Hand;
    }
}
CombatSelectionPrompt App::combat_selection_prompt()const{
    if(combat_selection_kind()==CombatSelectionKind::Offer)return {"gameplay_ui","CHOOSE_CARD_HEADER"};
    if(game.phase==Phase::WorldRelicChoice){
        if(combat_selection_kind()==CombatSelectionKind::SimpleGrid)return {"relics","CHOICES_PARADOX.selectionScreenPrompt"};
        for(unsigned i=0;i<game.world_relics.count;++i){const auto&r=game.world_relics.entries[i];
            if(r.serial==game.world_relics.choice_owner&&r.id==WorldRelicGamblingChip)return {"relics","GAMBLING_CHIP.selectionScreenPrompt"};}
        return {"card_selection","TO_EXHAUST"};
    }
    if(game.phase==Phase::DiscardChoice)return {"card_selection","TO_DISCARD"};
    if(game.phase==Phase::ExhaustManyChoice)return {"cards","PURITY.selectionScreenPrompt"};
    if(game.phase==Phase::ReturnChoice)return {"cards","HEADBUTT.selectionScreenPrompt"};
    if(game.phase!=Phase::ExtendedChoice)return {"card_selection","TO_EXHAUST"};
    const CardKind played=game.playing>=0&&game.playing<game.card_count?game.cards[game.playing].kind:CardKinds;
    switch(game.expansion.choice){
    case XChoice::Top:return {"cards",played==RegentGlimmer?"GLIMMER.selectionScreenPrompt":played==RegentPhotonCut?"PHOTON_CUT.selectionScreenPrompt":"THINKING_AHEAD.selectionScreenPrompt"};
    case XChoice::Draw:
        if(game.expansion.potion_active&&game.playing<0)return {"potions","DROPLET_OF_PRECOGNITION.selectionScreenPrompt"};
        if(played==SecretTechnique)return {"cards","SECRET_TECHNIQUE.selectionScreenPrompt"};
        if(played==SecretWeapon)return {"cards","SECRET_WEAPON.selectionScreenPrompt"};
        if(played==SeekerStrike)return {"cards","SEEKER_STRIKE.selectionScreenPrompt"};
        return {"powers","STRATAGEM_POWER.selectionScreenPrompt"};
    case XChoice::Discard:return game.expansion.potion_active&&game.playing<0?CombatSelectionPrompt{"potions","LIQUID_MEMORIES.selectionScreenPrompt"}:CombatSelectionPrompt{"cards","HOLOGRAM.selectionScreenPrompt"};
    case XChoice::DiscardRedraw:return {"potions","GAMBLERS_BREW.selectionScreenPrompt"};
    case XChoice::Exhaust:return {"potions","ASHWATER.selectionScreenPrompt"};
    case XChoice::Free:return {"potions","TOUCH_OF_INSANITY.selectionScreenPrompt"};
    case XChoice::IUpgrade:return {"gameplay_ui","CHOOSE_CARD_UPGRADE_HEADER"};
    case XChoice::SlySkill:return {"cards","HAND_TRICK.selectionScreenPrompt"};
    case XChoice::NightmareCard:return {"cards","NIGHTMARE.selectionScreenPrompt"};
    case XChoice::RTransformGuards:return {"cards","GUARDS.selectionScreenPrompt"};
    case XChoice::RDecisions:return {"cards","DECISIONS_DECISIONS.selectionScreenPrompt"};
    case XChoice::RHeirloom:return {"cards","HEIRLOOM_HAMMER.selectionScreenPrompt"};
    case XChoice::RDiscardTop:return {"cards","COSMIC_INDIFFERENCE.selectionScreenPrompt"};
    case XChoice::RForegone:return {"powers","FOREGONE_CONCLUSION_POWER.selectionScreenPrompt"};
    case XChoice::RTyranny:case XChoice::NCleanse:return {"card_selection","TO_EXHAUST"};
    case XChoice::NDredge:return {"cards",played==NecroGraveblast?"GRAVEBLAST.selectionScreenPrompt":"DREDGE.selectionScreenPrompt"};
    case XChoice::NEthereal:return {"cards","SCULPTING_STRIKE.selectionScreenPrompt"};
    case XChoice::NRetain:return {"cards","SNAP.selectionScreenPrompt"};
    case XChoice::NTransfigure:return {"cards","TRANSFIGURE.selectionScreenPrompt"};
    case XChoice::EventDualWield:return {"cards","DUAL_WIELD.selectionScreenPrompt"};
    case XChoice::EventNeowsFury:return {"cards","NEOWS_FURY.selectionScreenPrompt"};
    case XChoice::EventWish:return {"cards","WISH.selectionScreenPrompt"};
    default:return {"card_selection","TO_TRANSFORM"};
    }
}
unsigned App::combat_selection_items(uint16_t*out,unsigned capacity)const{
    if(combat_selection_kind()==CombatSelectionKind::None)return 0;
    if(game.phase==Phase::WorldRelicChoice)return world_choice_options(out,capacity);
    const auto&pile=choice_pile(game);for(unsigned i=0;out&&i<pile.count&&i<capacity;++i)out[i]=pile.ids[i];return pile.count;
}
unsigned App::combat_selection_min()const{
    if(game.phase==Phase::WorldRelicChoice)return game.world_relics.choice==WorldRelicChoice::CombatOffer?1:world_choice_min();
    return game.phase==Phase::ExtendedChoice&&game.expansion.choice==XChoice::Offer?0:choice_min(game);
}
unsigned App::combat_selection_max()const{return game.phase==Phase::WorldRelicChoice?world_choice_max():choice_max(game);}
bool App::combat_selection_manual()const{
    const auto kind=combat_selection_kind();
    return combat_selection_hand(kind)||(kind==CombatSelectionKind::Pile&&game.phase==Phase::ExtendedChoice&&game.expansion.choice==XChoice::EventNeowsFury);
}
bool App::combat_selection_interactive()const{
    return combat_selection_kind()!=CombatSelectionKind::None&&!title&&!paused&&!help&&!library&&!panel&&!settings_open&&!frontend_confirm&&
        !map_window.is_open&&!character_select&&detail_card<0&&detail_item<0&&status_actor<0&&!event_reader&&!choice_peeking&&!ftue.active();
}
static void combat_selection_confirm_motion(App&a){
    const auto opened=a.combat_selection.row.opened;
    a.native_confirm_motion.observe(NativeConfirmPresentation::identity(opened,a.game.seed,unsigned(UiAction::ConfirmChoice),unsigned(a.game.phase)),a.combat_selection_confirmable(),opened,a.map_clock_ms);
}
void App::sync_combat_selection(){
    const auto kind=combat_selection_kind();
    if(kind==CombatSelectionKind::None||!combat_choice_ready()){combat_selection.row.active=false;return;}
    const uint32_t key=game.seed^game.revision*0x9e3779b9u^unsigned(game.phase)*0x85ebca6bu;
    if(combat_selection.observe(kind,key,combat_selection_items(),map_clock_ms))cursor=0;
    if(kind!=CombatSelectionKind::Offer)combat_selection_confirm_motion(*this);
}
bool App::combat_selection_ready(){
    if(!combat_choice_ready())return false;sync_combat_selection();
    return combat_selection.kind!=CombatSelectionKind::Offer||combat_selection.offer_clickable(map_clock_ms);
}
bool App::combat_selection_confirmable()const{
    return combat_selection_manual()&&choice_selected_count>=combat_selection_min()&&choice_selected_count<=combat_selection_max();
}
void App::skip_combat_selection(){
    if(!combat_selection_interactive()||!combat_choice_ready()||combat_selection_kind()!=CombatSelectionKind::Offer||combat_selection_min())return;
    choice_selected_count=0;confirm_choice();
}
void App::toggle_choice(int id){
    if(choice_peeking||!combat_choice_ready()||combat_selection_kind()==CombatSelectionKind::None)return;
    sync_selection();if(id<0||!combat_selection_ready())return;
    uint16_t ids[MaxWorldUiChoices];const unsigned count=combat_selection_items(ids,MaxWorldUiChoices);
    bool found=false;for(unsigned i=0;i<count&&i<MaxWorldUiChoices;++i)if(ids[i]==id)found=true;if(!found)return;
    const auto kind=combat_selection_kind();
    for(unsigned i=0;i<choice_selected_count;++i)if(choice_selected[i]==id){
        for(unsigned j=i+1;j<choice_selected_count;++j)choice_selected[j-1]=choice_selected[j];--choice_selected_count;
        combat_selection.select_count(choice_selected_count,map_clock_ms);combat_selection_confirm_motion(*this);return;
    }
    const unsigned maximum=combat_selection_max();
    // NPlayerHand replaces the last selected holder at capacity. The pile
    // grid instead ignores extra clicks while its manual Confirm is enabled.
    if(combat_selection_hand(kind)&&maximum&&choice_selected_count>=maximum)--choice_selected_count;
    if(choice_selected_count<maximum&&choice_selected_count<MaxDeck)choice_selected[choice_selected_count++]=uint16_t(id);
    combat_selection.select_count(choice_selected_count,map_clock_ms);
    if(kind!=CombatSelectionKind::Offer)combat_selection_confirm_motion(*this);
    if(kind==CombatSelectionKind::Offer||(!combat_selection_manual()&&choice_selected_count>0&&choice_selected_count>=(maximum<count?maximum:count)))confirm_choice();
}
void App::confirm_choice(){
    if(choice_peeking||!combat_choice_ready()||combat_selection_kind()==CombatSelectionKind::None)return;
    sync_selection();
    const unsigned minimum=combat_selection_min(),maximum=combat_selection_max();
    if(choice_selected_count<minimum||choice_selected_count>maximum)return;
    if(combat_selection_kind()==CombatSelectionKind::Offer&&choice_selected_count&&!combat_selection_ready())return;
    // Numeric shortcuts also enter here directly. Keep their continuation in
    // the same scoped observer as pointer and ordinary keyboard selection.
    UiCoreObserver observer(*this);continue_combat_choice();prepare_combat_selection_motion();bool done=false;
    if(game.phase==Phase::WorldRelicChoice)done=world_relic_select(game,choice_selected,choice_selected_count);
    else if(game.phase==Phase::ExtendedChoice)done=choose_extended(game,choice_selected,choice_selected_count);
    else if(game.phase==Phase::ReturnChoice)done=choose_return_to_draw(game,choice_selected[0]);
    else if(game.phase==Phase::ExhaustManyChoice)done=choose_exhaust_many(game,choice_selected,choice_selected_count);
    else if(game.phase==Phase::DiscardChoice)done=choose_discard(game,choice_selected,maximum);
    else for(int i=0;i<game.hand.count;++i)if(game.hand.ids[i]==choice_selected[0]){done=choose_exhaust(game,i);break;}
    if(done){choice_selected_count=0;choice_revision=game.revision;cursor=0;busy=0;pointer_cancel();combat_selection.reset();}
}
bool App::selection_input(Key key){
    if(!combat_selection_interactive()||!combat_choice_ready())return false;
    sync_selection();sync_combat_selection();uint16_t ids[MaxWorldUiChoices];const unsigned count=combat_selection_items(ids,MaxWorldUiChoices);
    if(!count||count>MaxWorldUiChoices)return false;cursor=CardGridScroll::clamp(cursor,0,int(count)-1);
    const auto kind=combat_selection_kind();
    if(key==Key::Left)cursor=(cursor+int(count)-1)%int(count);
    else if(key==Key::Right)cursor=(cursor+1)%int(count);
    else if(key==Key::Up||key==Key::Down){
        if(combat_selection_hand(kind)){
            const bool selected_row=key==Key::Up;for(unsigned i=0;i<count;++i){bool selected=false;
                for(unsigned j=0;j<choice_selected_count;++j)if(choice_selected[j]==ids[i])selected=true;
                if(selected==selected_row){cursor=int(i);break;}}
        }
    }else if(key==Key::Confirm)toggle_choice(ids[cursor]);
    else if(key==Key::EndTurn){if(kind==CombatSelectionKind::Offer){if(game.phase==Phase::WorldRelicChoice)world_choice_detail(ids[cursor]);else open_choice_detail(ids[cursor]);}else if(combat_selection_confirmable())confirm_choice();}
    else if(key==Key::Detail){if(game.phase==Phase::WorldRelicChoice)world_choice_detail(ids[cursor]);else open_choice_detail(ids[cursor]);}
    else if(key==Key::Back&&kind==CombatSelectionKind::Offer&&combat_selection_min()==0)skip_combat_selection();
    else return false;
    return true;
}
