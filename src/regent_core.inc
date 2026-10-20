// Complete single-player Regent pool, locked to v0.111.0 / 41cef1ea.
// Original descriptions and implementation disassembly are not embedded here.
struct RegentVariable { CardKind kind;const char*key;int16_t base,upgrade; };
static const RegentVariable regent_variables[]={
 {RegentAlignment,"Energy",2,1},
 {RegentArsenal,"ArsenalPower",1,0},
 {RegentAstralPulse,"Damage",6,2},
 {RegentBeatIntoShape,"Damage",5,2},
 {RegentBeatIntoShape,"CalculationBase",5,2},
 {RegentBeatIntoShape,"CalculationExtra",5,2},
 {RegentBigBang,"Cards",1,0},
 {RegentBigBang,"Energy",1,0},
 {RegentBigBang,"Stars",1,0},
 {RegentBigBang,"Forge",5,0},
 {RegentBlackHole,"BlackHolePower",3,1},
 {RegentBombardment,"Damage",18,6},
 {RegentBulwark,"Block",12,3},
 {RegentBulwark,"Forge",10,3},
 {RegentBundleOfJoy,"Cards",3,1},
 {RegentCelestialMight,"Damage",6,0},
 {RegentCelestialMight,"Repeat",3,1},
 {RegentCharge,"Cards",2,0},
 {RegentChildOfTheStars,"BlockForStars",2,1},
 {RegentCloakOfStars,"Block",7,3},
 {RegentCollisionCourse,"Damage",10,4},
 {RegentComet,"Damage",33,11},
 {RegentComet,"VulnerablePower",3,0},
 {RegentComet,"WeakPower",3,0},
 {RegentConqueror,"Forge",3,2},
 {RegentConvergence,"Energy",1,0},
 {RegentConvergence,"Stars",1,1},
 {RegentCosmicIndifference,"Block",6,3},
 {RegentCrashLanding,"Damage",21,5},
 {RegentCrescentSpear,"CalculationBase",8,0},
 {RegentCrescentSpear,"ExtraDamage",2,1},
 {RegentCrushUnder,"Damage",8,1},
 {RegentCrushUnder,"StrengthLoss",1,1},
 {RegentDecisionsDecisions,"Cards",3,2},
 {RegentDecisionsDecisions,"Repeat",3,0},
 {RegentDefend,"Block",5,3},
 {RegentDevastate,"Damage",35,10},
 {RegentDyingStar,"Damage",9,2},
 {RegentDyingStar,"StrengthLoss",9,2},
 {RegentFallingStar,"Damage",8,4},
 {RegentFallingStar,"VulnerablePower",1,0},
 {RegentFallingStar,"WeakPower",1,0},
 {RegentForegoneConclusion,"Cards",2,1},
 {RegentFurnace,"Forge",5,2},
 {RegentGammaBlast,"Damage",13,5},
 {RegentGammaBlast,"VulnerablePower",2,0},
 {RegentGammaBlast,"WeakPower",2,0},
 {RegentGatherLight,"Block",8,3},
 {RegentGatherLight,"Stars",1,0},
 {RegentGenesis,"StarsPerTurn",2,1},
 {RegentGlimmer,"Cards",3,1},
 {RegentGlimmer,"PutBack",1,0},
 {RegentGlitterstream,"Block",11,2},
 {RegentGlitterstream,"BlockNextTurn",5,2},
 {RegentGlow,"Stars",1,1},
 {RegentGlow,"Cards",1,0},
 {RegentGuidingStar,"Damage",12,1},
 {RegentGuidingStar,"Cards",2,1},
 {RegentHeavenlyDrill,"Damage",8,2},
 {RegentHeavenlyDrill,"Energy",4,0},
 {RegentHegemony,"Damage",15,3},
 {RegentHegemony,"Energy",2,1},
 {RegentHeirloomHammer,"Damage",20,5},
 {RegentHeirloomHammer,"Repeat",1,0},
 {RegentHiddenCache,"Stars",1,0},
 {RegentHiddenCache,"StarNextTurnPower",3,1},
 {RegentIAmInvincible,"Block",10,3},
 {RegentKinglyKick,"Damage",27,8},
 {RegentKinglyPunch,"Damage",8,2},
 {RegentKinglyPunch,"Increase",4,2},
 {RegentKnockoutBlow,"Damage",30,8},
 {RegentKnockoutBlow,"Stars",5,0},
 {RegentKnowThyPlace,"WeakPower",1,0},
 {RegentKnowThyPlace,"VulnerablePower",1,0},
 {RegentLunarBlast,"Damage",4,1},
 {RegentLunarBlast,"CalculationBase",0,0},
 {RegentLunarBlast,"CalculationExtra",1,0},
 {RegentMakeItSo,"Damage",6,3},
 {RegentMakeItSo,"Cards",3,0},
 {RegentManifestAuthority,"Block",7,1},
 {RegentMeteorShower,"Damage",14,7},
 {RegentMeteorShower,"VulnerablePower",2,0},
 {RegentMeteorShower,"WeakPower",2,0},
 {RegentMonarchsGaze,"StrengthLoss",1,0},
 {RegentMonologue,"Power",1,0},
 {RegentNeutronAegis,"PlatingPower",8,3},
 {RegentOrbit,"Energy",1,0},
 {RegentPaleBlueDot,"Cards",1,1},
 {RegentPaleBlueDot,"CardPlay",5,0},
 {RegentParry,"ParryPower",10,4},
 {RegentParticleWall,"Block",9,3},
 {RegentPatter,"Block",8,2},
 {RegentPatter,"VigorPower",2,1},
 {RegentPhotonCut,"Damage",10,3},
 {RegentPhotonCut,"Cards",1,1},
 {RegentPhotonCut,"PutBack",1,0},
 {RegentPillarOfCreation,"Block",2,1},
 {RegentProphesize,"Cards",6,3},
 {RegentRadiate,"Damage",3,1},
 {RegentRadiate,"Stars",1,0},
 {RegentRadiate,"CalculationBase",0,0},
 {RegentRadiate,"CalculationExtra",1,0},
 {RegentRefineBlade,"Forge",8,4},
 {RegentRefineBlade,"Energy",1,0},
 {RegentReflect,"Block",15,5},
 {RegentResonance,"StrengthPower",1,1},
 {RegentRoyalGamble,"Stars",9,0},
 {RegentRoyalties,"Gold",30,10},
 {RegentSeekingEdge,"Forge",7,4},
 {RegentSevenStars,"Damage",7,0},
 {RegentSevenStars,"Repeat",7,0},
 {RegentShiningStrike,"Damage",8,3},
 {RegentShiningStrike,"Stars",2,0},
 {RegentSolarStrike,"Damage",9,1},
 {RegentSolarStrike,"Stars",1,1},
 {RegentSpectrumShift,"Cards",1,0},
 {RegentSpoilsOfBattle,"Forge",6,3},
 {RegentSpoilsOfBattle,"Cards",2,0},
 {RegentStardust,"Damage",5,2},
 {RegentStrike,"Damage",6,3},
 {RegentSummonForth,"Forge",8,3},
 {RegentSupermassive,"CalculationBase",5,0},
 {RegentSupermassive,"ExtraDamage",3,1},
 {RegentSwordSage,"SwordSagePower",1,0},
 {RegentTerraforming,"VigorPower",7,3},
 {RegentTheSmith,"Forge",30,10},
 {RegentVenerate,"Stars",2,1},
 {RegentVoidForm,"VoidFormPower",2,0},
 {RegentWroughtInWar,"Damage",7,2},
 {RegentWroughtInWar,"Forge",7,2},
};
static bool regent_word(const char*a,const char*b){while(*a&&*a==*b){++a;++b;}return *a==*b;}
static int rvar(Card c,const char*key){for(const auto&v:regent_variables)if(v.kind==c.kind&&regent_word(v.key,key))return v.base+(c.upgraded?v.upgrade:0);return 0;}
static int rpower(const State&s,RPower p){return s.regent.powers[(unsigned)p];}
static bool is_regent_card(unsigned k){return k>=RegentAlignment&&k<=RegentWroughtInWar;}
static int regent_base_star_cost(Card c){
 switch(c.kind){
 case RegentAlignment:return 2;case RegentAstralPulse:return 3;case RegentCloakOfStars:return 1;
 case RegentComet:return 5;case RegentCrescentSpear:return 1;case RegentDecisionsDecisions:return 6;
 case RegentDevastate:return 4;case RegentDyingStar:return 3;case RegentFallingStar:return 2;
 case RegentGammaBlast:return 3;case RegentGuidingStar:return 1;case RegentMeteorShower:return 2;
 case RegentNeutronAegis:return 5;case RegentParticleWall:return 2;case RegentQuasar:return 2;
 case RegentReflect:return 3;case RegentResonance:return 2;case RegentRoyalGamble:return 5;
 case RegentSevenStars:return 7;case RegentStardust:return -1;case RegentTheSealedThrone:return 3;
 case RegentTheSmith:return 4;default:return 0;
 }
}
static bool regent_void_free(const State&s,int id){
 return id>=0&&id<s.card_count&&(s.zones[id]==Zone::Hand||s.zones[id]==Zone::Limbo)&&
        s.regent.void_played_turn<rpower(s,RPower::VoidForm);
}
int card_star_cost(const State&s,Card c,int id){
 int n=regent_base_star_cost(c);if(n<0)return n;
 if(id>=0&&id<s.card_count&&s.cards[id].kind==c.kind&&
    (s.expansion.star_turn_free[id]||s.expansion.star_combat_free[id]||regent_void_free(s,id)))return 0;
 return wr_star_cost(s,c,id,n);
}
static int regent_cost(const State&s,int id,int price){
 if(id<0||id>=s.card_count||price<0)return price;
 const auto&r=s.regent;
 if(s.cards[id].kind==RegentKinglyKick){int origin=s.expansion.turn_cost[id]?r.kick_turn_origin[id]:s.expansion.combat_cost[id]?r.kick_combat_origin[id]:0;price=max(0,price-(r.kick_discount[id]-origin));}
 return regent_void_free(s,id)?0:price;
}
static void regent_card_reset(State&s,int id){
 if(id<0||id>=MaxCards)return;auto&r=s.regent;r.kick_discount[id]=r.kick_turn_origin[id]=r.kick_combat_origin[id]=0;r.punch_damage[id]=0;
}
static void regent_card_created(State&s,int id){regent_card_reset(s,id);}
static void regent_card_copied(State&s,int to,int from){
 if(to<0||from<0||to>=s.card_count||from>=s.card_count)return;auto&r=s.regent;
 r.kick_discount[to]=r.kick_discount[from];r.kick_turn_origin[to]=r.kick_turn_origin[from];r.kick_combat_origin[to]=r.kick_combat_origin[from];r.punch_damage[to]=r.punch_damage[from];
}
static void regent_temporary_cost_set(State&s,int id){if(id>=0&&id<s.card_count&&s.expansion.turn_cost[id])s.regent.kick_turn_origin[id]=s.regent.kick_discount[id];}
static void regent_combat_cost_set(State&s,int id){if(id>=0&&id<s.card_count&&s.expansion.combat_cost[id])s.regent.kick_combat_origin[id]=s.regent.kick_discount[id];}
static void regent_after_draw(State&s,int id){
 if(id<0||id>=s.card_count)return;auto&r=s.regent;Card c=s.cards[id];
 if(c.kind==RegentKinglyKick)r.kick_discount[id]=(uint16_t)min(9999,r.kick_discount[id]+1);
 if(c.kind==RegentKinglyPunch)r.punch_damage[id]=(int16_t)min(9999,r.punch_damage[id]+rvar(c,"Increase"));
}
static int regent_base_damage(const State&s,Card c,int,int id,int base){
 if(c.kind==RegentCrescentSpear){int count=0;for(unsigned n=0;n<s.card_count;n++)if(regent_base_star_cost(s.cards[n]))++count;return min(9999,8+rvar(c,"ExtraDamage")*count);}
 if(c.kind==RegentSupermassive)return min(9999,5+rvar(c,"ExtraDamage")*s.regent.cards_created);
 if(c.kind==RegentKinglyPunch&&id>=0&&id<s.card_count)return min(9999,base+s.regent.punch_damage[id]);
 return base;
}
static int regent_damage_multiplier(const State&s,Card c,int enemy){return c.kind==SovereignBlade&&enemy>=0&&enemy<s.enemy_count&&s.regent.conqueror[enemy]>0?2:1;}
static int regent_blade_block(const State&s){return rpower(s,RPower::Parry);}
static int regent_replays(const State&s,Card c){return c.kind==SovereignBlade?rpower(s,RPower::SwordSage):0;}
static bool regent_targets_enemy(const State&s,Card c,bool previous){return c.kind==SovereignBlade&&rpower(s,RPower::SeekingEdge)?false:previous;}
static bool regent_returns_to_hand(const State&s,int id){return id>=0&&id<s.card_count&&s.cards[id].kind==RegentParticleWall&&!s.expansion.returning;}
static unsigned regent_actor(RPower p){
 if(p==RPower::MonarchsGaze)return(unsigned)PostPlayActor::RegentMonarchsGaze;
 if(p==RPower::Arsenal)return(unsigned)PostPlayActor::RegentArsenal;
 if(p==RPower::PillarOfCreation)return(unsigned)PostPlayActor::RegentPillarOfCreation;
 if(p==RPower::BlackHole)return(unsigned)PostPlayActor::RegentBlackHole;
 if(p==RPower::PaleBlueDot)return(unsigned)PostPlayActor::RegentPaleBlueDot;
 if(p==RPower::VoidForm)return(unsigned)PostPlayActor::RegentVoidForm;
 return 0;
}
static void regent_gain_power(State&s,RPower p,int amount){
 auto&r=s.regent;unsigned k=(unsigned)p;int old=r.powers[k],value=max(0,min(9999,old+amount));
 if(p==RPower::SeekingEdge)value=min(1,value);
 if(amount>0&&(p==RPower::Orbit||p==RPower::Monologue)){
  int16_t*slots=p==RPower::Orbit?r.orbit:r.monologue;int at=0;while(at<MaxPowerInstances&&slots[at])++at;
  if(at==MaxPowerInstances){fail(s);return;}slots[at]=(int16_t)amount;
  if(p==RPower::Monologue)after_play_register(s,(unsigned)PostPlayActor::RegentMonologueStart+at);
 }
 if(!old&&value){if(r.power_count==RPowerCount){fail(s);return;}r.power_order[r.power_count++]=(uint8_t)k;}
 if(old&&!value){for(unsigned at=0;at<r.power_count;at++)if(r.power_order[at]==k){for(unsigned j=at+1;j<r.power_count;j++)r.power_order[j-1]=r.power_order[j];r.power_order[--r.power_count]=0;break;}}
 r.powers[k]=(int16_t)value;unsigned actor=regent_actor(p);
 if(p==RPower::VoidForm&&amount>0)r.void_played_turn=9999;
 if(actor&&!old&&value)after_play_register(s,actor);if(actor&&old&&!value)after_play_unregister(s,actor);
}
void regent_gain_stars(State&s,int amount){
 if(amount<=0||!combat_phase(s.phase)||s.hp<=0)return;
 amount=min(amount,9999-s.expansion.stars);if(amount<=0)return;
 s.expansion.stars+=(int16_t)amount;s.regent.stars_gained_turn=(uint16_t)min(9999,s.regent.stars_gained_turn+amount);
 if(rpower(s,RPower::BlackHole))queue_extension(s,XOp::AllDamage,rpower(s,RPower::BlackHole),0,true);
}
static void regent_card_generated(State&s,int id){
 if(id<0||id>=s.card_count||!combat_phase(s.phase))return;auto&r=s.regent;r.cards_created=(uint16_t)min(9999,r.cards_created+1);
 for(int at=r.power_count-1;at>=0;--at){RPower p=(RPower)r.power_order[at];
  if(p==RPower::Arsenal)enqueue(s,EffectKind::Strength,rpower(s,p),0,true);
  if(p==RPower::PillarOfCreation)queue_extension(s,XOp::RUnpoweredBlock,rpower(s,p),0,true);
 }
}
static void regent_energy_spent(State&s,int energy_spent){
 auto&r=s.regent;if(energy_spent>0)for(int at=MaxPowerInstances-1;at>=0;--at)if(r.orbit[at]){
  int amount=r.orbit_remainder[at]+energy_spent;int triggers=amount/4;r.orbit_remainder[at]=(uint8_t)(amount%4);
  if(triggers)enqueue(s,EffectKind::Energy,min(999,r.orbit[at]*triggers),0,true);
 }
}
static void regent_stars_spent(State&s,int stars_spent){
 if(stars_spent>0&&rpower(s,RPower::ChildOfTheStars))queue_extension(s,XOp::RUnpoweredBlock,min(9999,stars_spent*rpower(s,RPower::ChildOfTheStars)),0,true);
}
static void regent_begin_play(State&s,int id,bool automatic,int energy_spent,int stars_spent,int captured_star_x){
 auto&r=s.regent;unsigned depth=s.expansion.depth;r.automatic[depth]=(uint8_t)automatic;r.stars_spent[depth]=(int16_t)stars_spent;
 r.star_x[depth]=(int16_t)(automatic?(captured_star_x>=0?captured_star_x:s.expansion.stars):stars_spent);
 if(id>=0&&id<s.card_count&&regent_base_star_cost(s.cards[id])<0)r.star_x[depth]=(int16_t)wr_x_value(s,r.star_x[depth]);
 // Front insertion preserves energy callbacks before star callbacks.
 regent_stars_spent(s,stars_spent);regent_energy_spent(s,energy_spent);
}
static void regent_before_card(State&s,Card){
 auto&r=s.regent;unsigned depth=s.expansion.depth;
 for(unsigned at=0;at<MaxPowerInstances;at++)r.monologue_capture[at][depth]=r.monologue[at];
 if(rpower(s,RPower::TheSealedThrone))queue_extension(s,XOp::RStars,rpower(s,RPower::TheSealedThrone));
}
static void regent_after_card(State&s,Card c){
 auto&r=s.regent;r.cards_finished_turn=(uint16_t)min(9999,r.cards_finished_turn+1);
 if(card_type(s,c)==CardType::Skill)r.skills_finished_turn=(uint16_t)min(9999,r.skills_finished_turn+1);
}
static bool regent_post_play_actor_active(const State&s,unsigned actor){
 if(actor==(unsigned)PostPlayActor::RegentMonarchsGaze)return rpower(s,RPower::MonarchsGaze)>0;
 if(actor==(unsigned)PostPlayActor::RegentArsenal)return rpower(s,RPower::Arsenal)>0;
 if(actor==(unsigned)PostPlayActor::RegentPillarOfCreation)return rpower(s,RPower::PillarOfCreation)>0;
 if(actor==(unsigned)PostPlayActor::RegentBlackHole)return rpower(s,RPower::BlackHole)>0;
 if(actor==(unsigned)PostPlayActor::RegentPaleBlueDot)return rpower(s,RPower::PaleBlueDot)>0;
 if(actor==(unsigned)PostPlayActor::RegentVoidForm)return rpower(s,RPower::VoidForm)>0;
 return actor>=(unsigned)PostPlayActor::RegentMonologueStart&&actor<(unsigned)PostPlayActor::RegentMonologueStart+MaxPowerInstances&&s.regent.monologue[actor-(unsigned)PostPlayActor::RegentMonologueStart]>0;
}
static void regent_after_play_listener(State&s,unsigned actor){
 auto&r=s.regent;unsigned d=s.expansion.depth;
 if(actor==(unsigned)PostPlayActor::RegentBlackHole){if(!s.expansion.remaining&&r.stars_spent[d]>0&&rpower(s,RPower::BlackHole))queue_extension(s,XOp::AllDamage,rpower(s,RPower::BlackHole),0,true);return;}
 if(actor==(unsigned)PostPlayActor::RegentPaleBlueDot){if(!r.pale_activated&&r.cards_finished_turn>=5&&rpower(s,RPower::PaleBlueDot)){r.pale_activated=1;s.draw_next_turn=(int16_t)min(MaxCards,s.draw_next_turn+rpower(s,RPower::PaleBlueDot));}return;}
 if(actor==(unsigned)PostPlayActor::RegentVoidForm){if(!r.automatic[d]&&!s.expansion.remaining)r.void_played_turn=(uint16_t)min(9999,r.void_played_turn+1);return;}
 if(actor>=(unsigned)PostPlayActor::RegentMonologueStart&&actor<(unsigned)PostPlayActor::RegentMonologueStart+MaxPowerInstances){
  int at=actor-(unsigned)PostPlayActor::RegentMonologueStart;int amount=r.monologue_capture[at][d];r.monologue_capture[at][d]=0;
  if(amount)queue_extension(s,XOp::RMonologueStrength,at,amount,true);
 }
}
static void regent_return_to_hand(State&s,int id){
 if(id<0||id>=s.card_count||s.zones[id]==Zone::Hand)return;
 Zone z=s.zones[id];Pile*from=z==Zone::Draw?&s.draw:z==Zone::Discard?&s.discard:z==Zone::Exhaust?&s.exhaust:z==Zone::Power?&s.powers:nullptr;
 bool room=s.hand.count<MaxHand;
 if(from)move_instance(s,id,*from,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard);
 else if(z==Zone::Limbo)put(s,room?s.hand:s.discard,id,room?Zone::Hand:Zone::Discard);
}
static void regent_after_play_late(State&s,Card c){
 if(card_type(s,c)!=CardType::Skill||!s.regent.skills_finished_turn||s.regent.skills_finished_turn%3)return;
 for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==RegentMakeItSo)regent_return_to_hand(s,id);
}
static void regent_after_play_group(State&s,int){
 auto&r=s.regent;unsigned d=s.expansion.depth;r.stars_spent[d]=r.star_x[d]=0;r.automatic[d]=0;
 for(unsigned at=0;at<MaxPowerInstances;at++)r.monologue_capture[at][d]=0;
}
static void regent_after_damage(State&s,int enemy,bool powered){
 if(!powered||enemy<0||enemy>=s.enemy_count)return;auto&r=s.regent;r.hits_turn[enemy]=(uint16_t)min(9999,r.hits_turn[enemy]+1);
}
static void regent_after_blocked_attack(State&s,int enemy,int amount){
 if(amount>0&&enemy>=0&&enemy<s.enemy_count&&s.enemies[enemy].hp&&rpower(s,RPower::Reflect))queue_extension(s,XOp::RUnpoweredDamage,amount,enemy,true);
}
static void regent_turn_start(State&s){
 auto&r=s.regent;r.stars_gained_turn=r.cards_finished_turn=r.skills_finished_turn=r.void_played_turn=0;r.pale_activated=0;
 for(auto&n:r.hits_turn)n=0;if(rpower(s,RPower::Reflect))regent_gain_power(s,RPower::Reflect,-1);
}
static void regent_after_energy_reset(State&s){
 const auto&r=s.regent;for(int at=r.power_count-1;at>=0;--at){RPower p=(RPower)r.power_order[at];
  if(p==RPower::Genesis||p==RPower::StarsNextTurn||p==RPower::EnergyNextTurn)queue_extension(s,XOp::RStartPower,(int)p,0,true);
 }
}
static void regent_auto_preplay(State&s){queue_extension(s,XOp::RBombardments,0,0,true);}
static void regent_before_hand_draw(State&s){
 const auto&r=s.regent;for(int at=r.power_count-1;at>=0;--at){RPower p=(RPower)r.power_order[at];
  if(p==RPower::ForegoneConclusion||p==RPower::SpectrumShift)queue_extension(s,XOp::RStartPower,(int)p,0,true);
 }
}
static int regent_hand_draw_bonus(const State&s){return rpower(s,RPower::Tyranny);}
static void regent_after_hand_draw(State&s){
 // AfterSideTurnStart precedes AfterPlayerTurnStart; Tyranny's bonus has
 // already modified the one normal hand draw before its exhaust choice.
 if(rpower(s,RPower::Tyranny))queue_extension(s,XOp::RTyranny,rpower(s,RPower::Tyranny),0,true);
 if(rpower(s,RPower::Furnace))queue_extension(s,XOp::Forge,rpower(s,RPower::Furnace),0,true);
}
static void regent_turn_end(State&s){queue_extension(s,XOp::RInvincible,0,0,true);}
static void regent_player_side_end(State&s){
 auto&r=s.regent;for(unsigned at=0;at<MaxPowerInstances;at++)if(r.monologue[at]){
  s.strength=(int16_t)max(-9999,s.strength-r.monologue_applied[at]);r.monologue[at]=r.monologue_applied[at]=0;
  for(auto&n:r.monologue_capture[at])n=0;after_play_unregister(s,(unsigned)PostPlayActor::RegentMonologueStart+at);
 }
 if(rpower(s,RPower::Monologue))regent_gain_power(s,RPower::Monologue,-rpower(s,RPower::Monologue));
 r.pale_activated=0;
}
static void regent_enemy_side_end(State&s){for(auto&v:s.regent.conqueror)if(v)--v;}
static void regent_combat_end(State&s,bool won){
 auto&r=s.regent;if(won&&rpower(s,RPower::Royalties)){
  if(s.world.active)s.world.combat_bonus_gold=uint16_t(min(9999,int(s.world.combat_bonus_gold)+rpower(s,RPower::Royalties)));
  else gain_gold(s,rpower(s,RPower::Royalties));
 }
 r.end_turn_requested=0;for(unsigned d=0;d<=MaxAutoFrames;d++){r.stars_spent[d]=r.star_x[d]=0;r.automatic[d]=0;for(unsigned at=0;at<MaxPowerInstances;at++)r.monologue_capture[at][d]=0;}
}

