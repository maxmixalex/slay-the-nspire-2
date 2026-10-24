namespace {
static bool world_add_rest_option(State& s,WorldRestOption option){
 auto& w=s.world;if(w.rest_count==MaxWorldRestOptions)return world_fail(s);
 w.rest_options[w.rest_count++]=uint8_t(option);return true;
}
static int world_rest_relic(const State& s,RelicId id){
 for(unsigned i=0;i<s.world_relics.count;++i)if(s.world_relics.entries[i].id==id&&!(s.world_relics.entries[i].flags&WorldRelicMelted))return int(i);
 return -1;
}
static bool world_make_rest(State& s){
 auto& w=s.world;s.phase=Phase::Camp;w.rest_count=w.rest_pending=0;w.rest_used=0;w.rest_deck_before=0;
 if(!world_add_rest_option(s,WorldRestOption::Heal))return false;
 if(!has_run_modifier(s,RunModifier::Midas)&&!world_add_rest_option(s,WorldRestOption::Smith))return false;
 for(unsigned i=0;i<s.deck_count;++i)if(s.deck[i].kind==EventCardByrdonisEgg&&!world_add_rest_option(s,WorldRestOption::Hatch))return false;
 for(unsigned i=0;i<s.world_relics.count;++i){const auto& r=s.world_relics.entries[i];if(r.flags&WorldRelicMelted)continue;
  WorldRestOption option=WorldRestOption::Count;
  switch(r.id){
  case WorldRelicShovel:option=WorldRestOption::Dig;break;
  case WorldRelicGirya:if(r.values[0]<3)option=WorldRestOption::Lift;break;
  case WorldRelicPumpkinCandle:option=WorldRestOption::Kindle;break;
  case WorldRelicMeatCleaver:option=WorldRestOption::Cook;break;
  case WorldRelicPaelsGrowth:option=WorldRestOption::Clone;break;
  default:break;
  }
  if(option!=WorldRestOption::Count&&!world_add_rest_option(s,option))return false;
 }
 return true;
}
}

unsigned world_rest_option_count(const State& s){return s.world.active?s.world.rest_count:2;}
const char* world_rest_option_key(const State& s,unsigned index){
 static const char* const keys[]={"HEAL","SMITH","HATCH","DIG","LIFT","KINDLE","COOK","CLONE"};
 if(!s.world.active)return index<2?keys[index]:"";
 return index<s.world.rest_count&&s.world.rest_options[index]<unsigned(WorldRestOption::Count)?keys[s.world.rest_options[index]]:"";
}
bool world_rest_option_enabled(const State& s,unsigned index){
 if(!s.world.active)return index==0||(index==1&&has_upgrade(s));
 const auto& w=s.world;if(s.phase!=Phase::Camp||!world_rest_option_visible(s,index)||w.rest_pending)return false;
 switch(WorldRestOption(w.rest_options[index])){
 case WorldRestOption::Smith:return has_upgrade(s);
 case WorldRestOption::Cook:{unsigned n=0;for(unsigned i=0;i<s.deck_count;++i)if(card_removable(s,s.deck[i],int(i)))++n;return n>=2;}
 default:return true;
 }
}
bool world_rest_option_visible(const State& s,unsigned index){
 if(!s.world.active)return index<2;
 const auto& w=s.world;
 return index<w.rest_count&&index<MaxWorldRestOptions&&!(w.rest_used&(uint32_t(1)<<index));
}
bool world_rest_choose(State& s,unsigned index){
 if(!s.world.active||!world_rest_option_enabled(s,index))return false;
 auto& w=s.world;auto option=WorldRestOption(w.rest_options[index]);w.rest_pending=uint8_t(index+1);w.rest_deck_before=s.deck_count;
 switch(option){
 case WorldRestOption::Heal:
  heal(s,rest_heal_amount(s));world_relic_hook(s,WorldRelicHook::AfterRest);
  if(has_run_modifier(s,RunModifier::NightTerrors)&&s.max_hp>1)player_lose_max_hp(s,min(5,s.max_hp-1));
  world_relic_rest_rewards(s);break;
 case WorldRestOption::Smith:s.phase=Phase::Upgrade;++s.revision;return true;
 case WorldRestOption::Hatch:if(!gain_world_relic(s,WorldRelicByrdpip))return false;break;
 case WorldRestOption::Dig:{RelicId id=random_world_relic(s);if(id==NoWorldRelic||!gain_world_relic(s,id))return world_fail(s);break;}
 case WorldRestOption::Lift:{int at=world_rest_relic(s,WorldRelicGirya);if(at<0)return world_fail(s);++s.world_relics.entries[at].values[0];world_relic_flash(s,WorldRelicGirya);break;}
 case WorldRestOption::Kindle:{int at=world_rest_relic(s,WorldRelicPumpkinCandle);if(at<0)return world_fail(s);s.world_relics.entries[at].values[0]+=5;world_relic_flash(s,WorldRelicPumpkinCandle);break;}
 case WorldRestOption::Cook:if(!world_relic_rest_select(s,true))return world_fail(s);break;
 case WorldRestOption::Clone:{
  const unsigned before=s.deck_count;unsigned count=0;for(unsigned i=0;i<before;++i)if(s.event_cards.deck[i].enchantment==EventEnchantment::Clone)++count;
  if(before+count>MaxDeck)return world_fail(s);
  for(unsigned i=0;i<before;++i)if(s.event_cards.deck[i].enchantment==EventEnchantment::Clone){
   int to=event_add_deck_card(s,s.deck[i],&s.event_cards.deck[i],true);if(to<0)return world_fail(s);notify_deck_copied(s,to,int(i));
  }
  world_relic_flash(s,WorldRelicPaelsGrowth);break;
 }
 default:return world_fail(s);
 }
 world_finish_room(s);++s.revision;return s.phase!=Phase::Error;
}
bool world_rest_cancel(State& s){
 auto& w=s.world;if(!w.active||!w.rest_pending||s.phase!=Phase::Upgrade||
    w.rest_pending>w.rest_count||WorldRestOption(w.rest_options[w.rest_pending-1])!=WorldRestOption::Smith)return false;
 w.rest_pending=0;w.rest_deck_before=0;w.finish_pending=0;s.phase=Phase::Camp;++s.revision;return true;
}
bool world_rest_can_proceed(const State& s){
 const auto&w=s.world;
 return w.active&&w.room_open&&w.room_type==unsigned(WorldRoom::Rest)&&s.phase==Phase::Camp&&!w.rest_pending&&
        !world_relic_pending(s)&&!s.original_events.active&&(w.rest_used||!w.rest_count);
}
bool world_rest_proceed(State& s){return world_rest_can_proceed(s)&&world_finish_room(s);}
bool world_rest_selection_allowed(const State& s,unsigned count){
 const auto& w=s.world;
 if(!w.active||!w.rest_pending||w.rest_pending>w.rest_count||WorldRestOption(w.rest_options[w.rest_pending-1])!=WorldRestOption::Cook)return true;
 return count==0||count==2;
}
