// CardSelectCmd returns the complete eligible set without opening a selector
// when a nonmanual hand/pile request cannot offer a meaningful choice. Apply
// that result inside the existing effect loop, before presentation observes
// it. These helpers never reenter resolve or a public choose entry point.
static bool apply_exhaust_choice(State&s,int index){
 if(s.phase!=Phase::ExhaustChoice||index<0||index>=s.hand.count)return false;
 s.phase=Phase::Combat;exhaust(s,index);return true;
}
static bool apply_discard_choice(State&s,const uint16_t*ids,unsigned count){
 if(s.phase!=Phase::DiscardChoice||!ids||count!=s.discard_required||!count||count>MaxHand)return false;
 uint16_t selected[MaxHand];
 for(unsigned i=0;i<count;i++){
  bool in_hand=false;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i])in_hand=true;
  if(!in_hand)return false;for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;
  selected[i]=ids[i];
 }
 for(unsigned i=0;i<count;i++)for(int j=0;j<s.hand.count;j++)if(s.hand.ids[j]==selected[i]){
  int id=remove(s.hand,j);put(s,s.discard,id,Zone::Discard);record(s,LogKind::Discard,s.cards[id].kind);break;
 }
 for(unsigned i=count;i>0;i--)silent_card_discarded(s,selected[i-1]);
 s.discard_required=0;s.phase=Phase::Combat;return true;
}
static bool apply_return_choice(State&s,int card_id){
 if(s.phase!=Phase::ReturnChoice||card_id<0||card_id>=s.card_count)return false;
 int index=-1;for(int i=0;i<s.discard.count;i++)if(s.discard.ids[i]==card_id){index=i;break;}
 if(index<0)return false;
 int id=remove(s.discard,index);put(s,s.draw,id,Zone::Draw);s.phase=Phase::Combat;return true;
}
static bool fixed_combat_selection(XChoice choice){
 // Source route metadata, not an inference from the clamped saved min/max.
 // Optional hand/pile choices and generated offers are deliberately absent.
 switch(choice){
 case XChoice::Draw:case XChoice::Discard:case XChoice::Top:case XChoice::Free:case XChoice::Transform:
 case XChoice::IUpgrade:case XChoice::SlySkill:case XChoice::NightmareCard:
 case XChoice::RTransformHand:case XChoice::RTransformDraw:case XChoice::RDiscardTop:
 case XChoice::RForegone:case XChoice::RDecisions:case XChoice::RHeirloom:case XChoice::RTyranny:
 case XChoice::NCleanse:case XChoice::NDredge:case XChoice::NEthereal:case XChoice::NRetain:
 case XChoice::NSeance:case XChoice::NTransfigure:case XChoice::EventDualWield:case XChoice::EventWish:return true;
 default:return false;
 }
}
static bool selection_from_draw(XChoice choice){
 return choice==XChoice::Draw||choice==XChoice::RTransformDraw||choice==XChoice::RForegone||
        choice==XChoice::NCleanse||choice==XChoice::NSeance||choice==XChoice::EventWish;
}
static void apply_complete_draw_choice(State&s,const Pile&pool,bool free){
 // A full Draw alias is consumed from its last index; filtered local pools
 // retain their IDs. Neither case needs a second MaxCards-sized snapshot.
 for(unsigned i=pool.count;i>0;--i){
  const int id=pool.ids[i-1];const bool room=s.hand.count<MaxHand;
  if(!move_instance(s,id,s.draw,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard)){fail(s);return;}
  if(free)set_free_this_turn(s,id);
 }
}
static void resolve_trivial_card_selection(State&s){
 bool applied=true;
 if(s.phase==Phase::ExhaustChoice&&s.hand.count==1)applied=apply_exhaust_choice(s,0);
 else if(s.phase==Phase::DiscardChoice&&s.hand.count&&s.hand.count<=s.discard_required)
  applied=apply_discard_choice(s,s.hand.ids,s.hand.count);
 else if(s.phase==Phase::ReturnChoice&&s.discard.count==1)applied=apply_return_choice(s,s.discard.ids[0]);
 else if(s.phase==Phase::ExtendedChoice){
  const auto&x=s.expansion;const unsigned count=x.choices.count;
  if(!fixed_combat_selection(x.choice)||!count||count>MaxHand||count>x.choice_min||x.choice_min!=x.choice_max)return;
  uint16_t selected[MaxHand];
  // Original Draw.Cards enumerates top first; this engine stores its top at
  // the last index. The shortcut precedes the screen's rarity/name sorting.
  const bool reverse=selection_from_draw(x.choice);
  for(unsigned i=0;i<count;i++)selected[i]=x.choices.ids[reverse?count-1-i:i];
  applied=apply_extended_choice(s,selected,count);
 }
 if(!applied)fail(s);
}