static bool regent_choice(XChoice k){return k>=XChoice::RTransformHand&&k<=XChoice::RTyranny;}
static bool choose_regent(State&s,const uint16_t*ids,unsigned n){
 auto&x=s.expansion;XChoice kind=x.choice;
 if(s.phase!=Phase::ExtendedChoice||!regent_choice(kind)||n<x.choice_min||n>x.choice_max||n>MaxHand||(n&&!ids))return false;
 unsigned queued=(kind==XChoice::RDecisions)?3:n;
 if(s.effect_count+queued>MaxEffects||(kind==XChoice::RHeirloom&&s.card_count+n>MaxCards))return false;
 uint16_t selected[MaxHand];
 for(unsigned i=0;i<n;i++){
  int id=ids[i];bool found=false;for(unsigned j=0;j<x.choices.count;j++)if(x.choices.ids[j]==id)found=true;
  if(!found||id<0||id>=s.card_count)return false;for(unsigned j=0;j<i;j++)if(selected[j]==id)return false;
  Zone expected=(kind==XChoice::RTransformDraw||kind==XChoice::RForegone)?Zone::Draw:kind==XChoice::RDiscardTop?Zone::Discard:Zone::Hand;
  if(s.zones[id]!=expected)return false;
  if(kind==XChoice::RDecisions&&card_type(s,s.cards[id],id)!=CardType::Skill)return false;
  if(kind==XChoice::RHeirloom&&!card_colorless(s.cards[id].kind))return false;
  selected[i]=(uint16_t)id;
 }
 bool upgraded=x.choice_free;clear_extended_choice(s);s.phase=Phase::Combat;
 if(kind==XChoice::RDecisions&&n){for(unsigned i=0;i<3;i++)queue_extension(s,XOp::AutoInstance,selected[0],0,true);}
 else for(unsigned i=n;i>0;i--){int id=selected[i-1];
  if(kind==XChoice::RTransformHand||kind==XChoice::RTransformDraw||kind==XChoice::RTransformGuards){
   CardKind token=kind==XChoice::RTransformHand?MinionStrike:kind==XChoice::RTransformDraw?MinionDiveBomb:MinionSacrifice;
   queue_extension(s,XOp::RTransformCard,id,card_token({token,(uint8_t)upgraded}),true);
  }else if(kind==XChoice::RHeirloom)queue_extension(s,XOp::RClone,id,0,true);
  else if(kind==XChoice::RTyranny)enqueue(s,EffectKind::ExhaustCard,id,0,true);
  else if(kind==XChoice::RDiscardTop)move_instance(s,id,s.discard,s.draw,Zone::Draw);
  else if(kind==XChoice::RForegone){bool room=s.hand.count<MaxHand;move_instance(s,id,s.draw,room?s.hand:s.discard,room?Zone::Hand:Zone::Discard);}
 }
 resolve_pending(s);++s.revision;return true;
}

