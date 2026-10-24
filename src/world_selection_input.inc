// Source CardSelectCmd routes deck, simple-grid and bundle selection to
// different overlays. The pending core choice keeps the original indexes;
// previewing never copies State or consumes any gameplay random stream.
RelicId App::world_selection_owner()const{
    if(game.phase!=Phase::WorldRelicChoice)return NoWorldRelic;
    const auto&w=game.world_relics;
    for(unsigned i=0;i<w.count;++i)if(w.entries[i].serial==w.choice_owner)return w.entries[i].id;
    return NoWorldRelic;
}
WorldSelectionKind App::world_selection_kind()const{
    if(combat_selection_kind()!=CombatSelectionKind::None||world_choice_is_card_reward())return WorldSelectionKind::None;
    if(game.phase==Phase::WorldRelicChoice){switch(game.world_relics.choice){
    case WorldRelicChoice::Remove:case WorldRelicChoice::Duplicate:return WorldSelectionKind::Deck;
    case WorldRelicChoice::Upgrade:return WorldSelectionKind::Upgrade;
    case WorldRelicChoice::Transform:return WorldSelectionKind::Transform;
    case WorldRelicChoice::Enchant:return WorldSelectionKind::Enchant;
    case WorldRelicChoice::OfferBundle:return WorldSelectionKind::Bundle;
    case WorldRelicChoice::OfferCard:if(world_selection_owner()==WorldRelicSeaGlass)return WorldSelectionKind::Simple;break;
    default:break;}
    }else if(game.phase==Phase::OriginalEventChoice){const auto&e=game.original_events;switch(e.selection){
    case OriginalEventSelection::Remove:case OriginalEventSelection::Duplicate:return WorldSelectionKind::Deck;
    case OriginalEventSelection::Upgrade:return WorldSelectionKind::Upgrade;
    case OriginalEventSelection::Transform:return e.id==OriginalEventId::WoodCarvings?WorldSelectionKind::Deck:WorldSelectionKind::Transform;
    case OriginalEventSelection::Enchant:return WorldSelectionKind::Enchant;
    case OriginalEventSelection::OfferCard:
        if(neow_modifier_selection(game,RunModifier::SealedDeck)||e.id==OriginalEventId::BrainLeech||e.id==OriginalEventId::RoomFullOfCheese)return WorldSelectionKind::Simple;break;
    default:break;}}
    return WorldSelectionKind::None;
}
bool App::world_selection_active()const{
    return world_selection_kind()!=WorldSelectionKind::None&&!title&&!paused&&!help&&!panel&&!library&&!character_select&&
        !settings_open&&!frontend_confirm&&!map_preview.open&&!map_window.is_open&&!event_reader&&!ftue.active()&&
        detail_card<0&&detail_item<0&&status_actor<0&&!choice_peeking&&combat_choice_ready();
}
bool App::world_selection_cancelable()const{
    if(game.phase==Phase::OriginalEventChoice)return game.original_events.select_cancelable!=0;
    // The relic core uses min==0 as an empty-selection continuation flag.
    // Source Cancelable remains false for Claws, Gnarled Hammer and Kifuda.
    const auto&w=game.world;
    return game.phase==Phase::WorldRelicChoice&&w.active&&w.rest_pending&&w.rest_pending<=w.rest_count&&
        (WorldRestOption(w.rest_options[w.rest_pending-1])==WorldRestOption::Cook||WorldRestOption(w.rest_options[w.rest_pending-1])==WorldRestOption::Smith);
}
unsigned App::world_selection_min()const{
    return world_selection_cancelable()&&game.phase==Phase::WorldRelicChoice?world_choice_max():world_choice_min();
}
unsigned App::world_selection_max()const{
    // These source selectors keep their authored MaxSelect even when fewer
    // eligible cards exist. The core's clamped bound remains the commit limit.
    const auto owner=world_selection_owner();
    if(owner==WorldRelicClaws)return 6;
    if(owner==WorldRelicGnarledHammer||owner==WorldRelicKifuda)return 3;
    return world_choice_max();
}
bool App::world_selection_manual()const{
    if(world_selection_kind()==WorldSelectionKind::Simple){
        return game.phase==Phase::WorldRelicChoice||neow_modifier_selection(game,RunModifier::SealedDeck)||world_selection_min()!=world_choice_max();
    }
    return true; // Every deck overlay opens its original preview at MaxSelect.
}
bool App::world_selection_upgrades()const{
    const auto kind=world_selection_kind();return kind==WorldSelectionKind::Upgrade||kind==WorldSelectionKind::Transform;
}
bool App::world_selection_confirmable()const{
    if(!world_selection_active()||choice_selected_count<world_selection_min()||choice_selected_count>world_choice_max()||
       !world_rest_selection_allowed(game,choice_selected_count))return false;
    const auto kind=world_selection_kind();
    // The pinned enchant screen enables its empty-preview Confirm, but
    // ConfirmSelection itself ignores zero cards. Preserve that distinction.
    if(deck_selection.preview)return true;
    if(kind==WorldSelectionKind::Simple)return world_selection_manual();
    if(kind==WorldSelectionKind::Bundle||kind==WorldSelectionKind::Upgrade)return false;
    return world_selection_min()!=world_selection_max();
}
CombatSelectionPrompt App::world_selection_prompt()const{
    const auto kind=world_selection_kind();
    if(kind==WorldSelectionKind::Bundle)return {"gameplay_ui","CHOOSE_A_PACK"};
    if(game.phase==Phase::WorldRelicChoice){const auto owner=world_selection_owner();
        if(owner==WorldRelicDollysMirror)return {"relics","DOLLYS_MIRROR.selectionScreenPrompt"};
        if(owner==WorldRelicClaws)return {"relics","CLAWS.selectionScreenPrompt"};
        if(owner==WorldRelicSeaGlass)return {"relics","SEA_GLASS.selectionScreenPrompt"};
        if(game.world_relics.choice==WorldRelicChoice::Remove&&game.world_relics.choice_reward)return {"gameplay_ui","COMBAT_REWARD_CARD_REMOVAL.selectionScreenPrompt"};
    }else{const auto&e=game.original_events;
        if(neow_modifier_selection(game,RunModifier::SealedDeck))return {"modifiers","SEALED_DECK.selectionPrompt"};
        if(kind==WorldSelectionKind::Simple&&e.id==OriginalEventId::BrainLeech)return {"events","BRAIN_LEECH.pages.SHARE_KNOWLEDGE.selectionScreenPrompt"};
        if(kind==WorldSelectionKind::Simple&&e.id==OriginalEventId::RoomFullOfCheese)return {"events","ROOM_FULL_OF_CHEESE.pages.GORGE.selectionScreenPrompt"};
        if(e.selection==OriginalEventSelection::Transform)return {"card_selection","TO_TRANSFORM"};
    }
    return {"card_selection",kind==WorldSelectionKind::Upgrade?"TO_UPGRADE":kind==WorldSelectionKind::Transform?"TO_TRANSFORM":kind==WorldSelectionKind::Enchant?"TO_ENCHANT":"TO_REMOVE"};
}
EventEnchantment App::world_selection_enchantment(int*amount)const{
    if(amount)*amount=game.phase==Phase::WorldRelicChoice?game.world_relics.choice_amount:game.original_events.select_amount;
    return game.phase==Phase::WorldRelicChoice?game.world_relics.choice_enchantment:EventEnchantment(game.original_events.select_enchantment);
}
void App::sync_world_selection(){
    if(world_selection_kind()==WorldSelectionKind::None)return;
    sync_selection();auto&m=deck_selection;
    if(!m.bound||m.revision!=game.revision||m.phase!=unsigned(game.phase)){
        m.reset();m.bound=true;m.revision=game.revision;m.phase=uint8_t(game.phase);m.opened=map_clock_ms;
    }
    if(!pointer.visible&&world_selection_active())m.show_upgrades=false;
}
void App::cancel_world_selection(){
    if(!world_selection_active())return;sync_world_selection();
    if(deck_selection.preview){
        if(world_selection_kind()==WorldSelectionKind::Bundle&&choice_selected_count){
            auto&motion=card_grids[unsigned(CardGridSurface::WorldChoice)].faces[15];
            motion.index=int16_t(choice_selected[0]);motion.start=deck_selection.preview_opened;motion.selection_start=map_clock_ms;motion.chosen=true;
        }
        if(choice_selected_count){uint16_t ids[MaxWorldUiChoices];const unsigned count=world_choice_options(ids,MaxWorldUiChoices);
            for(unsigned i=0;i<count&&i<MaxWorldUiChoices;++i)if(ids[i]==choice_selected[choice_selected_count-1])cursor=int(i);}
        choice_selected_count=0;deck_selection.preview=false;pointer_cancel();focus_card_grid();return;
    }
    if(!world_selection_cancelable())return;
    continue_combat_choice();const bool done=game.phase==Phase::WorldRelicChoice?world_relic_select(game,nullptr,0):original_event_select(game,nullptr,0);
    if(done){choice_selected_count=0;choice_revision=game.revision;deck_selection.reset();cursor=0;busy=0;pointer_cancel();}
}
unsigned App::world_selection_preview_cards()const{
    if(!deck_selection.preview)return 0;const auto kind=world_selection_kind();
    if(kind==WorldSelectionKind::Bundle)return choice_selected_count?3:0;
    if(kind==WorldSelectionKind::Transform)return choice_selected_count*2;
    if((kind==WorldSelectionKind::Upgrade||kind==WorldSelectionKind::Enchant)&&world_selection_max()==1)return choice_selected_count*2;
    return choice_selected_count;
}
bool App::world_selection_preview_card(unsigned index,WorldSelectionCard&out)const{
    out={};const auto kind=world_selection_kind();const unsigned count=choice_selected_count;
    if(index>=world_selection_preview_cards())return false;
    using namespace deck_selection_layout;
    const bool paired=kind==WorldSelectionKind::Transform||((kind==WorldSelectionKind::Upgrade||kind==WorldSelectionKind::Enchant)&&world_selection_max()==1);
    const bool after=paired&&index>=count;const unsigned at=paired?index%count:index;
    const int deck=kind==WorldSelectionKind::Bundle?-1:choice_selected[at];
    if(kind==WorldSelectionKind::Bundle){const unsigned offer=choice_selected[0]*3+at;if(offer>=game.world_relics.offer_count)return false;
        out.card=card_from_token(game.world_relics.offers[offer]);out.facts=game.world_relics.offer_facts[offer];
    }else{if(deck<0||deck>=game.deck_count)return false;out.deck=int16_t(deck);out.card=game.deck[deck];out.facts=game.event_cards.deck[deck];}
    if(kind==WorldSelectionKind::Upgrade&&(after||!paired)){out.card.upgraded=1;out.upgraded=true;}
    if(kind==WorldSelectionKind::Enchant&&after){int amount=0;const auto enchant=world_selection_enchantment(&amount);
        if(!event_card_apply_enchantment(out.card,out.facts,enchant,amount))return false;}
    if(kind==WorldSelectionKind::Transform&&after){
        const Card original=out.card;CardKind replacement=CardKinds;const auto owner=world_selection_owner();
        if(game.phase==Phase::WorldRelicChoice&&owner==WorldRelicClaws)replacement=EventCardMaul;
        else if(game.phase==Phase::OriginalEventChoice&&game.original_events.select_extra>0)replacement=CardKind(game.original_events.select_extra-1);
        out.hover=replacement<CardKinds;out.deck=-1;out.facts={};
        if(replacement>=CardKinds){
            const unsigned rarity=card_rarity(original.kind);WorldCardOptions options;options.flags=WorldNoModifications;
            options.pools[0]=definitions[original.kind].type==CardType::Quest||rarity==5||rarity==6||rarity==7?WorldCardPool::Colorless:world_card_pool(original.kind);
            if(rarity!=8&&rarity!=9)options.rarity_mask=world_rarity_mask(2)|world_rarity_mask(3)|world_rarity_mask(4);
            CardKind candidates[CardKinds];const unsigned available=world_card_candidates(game,options,candidates,CardKinds);
            unsigned valid=0;for(unsigned i=0;i<available;++i)if(candidates[i]!=original.kind)candidates[valid++]=candidates[i];
            if(!valid)return false;
            // The source cycles an independently shuffled preview every .2s.
            // This deterministic presentation order never touches run RNG.
            replacement=candidates[(uint32_t(map_clock_ms-deck_selection.preview_opened)/200+at*17+unsigned(original.kind))%valid];
        }
        out.card={replacement,0};
        if(owner==WorldRelicClaws){out.card.upgraded=uint8_t(original.upgraded&&card_upgradeable(out.card));const auto&facts=game.event_cards.deck[deck];
            if(facts.enchantment!=EventEnchantment::None&&event_card_can_enchant(game,out.card,out.facts,facts.enchantment))event_card_apply_enchantment(out.card,out.facts,facts.enchantment,facts.enchantment_amount);}
        if(owner==WorldRelicAstrolabe&&card_upgradeable(out.card))out.card.upgraded=1;
    }
    int scale=1000,cx=160,cy=120;
    if(kind==WorldSelectionKind::Transform){
        const int source_width=int(count)*300+int(count-1)*30;scale=source_width>610?610000/source_width:1000;
        const int center=after?960:710,offset=after?((int(at)*2+1)*150*scale/1000+int(at)*30):(-((int(count-at)*2-1)*150*scale/1000)-int(count-at-1)*30);
        cx=(center+offset)*4/21;
    }else if(paired)cx=after?213:107;
    else{
        scale=count>6?550:count>3?800:1000;
        const unsigned displayed=kind==WorldSelectionKind::Bundle?3:count;
        const int slot=kind==WorldSelectionKind::Bundle?int(displayed-1-at):int(at);
        cx=160+(2*slot-int(displayed)+1)*200*4*scale/(21*1000);
        if(kind==WorldSelectionKind::Bundle)cy=131;
    }
    out.w=int16_t((PreviewW*scale+500)/1000);out.h=int16_t((PreviewH*scale+500)/1000);
    out.x=int16_t(cx-out.w/2);out.y=int16_t(cy-out.h/2);return true;
}
