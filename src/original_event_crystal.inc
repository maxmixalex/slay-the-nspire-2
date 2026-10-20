// CrystalSphereMinigame: x-major placement candidates and source clear order.
// Item sizes are rules, independent of the scaled calculator presentation.
bool crystal_begin(State&s,unsigned charges){auto&e=s.original_events;
 e.crystal_tool=2;e.crystal_charges=uint8_t(charges);e.crystal_item_count=e.crystal_revealed_count=0;
 for(auto&v:e.crystal_owner)v=0;for(auto&v:e.crystal_clear)v=0;for(auto&v:e.crystal_items)v={};for(auto&v:e.crystal_revealed)v=0;
 for(unsigned x=0;x<11;x++)for(unsigned y=0;y<11;y++)if(smaller(int(x),10-int(x))+smaller(int(y),10-int(y))<=2)e.crystal_clear[y*11+x]=1;
 auto rng=event_random(s);bool placed_all=false;
 for(unsigned attempt=0;attempt<10&&!placed_all;attempt++){
  placed_all=true;
  for(unsigned item=0;item<15;item++){
   auto&v=e.crystal_items[e.crystal_item_count];
   if(item==0){v.kind=4;v.w=v.h=4;}
   else if(item<=3){v.kind=3;v.model=item<3?1:3;v.w=item<3?1:2;v.h=item<3?3:2;}
   else if(item<=6){v.kind=0;v.model=uint16_t(item-2);v.w=v.h=2;}
   else if(item==7){v.kind=1;v.w=v.h=2;}
   else{v.kind=2;v.w=item<13?1:2;v.h=1;v.amount=item<13?10:30;}
   if(placed_all){uint16_t positions[CrystalCells];unsigned count=0;
    for(unsigned x=0;x+v.w<=11;x++)for(unsigned y=0;y+v.h<=11;y++){bool valid=true;for(unsigned dx=0;dx<v.w;dx++)for(unsigned dy=0;dy<v.h;dy++)if(e.crystal_clear[(y+dy)*11+x+dx]||e.crystal_owner[(y+dy)*11+x+dx])valid=false;if(valid)positions[count++]=uint16_t(y*11+x);}
    if(!count)placed_all=false;else{unsigned pos=positions[rng.below(count)];v.x=uint8_t(pos%11);v.y=uint8_t(pos/11);for(unsigned dx=0;dx<v.w;dx++)for(unsigned dy=0;dy<v.h;dy++)e.crystal_owner[(v.y+dy)*11+v.x+dx]=uint8_t(e.crystal_item_count+1);}
   }
   ++e.crystal_item_count;
  }
  // The pinned constructor retries without clearing the accumulated items.
  // Its repeated Revealed subscription is also persisted, rather than silently
  // substituting a different board generator for these rare failed placements.
  for(unsigned i=0;i<e.crystal_item_count;i++)++e.crystal_items[i].subscriptions;
 }
 keep_random(s,rng);e.page=1;e.selection=OriginalEventSelection::Crystal;e.select_min=e.select_max=0;e.select_cancelable=0;s.phase=Phase::OriginalEventChoice;return true;
}
bool crystal_clear(State&s,unsigned x,unsigned y){auto&e=s.original_events;if(x>=11||y>=11)return false;unsigned pos=y*11+x;if(e.crystal_clear[pos])return true;e.crystal_clear[pos]=1;
 if(!e.crystal_owner[pos])return true;unsigned index=e.crystal_owner[pos]-1;auto&item=e.crystal_items[index];if(item.revealed)return true;
 for(unsigned dx=0;dx<item.w;dx++)for(unsigned dy=0;dy<item.h;dy++)if(!e.crystal_clear[(item.y+dy)*11+item.x+dx])return true;
 item.revealed=1;for(unsigned n=0;n<item.subscriptions;n++){if(e.crystal_revealed_count>=MaxCrystalReveals)return false;e.crystal_revealed[e.crystal_revealed_count++]=uint8_t(index);}
 if(item.kind==1)return event_add_deck_card(s,{EventCardDoubt,0})>=0;return true;
}
bool crystal_next(State&s){auto&e=s.original_events;
 if(e.crystal_charges){e.selection=OriginalEventSelection::Crystal;e.select_min=e.select_max=0;e.select_cancelable=0;s.phase=Phase::OriginalEventChoice;return true;}
 reward_reset(s);
 // ToReward is evaluated for the whole revealed list before RewardsSet.Populate.
 // Potion models are selected at ToReward time; other models populate later.
 uint16_t potion_models[MaxCrystalReveals]{};
 for(unsigned i=0;i<e.crystal_revealed_count;i++){auto&item=e.crystal_items[e.crystal_revealed[i]];if(item.kind==3){Potion potion;auto rng=event_random(s);if(world_random_potions(s,rng,1,&potion,item.model)!=1)return false;keep_random(s,rng);potion_models[i]=uint16_t(potion);}}
 for(unsigned i=0;i<e.crystal_revealed_count;i++){const auto&item=e.crystal_items[e.crystal_revealed[i]];
  switch(item.kind){
  case 0:{WorldCardOptions o;o.odds=WorldCardOdds::Uniform;o.rarity_mask=world_rarity_mask(item.model);o.flags=WorldIsCardReward;if(!reward_cards(s,3,o,true))return false;break;}
  case 1:break;
  case 2:rand_range(s,item.amount,item.amount+1);if(!reward_gold(s,item.amount))return false;break;
  case 3:if(!reward_potion(s,Potion(potion_models[i])))return false;break;
  case 4:if(!reward_random_relic(s,true))return false;break;
  default:return false;
  }
 }
 finish_page(s,"FINISH");return reward_offer(s);
}