static bool queue_regent(State&s,Card c,int target,int energy){
 if(!is_regent_card(c.kind)&&c.kind!=SovereignBlade&&c.kind!=MinionStrike&&c.kind!=MinionDiveBomb&&c.kind!=MinionSacrifice&&c.kind!=Debris)return false;
 const auto&d=definitions[c.kind];int hit=card_base_damage(s,c,target,s.playing),block=d.block+(c.upgraded?d.block_up:0);
 record(s,LogKind::Play,c.kind,c.upgraded);if(d.type==CardType::Attack)relic_attack_begin(s);
 auto op=[&](XOp k,int a=0,int b=0){queue_extension(s,k,a,b);};
 auto pw=[&](RPower p,int n){op(XOp::RPower,(int)p,n);};
 auto attack=[&](){enqueue(s,EffectKind::Damage,hit,target);};
 auto all=[&](int times=1){op(XOp::RAllHits,hit,times);};
 auto defend=[&](){op(XOp::RBlock,block);};
 auto stars=[&](){op(XOp::RStars,rvar(c,"Stars"));};
 auto forge=[&](){op(XOp::Forge,rvar(c,"Forge"));};
 auto debuffs=[&](bool every=false){int lo=every?0:target,hi=every?s.enemy_count:target+1;for(int t=lo;t<hi;t++)enqueue(s,EffectKind::Weak,rvar(c,"WeakPower"),t);for(int t=lo;t<hi;t++)enqueue(s,EffectKind::Vulnerable,rvar(c,"VulnerablePower"),t);};
 switch(c.kind){
 case RegentAlignment:enqueue(s,EffectKind::Energy,rvar(c,"Energy"));break;
 case RegentArsenal:pw(RPower::Arsenal,rvar(c,"ArsenalPower"));break;
 case RegentAstralPulse:all(2);break;
 case RegentBeatIntoShape:{int prior=target>=0&&target<s.enemy_count?s.regent.hits_turn[target]:0;attack();op(XOp::RForgeHits,min(9999,rvar(c,"CalculationBase")+rvar(c,"CalculationExtra")*prior));break;}
 case RegentBegone:op(XOp::RTransform,0,c.upgraded);break;
 case RegentBigBang:op(XOp::Draw,rvar(c,"Cards"));stars();enqueue(s,EffectKind::Energy,rvar(c,"Energy"));forge();break;
 case RegentBlackHole:pw(RPower::BlackHole,rvar(c,"BlackHolePower"));break;
 case RegentBombardment:case RegentCrescentSpear:case RegentDevastate:case RegentKinglyKick:case RegentKinglyPunch:case RegentMakeItSo:case RegentStrike:case RegentSupermassive:case MinionDiveBomb:attack();break;
 case RegentBulwark:defend();forge();break;
 case RegentBundleOfJoy:op(XOp::Generate,4,rvar(c,"Cards")<<2);break;
 case RegentCelestialMight:op(XOp::RHits,hit,rvar(c,"Repeat"));break;
 case RegentCharge:op(XOp::RTransform,1,c.upgraded);break;
 case RegentChildOfTheStars:pw(RPower::ChildOfTheStars,rvar(c,"BlockForStars"));break;
 case RegentCloakOfStars:case RegentDefend:case RegentIAmInvincible:case RegentParticleWall:case MinionSacrifice:defend();break;
 case RegentCollisionCourse:attack();op(XOp::RDebris,1);break;
 case RegentComet:case RegentFallingStar:case RegentGammaBlast:attack();debuffs();break;
 case RegentConqueror:forge();op(XOp::RConqueror,target,1);break;
 case RegentConvergence:op(XOp::Power,(int)XPower::Retain,1);pw(RPower::EnergyNextTurn,rvar(c,"Energy"));pw(RPower::StarsNextTurn,rvar(c,"Stars"));break;
 case RegentCosmicIndifference:defend();op(XOp::RDiscardTop);break;
 case RegentCrashLanding:all();op(XOp::RDebris,0);break;
 case RegentCrushUnder:case RegentDyingStar:
  all();for(int t=0;t<s.enemy_count;t++)op(XOp::NTempStrength,(int)(c.kind==RegentCrushUnder?NTempStrength::CrushUnder:NTempStrength::DyingStar)*MaxEnemies+t,rvar(c,"StrengthLoss"));break;
 case RegentDecisionsDecisions:op(XOp::Draw,rvar(c,"Cards"));op(XOp::RDecisions);break;
 case RegentForegoneConclusion:pw(RPower::ForegoneConclusion,rvar(c,"Cards"));break;
 case RegentFurnace:pw(RPower::Furnace,rvar(c,"Forge"));break;
 case RegentGatherLight:defend();stars();break;
 case RegentGenesis:pw(RPower::Genesis,rvar(c,"StarsPerTurn"));break;
 case RegentGlimmer:op(XOp::Draw,rvar(c,"Cards"));op(XOp::ChooseTop);break;
 case RegentGlitterstream:op(XOp::RBlock,block,rvar(c,"BlockNextTurn"));break;
 case RegentGlow:stars();op(XOp::Draw,rvar(c,"Cards"));enqueue(s,EffectKind::DrawNextTurn,rvar(c,"Cards"));break;
 case RegentGuards:op(XOp::RTransform,2,c.upgraded);break;
 case RegentGuidingStar:attack();enqueue(s,EffectKind::DrawNextTurn,rvar(c,"Cards"));break;
 case RegentHeavenlyDrill:op(XOp::RHits,hit,min(9999,energy*(energy>=rvar(c,"Energy")?2:1)));break;
 case RegentHegemony:attack();pw(RPower::EnergyNextTurn,rvar(c,"Energy"));break;
 case RegentHeirloomHammer:attack();op(XOp::RHeirloom);break;
 case RegentHiddenCache:stars();pw(RPower::StarsNextTurn,rvar(c,"StarNextTurnPower"));break;
 case RegentKnockoutBlow:attack();op(XOp::RKnockoutReward,target,rvar(c,"Stars"));break;
 case RegentKnowThyPlace:debuffs();break;
 case RegentLunarBlast:op(XOp::RHits,hit,s.regent.skills_finished_turn);break;
 case RegentManifestAuthority:defend();op(XOp::Generate,4,c.upgraded?2:0);break;
 case RegentMeteorShower:all();debuffs(true);break;
 case RegentMonarchsGaze:pw(RPower::MonarchsGaze,rvar(c,"StrengthLoss"));break;
 case RegentMonologue:pw(RPower::Monologue,rvar(c,"Power"));break;
 case RegentNeutronAegis:op(XOp::Power,(int)XPower::Plating,rvar(c,"PlatingPower"));break;
 case RegentOrbit:pw(RPower::Orbit,rvar(c,"Energy"));break;
 case RegentPaleBlueDot:pw(RPower::PaleBlueDot,rvar(c,"Cards"));break;
 case RegentParry:pw(RPower::Parry,rvar(c,"ParryPower"));break;
 case RegentPatter:defend();op(XOp::RVigor,rvar(c,"VigorPower"));break;
 case RegentPhotonCut:attack();op(XOp::Draw,rvar(c,"Cards"));op(XOp::ChooseTop);break;
 case RegentPillarOfCreation:pw(RPower::PillarOfCreation,rvar(c,"Block"));break;
 case RegentProphesize:op(XOp::Draw,rvar(c,"Cards"));break;
 case RegentQuasar:op(XOp::Offer,4,c.upgraded?2:0);break;
 case RegentRadiate:all(-1);break;
 case RegentRefineBlade:forge();pw(RPower::EnergyNextTurn,rvar(c,"Energy"));break;
 case RegentReflect:defend();pw(RPower::Reflect,1);break;
 case RegentResonance:enqueue(s,EffectKind::Strength,rvar(c,"StrengthPower"));for(int t=0;t<s.enemy_count;t++)op(XOp::RStrengthLoss,t,1);break;
 case RegentRoyalGamble:case RegentVenerate:stars();break;
 case RegentRoyalties:pw(RPower::Royalties,rvar(c,"Gold"));break;
 case RegentSeekingEdge:pw(RPower::SeekingEdge,1);forge();break;
 case RegentSevenStars:all(rvar(c,"Repeat"));break;
 case RegentShiningStrike:attack();stars();break;
 case RegentSolarStrike:attack();stars();break;
 case RegentSpectrumShift:pw(RPower::SpectrumShift,rvar(c,"Cards"));break;
 case RegentSpoilsOfBattle:forge();op(XOp::Draw,rvar(c,"Cards"));break;
 case RegentStardust:op(XOp::RandomHits,hit,s.regent.star_x[s.expansion.depth]);break;
 case RegentSummonForth:op(XOp::RReturnBlade);forge();break;
 case RegentSwordSage:pw(RPower::SwordSage,rvar(c,"SwordSagePower"));break;
 case RegentTerraforming:op(XOp::RVigor,rvar(c,"VigorPower"));break;
 case RegentTheSealedThrone:pw(RPower::TheSealedThrone,1);break;
 case RegentTheSmith:forge();break;
 case RegentTyranny:pw(RPower::Tyranny,1);break;
 case RegentVoidForm:pw(RPower::VoidForm,rvar(c,"VoidFormPower"));op(XOp::REndTurn);break;
 case RegentWroughtInWar:attack();forge();break;
 case SovereignBlade:if(rpower(s,RPower::SeekingEdge))all();else attack();if(regent_blade_block(s))op(XOp::RBlock,regent_blade_block(s));break;
 case MinionStrike:attack();op(XOp::Draw,1);break;
 case Debris:break;
 default:return false;
 }
 enqueue(s,EffectKind::Finish);return true;
}

