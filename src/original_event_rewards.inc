void reward_reset(State&s){auto&e=s.original_events;e.reward_active=0;e.reward_count=e.reward_card_count=0;e.reward_current=255;for(auto&r:e.rewards)r={};for(auto&c:e.reward_cards)c=0;for(auto&f:e.reward_facts)f={};}
OriginalEventReward* reward_slot(State&s,unsigned kind){auto&e=s.original_events;if(e.reward_count>=MaxOriginalEventRewards){fail(s);return nullptr;}auto&r=e.rewards[e.reward_count++];r={};r.kind=uint8_t(kind);return &r;}
bool reward_gold(State&s,unsigned amount){auto*r=reward_slot(s,1);if(!r)return false;r->amount=uint16_t(amount);return true;}
bool reward_potion(State&s,Potion potion){auto*r=reward_slot(s,2);if(!r)return false;r->model=uint16_t(potion);return true;}
bool reward_relic(State&s,RelicId id){if(id>=WorldRelicKinds)return false;auto*r=reward_slot(s,3);if(!r)return false;r->model=id;return true;}
bool reward_random_potion(State&s,unsigned rarity=0,bool event_rng=false){auto local=event_random(s);auto&rng=event_rng?local:world_rng(s,WorldRng::Rewards);Potion potion{};if(world_random_potions(s,rng,1,&potion,rarity)!=1)return false;if(event_rng)keep_random(s,rng);return reward_potion(s,potion);}
bool reward_random_relic(State&s,bool event_rng=false){if(!event_rng)return reward_relic(s,random_world_relic(s));
 auto rng=event_random(s);auto draw=world_random_float(rng);WorldRelicPool pool=draw<world_float(0x3f000000U)?WorldRelicPool::Common:draw<world_float(0x3f547ae1U)?WorldRelicPool::Uncommon:WorldRelicPool::Rare;keep_random(s,rng);return reward_relic(s,world_relic_pull(s,pool));}
bool reward_cards(State&s,unsigned count,const WorldCardOptions&options,bool event_rng=false,bool upgrade=false){auto&e=s.original_events;
 if(e.reward_count>=MaxOriginalEventRewards||e.reward_card_count+count>MaxOriginalEventOffers)return false;
 Card cards[MaxOriginalEventOffers];EventCardFacts facts[MaxOriginalEventOffers];unsigned made=0;auto local=event_random(s);auto&rng=event_rng?local:world_rng(s,WorldRng::Rewards);
 if(!world_create_reward(s,rng,count,options,cards,facts,MaxOriginalEventOffers,&made)||e.reward_card_count+made>MaxOriginalEventOffers)return false;if(event_rng)keep_random(s,rng);
 auto*r=reward_slot(s,4);if(!r)return false;r->first=e.reward_card_count;r->count=uint8_t(made);
 for(unsigned i=0;i<made;i++){if(upgrade&&card_upgradeable(cards[i]))cards[i].upgraded=1;e.reward_cards[e.reward_card_count]=uint16_t(card_token(cards[i]));e.reward_facts[e.reward_card_count++]=facts[i];}return true;
}
bool reward_fixed_card(State&s,Card card){auto&e=s.original_events;if(e.reward_card_count>=MaxOriginalEventOffers)return false;auto*r=reward_slot(s,4);if(!r)return false;r->first=e.reward_card_count;r->count=1;e.reward_cards[e.reward_card_count]=uint16_t(card_token(card));e.reward_facts[e.reward_card_count++]={};return true;}
bool reward_menu(State&s){auto&e=s.original_events;unsigned left=0;for(unsigned i=0;i<e.reward_count;i++)if(!e.rewards[i].claimed)++left;
 if(!left){e.reward_active=0;return true;}e.reward_active=1;e.reward_current=255;return pause_selection(s,OriginalEventSelection::Rewards,OriginalEventFilter::Any,0,1,true);
}
bool reward_offer(State&s){auto&e=s.original_events;
 // RewardsSet populates the supplied sequence, then sorts it for presentation.
 for(unsigned i=1;i<e.reward_count;i++){auto value=e.rewards[i];unsigned j=i;while(j&&e.rewards[j-1].kind>value.kind){e.rewards[j]=e.rewards[j-1];--j;}e.rewards[j]=value;}
 world_relic_before_reward_offered(s,e.reward_card_count!=0);e.reward_active=1;return reward_menu(s);
}
