void reward_reset(State&s){auto&e=s.original_events;e.reward_active=0;e.reward_count=e.reward_card_count=0;e.reward_current=NoOriginalEventReward;for(auto&r:e.rewards)r={};for(auto&c:e.reward_cards)c=0;for(auto&f:e.reward_facts)f={};}
OriginalEventReward* reward_slot(State&s,unsigned kind){auto&e=s.original_events;if(e.reward_count>=MaxOriginalEventRewards){fail(s);return nullptr;}auto&r=e.rewards[e.reward_count++];r={};r.kind=uint8_t(kind);return &r;}
bool reward_gold(State&s,unsigned amount){auto*r=reward_slot(s,1);if(!r)return false;r->amount=uint16_t(amount);return true;}
bool reward_potion(State&s,Potion potion){auto*r=reward_slot(s,2);if(!r)return false;r->model=uint16_t(potion);return true;}
bool reward_relic(State&s,RelicId id){if(id>=WorldRelicKinds)return false;auto*r=reward_slot(s,3);if(!r)return false;r->model=id;return true;}
bool reward_random_potion(State&s,unsigned rarity=0,bool event_rng=false){auto local=event_random(s);auto&rng=event_rng?local:world_rng(s,WorldRng::Rewards);Potion potion{};if(world_random_potions(s,rng,1,&potion,rarity)!=1)return false;if(event_rng)keep_random(s,rng);return reward_potion(s,potion);}
bool reward_random_relic(State&s,bool event_rng=false){if(!event_rng)return reward_relic(s,random_world_relic(s));
 auto rng=event_random(s);auto draw=world_random_float(rng);WorldRelicPool pool=draw<world_float(0x3f000000U)?WorldRelicPool::Common:draw<world_float(0x3f547ae1U)?WorldRelicPool::Uncommon:WorldRelicPool::Rare;keep_random(s,rng);return reward_relic(s,world_relic_pull(s,pool));}
bool reward_cards(State&s,unsigned count,const WorldCardOptions&options,bool event_rng=false,bool upgrade=false){auto&e=s.original_events;
 if(e.reward_count>=MaxOriginalEventRewards||e.reward_card_count+count>MaxOriginalEventRewardCards)return false;
 Card cards[MaxOriginalEventOffers];EventCardFacts facts[MaxOriginalEventOffers];unsigned made=0;auto local=event_random(s);auto&rng=event_rng?local:world_rng(s,WorldRng::Rewards);
 if(!world_create_reward(s,rng,count,options,cards,facts,MaxOriginalEventOffers,&made)||e.reward_card_count+made>MaxOriginalEventRewardCards)return false;if(event_rng)keep_random(s,rng);
 auto*r=reward_slot(s,4);if(!r)return false;r->first=e.reward_card_count;r->count=uint8_t(made);r->flags=uint8_t(1|(upgrade?2:0)|(event_rng?16:0));r->original_count=uint8_t(count);r->type_mask=options.type_mask;r->zero_energy=options.zero_energy;r->rarity_mask=options.rarity_mask;r->creation_flags=options.flags;r->pool_count=options.pool_count;r->odds=uint8_t(options.odds);r->source=uint8_t(options.source);for(unsigned i=0;i<options.pool_count;i++)r->pools[i]=uint8_t(options.pools[i]);
 for(unsigned i=0;i<made;i++){if(upgrade&&card_upgradeable(cards[i]))cards[i].upgraded=1;e.reward_cards[e.reward_card_count]=uint16_t(card_token(cards[i]));e.reward_facts[e.reward_card_count++]=facts[i];}return true;
}
bool reward_fixed_card(State&s,Card card){auto&e=s.original_events;if(e.reward_card_count>=MaxOriginalEventRewardCards)return false;auto*r=reward_slot(s,4);if(!r)return false;r->first=e.reward_card_count;r->count=1;e.reward_cards[e.reward_card_count]=uint16_t(card_token(card));e.reward_facts[e.reward_card_count++]={};return true;}
bool reward_menu(State&s){auto&e=s.original_events;unsigned left=0;for(unsigned i=0;i<e.reward_count;i++)if(!e.rewards[i].claimed)++left;
 if(!left){e.reward_active=0;return true;}e.reward_active=1;e.reward_current=NoOriginalEventReward;return pause_selection(s,OriginalEventSelection::Rewards,OriginalEventFilter::Any,0,1,true);
}
bool reward_offer(State&s){auto&e=s.original_events;
 if(has_run_modifier(s,RunModifier::Midas))for(unsigned i=0;i<e.reward_count;++i)if(e.rewards[i].kind==1)e.rewards[i].amount=uint16_t(smaller(65535,int(e.rewards[i].amount)*2));
 // RewardsSet populates the supplied sequence, then sorts it for presentation.
 reference::comparison_sort(e.rewards,e.reward_count,[](const OriginalEventReward&r){return r.kind==4?((r.flags&1)?5:4):int(r.kind);});
 if(has_world_relic(s,WorldRelicDriftwood))for(unsigned i=0;i<e.reward_count;i++)if(e.rewards[i].kind==4&&(e.rewards[i].flags&1)&&!(e.rewards[i].flags&8))e.rewards[i].flags|=4;
 e.reward_active=1;return world_relic_pending(s)||reward_menu(s);
}