static bool regent_effect(State&s,Effect e){
 int a=e.value,b=e.aux;auto&r=s.regent;auto&x=s.expansion;
 switch((XOp)e.target){
 case XOp::RPower:regent_gain_power(s,(RPower)a,b);break;
 case XOp::RStars:regent_gain_stars(s,a);break;
 case XOp::RBlock:{
  Card c=s.playing>=0?s.cards[s.playing]:Card{RegentDefend,0};
  int next=b?silent_modify_block(s,ironclad_block(s,c,powered_block(s,c,b))):0;
  gain_card_block(s,powered_block(s,c,a));if(next)s.block_next_turn=(int16_t)min(9999,s.block_next_turn+next);break;
 }
 case XOp::RUnpoweredBlock:gain_unpowered_block(s,a);break;
 case XOp::RHits:
  if(b>0&&x.target>=0&&x.target<s.enemy_count&&s.enemies[x.target].hp){if(b>1)queue_extension(s,XOp::RHits,a,b-1,true);enqueue(s,EffectKind::Damage,a,x.target,true);}break;
 case XOp::RAllHits:
  if(b<0)b=r.stars_gained_turn;
  if(b>0&&alive(s)){if(b>1)queue_extension(s,XOp::RAllHits,a,b-1,true);for(int t=s.enemy_count-1;t>=0;t--)if(s.enemies[t].hp)enqueue(s,EffectKind::Damage,a,t,true);}break;
 case XOp::RTransform:
  if(a==1)set_extended_choice(s,XChoice::RTransformDraw,s.draw,2,2,b);
  else set_extended_choice(s,a==2?XChoice::RTransformGuards:XChoice::RTransformHand,s.hand,a==2?0:1,a==2?s.hand.count:1,b);break;
 case XOp::RTransformCard:
  if(a>=0&&a<s.card_count){Card c=card_from_token(b);s.cards[a]=c;reset_card_modifiers(s,a);notify_card_generated(s,a);record(s,LogKind::Create,1,c.kind);}break;
 case XOp::RDecisions:{
  Pile choices{};for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(card_type(s,s.cards[id],id)==CardType::Skill)choices.ids[choices.count++]=(uint16_t)id;}
  set_extended_choice(s,XChoice::RDecisions,choices,1,1);break;
 }
 case XOp::RHeirloom:{
  Pile choices{};for(unsigned i=0;i<s.hand.count;i++){int id=s.hand.ids[i];if(card_colorless(s.cards[id].kind))choices.ids[choices.count++]=(uint16_t)id;}
  set_extended_choice(s,XChoice::RHeirloom,choices,1,1);break;
 }
 case XOp::RClone:
  if(a>=0&&a<s.card_count){int id=create_to_hand(s,s.cards[a]);if(id>=0){copy_card_modifiers(s,id,a);x.return_turn[id]=x.return_turn[a]>=(unsigned)s.turn?x.return_turn[a]:0;}}break;
 case XOp::RDiscardTop:set_extended_choice(s,XChoice::RDiscardTop,s.discard,1,1);break;
 case XOp::RConqueror:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&b>0&&!silent_blocks_debuff(s,a)){
   int added=min(9999-r.conqueror[a],b);r.conqueror[a]+=(int16_t)added;
   if(added){necro_note_debuff(s,a,NDebuff::Conqueror);necro_debuff_applied(s,a,added);}
  }break;
 case XOp::RKnockoutReward:
  if(a>=0&&a<s.enemy_count&&(x.attack_observed[x.depth]?(x.attack_killed[x.depth]&(1U<<a)):!s.enemies[a].hp)&&x.last_damage>0)regent_gain_stars(s,b);break;
 case XOp::RForgeHits:forge_blade(s,a);break;
 case XOp::RStrengthLoss:
  if(a>=0&&a<s.enemy_count&&s.enemies[a].hp&&b>0&&!silent_blocks_debuff(s,a)){
   int lost=min(9999+s.enemies[a].strength,b);s.enemies[a].strength-=(int16_t)lost;
   if(lost){necro_note_debuff(s,a,NDebuff::Strength);necro_prune_debuffs(s);necro_debuff_applied(s,a,lost);}
  }break;
 case XOp::RStartPower:{
  RPower p=(RPower)a;int amount=rpower(s,p);if(!amount)break;
  if(p==RPower::EnergyNextTurn||p==RPower::StarsNextTurn||p==RPower::ForegoneConclusion)queue_extension(s,XOp::RPower,(int)p,-amount,true);
  if(p==RPower::Genesis||p==RPower::StarsNextTurn)regent_gain_stars(s,amount);
  else if(p==RPower::EnergyNextTurn)enqueue(s,EffectKind::Energy,amount,0,true);
  else if(p==RPower::ForegoneConclusion)queue_extension(s,XOp::RForegone,amount,0,true);
  else if(p==RPower::SpectrumShift)for(int n=0;n<amount;n++)queue_extension(s,XOp::Generate,4,0,true);
  break;
 }
 case XOp::RForegone:
  if(!s.draw.count&&s.discard.count&&!b){queue_extension(s,XOp::RForegone,a,1,true);shuffle(s);}
  else set_extended_choice(s,XChoice::RForegone,s.draw,a,a);break;
 case XOp::RTyranny:set_extended_choice(s,XChoice::RTyranny,s.hand,a,a);break;
 case XOp::RBombardments:
  if(!b){for(int id=s.card_count-1;id>=0;id--)if(s.cards[id].kind==RegentBombardment)queue_extension(s,XOp::RBombardments,id,1,true);}
  else if(a>=0&&a<s.card_count&&s.cards[a].kind==RegentBombardment&&s.zones[a]==Zone::Exhaust)start_instance(s,a,-1,true);break;
 case XOp::RInvincible:
  if(!b){for(int id=s.card_count-1;id>=0;id--)if(s.cards[id].kind==RegentIAmInvincible)queue_extension(s,XOp::RInvincible,id,1,true);}
  else if(s.draw.count&&a>=0&&a<s.card_count&&s.cards[a].kind==RegentIAmInvincible&&s.draw.ids[s.draw.count-1]==a)start_instance(s,a,-1,true);break;
 case XOp::RUnpoweredDamage:damage_unpowered(s,b,a);break;
 case XOp::RMonologueStrength:{
  int amount=min(b,min(9999-s.strength,9999-r.monologue_applied[a]));world_relic_gain_strength(s,amount);r.monologue_applied[a]+=(int16_t)amount;break;
 }
 case XOp::REndTurn:r.end_turn_requested=1;break;
 case XOp::RReturnBlade:
  for(unsigned id=0;id<s.card_count;id++)if(s.cards[id].kind==SovereignBlade)regent_return_to_hand(s,id);break;
 case XOp::RVigor:s.vigor=(int16_t)min(9999,s.vigor+a);break;
 case XOp::RDebris:{
  int count=a?a:MaxHand-s.hand.count;if(s.card_count+count>MaxCards){fail(s);break;}
  for(int n=0;n<count;n++)if(create_to_hand(s,{Debris,0})<0)break;break;
 }
 default:return false;
 }
 return true;
}

