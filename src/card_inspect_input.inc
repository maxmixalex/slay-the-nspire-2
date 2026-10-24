unsigned App::card_inspect_ids(int*out)const{
    const auto source=card_inspect.source;
    if(source==CardInspectSource::History){const auto*r=history_at(history_index);if(!r)return 0;if(out)for(unsigned i=0;i<r->deck_count;++i)out[i]=int(i);return r->deck_count;}
    if(source==CardInspectSource::Library){CardKind ids[CardKinds];const unsigned count=library_items(ids);unsigned seen=0;
        for(unsigned i=0;i<count;++i)if(library_card_seen(ids[i])){if(out)out[seen]=ids[i];++seen;}return seen;}
    if(source==CardInspectSource::DeckPile||source==CardInspectSource::InstancePile)return pile_items(out);
    if(source==CardInspectSource::DeckChoice){if(out)for(unsigned i=0;i<game.deck_count;++i)out[i]=int(i);return game.deck_count;}
    if(source==CardInspectSource::Hand){const auto hand=combat_hand();if(out)for(unsigned i=0;i<hand.count;++i)out[i]=hand.ids[i];return hand.count;}
    if(source==CardInspectSource::CombatChoice){const auto&pile=choice_pile(game);if(out)for(unsigned i=0;i<pile.count;++i)out[i]=pile.ids[i];return pile.count;}
    if(source==CardInspectSource::WorldChoice){uint16_t ids[MaxWorldUiChoices];const unsigned count=world_choice_options(out?ids:nullptr,MaxWorldUiChoices);if(count>CardKinds)return 0;if(out)for(unsigned i=0;i<count;++i)out[i]=ids[i];return count;}
    if(source==CardInspectSource::Reward){const unsigned count=game.world.active?game.world.reward_count:3;if(out)for(unsigned i=0;i<count;++i)out[i]=int(i);return count;}
    return 0;
}
void App::bind_card_inspect(){
    card_inspect={};card_inspect.opened=map_clock_ms;
    if(detail_card<0)return;
    int wanted=-1;const Card card=card_from_token(detail_card);
    if(history_open){card_inspect.source=CardInspectSource::History;wanted=history_selected;}
    else if(library&&library_tab==0){card_inspect.source=CardInspectSource::Library;wanted=card.kind;card_inspect.view_all_upgraded=library_upgraded;}
    else if(panel==1){card_inspect.source=pile_tab?CardInspectSource::InstancePile:CardInspectSource::DeckPile;wanted=pile_tab?detail_instance:detail_deck_index;card_inspect.view_all_upgraded=!pile_tab&&pile_upgraded;}
    else if(!panel&&(game.phase==Phase::Upgrade||game.phase==Phase::Remove||game.phase==Phase::Transform||grimoire_select)){
        card_inspect.source=CardInspectSource::DeckChoice;wanted=detail_deck_index;
    }else if(!panel&&(game.phase==Phase::WorldRelicChoice||game.phase==Phase::OriginalEventChoice)){
        const bool relic=game.phase==Phase::WorldRelicChoice;const auto&w=game.world_relics;const auto&e=game.original_events;
        const bool offer=relic?(w.choice==WorldRelicChoice::OfferCard||w.choice==WorldRelicChoice::CombatOffer):e.selection==OriginalEventSelection::OfferCard;
        const bool hand=relic&&w.choice==WorldRelicChoice::Hand;
        const bool invalid=world_choice_is_potion_reward()||(relic?(w.choice==WorldRelicChoice::OfferReward||w.choice==WorldRelicChoice::OfferBundle):(e.selection==OriginalEventSelection::Rewards||e.selection==OriginalEventSelection::Relic||e.selection==OriginalEventSelection::Crystal));
        if(!invalid){card_inspect.source=CardInspectSource::WorldChoice;wanted=hand?detail_instance:detail_deck_index;
            if(offer){wanted=-1;const unsigned count=relic?w.offer_count:e.offer_count;
                for(unsigned i=0;i<count;++i)if(card_from_token(relic?w.offers[i]:e.offers[i]).kind==card.kind){wanted=int(i);if(int(i)==cursor)break;}}
        }
    }else if(!panel&&game.phase==Phase::Reward&&reward_cards){
        card_inspect.source=CardInspectSource::Reward;const unsigned count=game.world.active?game.world.reward_count:3;
        for(unsigned i=0;i<count;++i)if((game.world.active?game.world.reward_cards[i].kind:game.reward[i])==card.kind){wanted=int(i);if(int(i)==cursor)break;}
    }else if(!panel&&choice_phase(game.phase)){card_inspect.source=CardInspectSource::CombatChoice;wanted=detail_instance;
        if(wanted<0){const auto&pile=choice_pile(game);for(unsigned i=0;i<pile.count;++i)if(choice_card(game,pile.ids[i]).kind==card.kind){wanted=pile.ids[i];if(int(i)==cursor)break;}}
    }else if(!panel&&game.phase==Phase::Combat&&detail_instance>=0){card_inspect.source=CardInspectSource::Hand;wanted=detail_instance;}
    int ids[CardKinds];const unsigned count=card_inspect_ids(ids);
    for(unsigned i=0;i<count;++i)if(ids[i]==wanted){card_inspect.index=int16_t(i);card_inspect.count=int16_t(count);return;}
    card_inspect.source=CardInspectSource::None;card_inspect.index=0;card_inspect.count=1;
}
void App::move_card_inspect(int direction){
    if(detail_card<0||!direction)return;
    int ids[CardKinds];const unsigned count=card_inspect_ids(ids);const int next=card_inspect.index+(direction<0?-1:1);
    if(next<0||next>=int(count))return;
    const auto context=card_inspect;const int id=ids[next];
    switch(context.source){
    case CardInspectSource::History:{const auto*r=history_at(history_index);if(!r||id<0||id>=r->deck_count)return;const auto&c=r->deck[id];history_selected=id;open_card_offer(c.card,c.facts,c.genetic,c.scythe);break;}
    case CardInspectSource::Library:open_card_detail({CardKind(id),0});break;
    case CardInspectSource::DeckPile:case CardInspectSource::DeckChoice:open_card_detail(game.deck[id],id);break;
    case CardInspectSource::InstancePile:case CardInspectSource::Hand:case CardInspectSource::CombatChoice:open_choice_detail(id);break;
    case CardInspectSource::WorldChoice:world_choice_detail(id);break;
    case CardInspectSource::Reward:
        if(game.world.active)open_card_offer(game.world.reward_cards[id],game.world.reward_facts[id]);
        else open_card_detail({game.reward[id],0});break;
    default:return;
    }
    if(context.view_all_upgraded){auto card=card_from_token(detail_card);if(card_upgradeable(card)){card.upgraded=1;detail_card=card_token(card);}}
    card_inspect=context;card_inspect.index=int16_t(next);card_inspect.count=int16_t(count);card_inspect.slid=map_clock_ms;card_inspect.direction=int8_t(direction<0?-1:1);card_inspect.description_truncated=false;
}