bool regent_description_value(const State&s,Card c,int target,int instance,const char*name,int&out){
 if(!name||(!is_regent_card(c.kind)&&c.kind!=SovereignBlade&&c.kind!=MinionStrike&&c.kind!=MinionDiveBomb&&c.kind!=MinionSacrifice&&c.kind!=Debris))return false;
 if(regent_word(name,"Damage")||regent_word(name,"CalculatedDamage")){out=damage(s,c,target,instance);return true;}
 if(regent_word(name,"Block")){out=c.kind==RegentPillarOfCreation?rvar(c,"Block"):c.kind==SovereignBlade?silent_modify_block(s,ironclad_block(s,c,powered_block(s,c,regent_blade_block(s)),instance)):card_block(s,c,instance);return true;}
 if(regent_word(name,"CalculatedHits")){
  if(c.kind==RegentLunarBlast)out=s.regent.skills_finished_turn;
  else if(c.kind==RegentRadiate)out=s.regent.stars_gained_turn;
  else if(c.kind==RegentHeavenlyDrill)out=s.energy*(s.energy>=4?2:1);
  else if(c.kind==RegentStardust)out=s.expansion.stars;else return false;return true;
 }
 if(c.kind==RegentBeatIntoShape&&regent_word(name,"CalculatedForge")){int prior=target>=0&&target<s.enemy_count?s.regent.hits_turn[target]:0;out=min(9999,rvar(c,"CalculationBase")+rvar(c,"CalculationExtra")*prior);return true;}
 if(c.kind==RegentGlitterstream&&regent_word(name,"BlockNextTurn")){out=silent_modify_block(s,ironclad_block(s,c,powered_block(s,c,rvar(c,"BlockNextTurn")),instance));return true;}
 if(c.kind==MinionStrike&&regent_word(name,"Cards")){out=1;return true;}
 if(c.kind==SovereignBlade&&regent_word(name,"Repeat")){out=1;return true;}
 for(const auto&v:regent_variables)if(v.kind==c.kind&&regent_word(v.key,name)){out=v.base+(c.upgraded?v.upgrade:0);return true;}
 return false;
}
