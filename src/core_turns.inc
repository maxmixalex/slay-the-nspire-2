void enter_combat(State&s){
    const EncounterId pending_encounter=s.enemy_world.encounter;
    s.expansion=ExpansionState{};
    s.ironclad=IroncladState{};silent_combat_start(s);defect_combat_start(s);s.regent=RegentState{};necro_combat_start(s);
    s.relic_flash=0;relic_reset_turn_counters(s);
    s.phase=Phase::Combat;s.block=s.strength=s.pain=s.embrace=s.dexterity=0;s.double_attack=0;s.power_count=0;s.no_draw=0;s.temporary_strength=s.temporary_dexterity=s.vigor=s.attack_vigor=s.thorns=0;
    s.noxious_fumes=s.accuracy=s.discard_required=0;
    s.no_block=s.exhaust_limit=s.attacked_this_turn=0;s.relic_counters[24]=0;
    s.demon_form=s.infinite_blades=s.block_next_turn=s.draw_next_turn=s.flame_barrier=0;
    s.turn=s.world.active?0:1;s.energy=3;s.playing=-1;s.effect_count=s.card_count=0;
    s.draw.count=s.hand.count=s.discard.count=s.exhaust.count=s.powers.count=0;
    if(s.world.active){
        if(!encounter_setup(s,pending_encounter,s.world.ascension)){fail(s);return;}
    }else{
        enemy_reset_combat(s);
        s.enemy_count=s.encounter?2:1;
        for(int i=0;i<s.enemy_count;i++)s.enemies[i]={(int16_t)(42+random_below(s.rng_world,5)),0,0,0,0,(uint8_t)(s.enemy_count==1?0:(i==0?1:2))};
        for(int i=0;i<s.enemy_count;i++)s.enemies[i].max_hp=s.enemies[i].hp;
    }
    event_card_combat_start(s);
    for(int i=0;i<s.deck_count;i++){int id=make_card(s,s.deck[i]);if(id>=0){notify_deck_card_created(s,id,i);put(s,s.discard,id,Zone::Discard);}}
    shuffle(s,true);
    event_card_combat_start(s);
    if(world_shared_active(s)){
        s.turn=1;world_stage(s,WorldStage::EnterRoom);resolve_pending(s);++s.revision;return;
    }
    relic_hook(s,RelicHook::CombatStart);relic_hook(s,RelicHook::TurnStart);resolve_pending(s);
    if(s.practice&&s.floor==1){
        // Explicit practice opening: lets the player exercise two exhaust hooks.
        const CardKind ironclad[5]={FeelNoPain,DarkEmbrace,BurningPact,TrueGrit,Strike};
        const CardKind silent[7]={DeadlyPoison,Acrobatics,BladeDance,Neutralize,Survivor,Footwork,PoisonedStab};
        const CardKind defect[5]={DefectZap,DefectDualcast,DefectBallLightning,DefectCoolheaded,DefectDefragment};
        const CardKind regent[5]={RegentFallingStar,RegentVenerate,RegentWroughtInWar,RegentCloakOfStars,RegentStrike};
        const CardKind necro[5]={NecroBodyguard,NecroUnleash,NecroGraveWarden,NecroScourge,NecroCountdown};
        const CardKind* opening=s.character==Character::Silent?silent:s.character==Character::Defect?defect:s.character==Character::Regent?regent:s.character==Character::Necrobinder?necro:ironclad;
        for(int k=0;k<(s.character==Character::Silent?7:5);k++)for(int j=0;j<s.draw.count;j++)if(s.cards[s.draw.ids[j]].kind==opening[k]){
            int id=remove(s.draw,j);put(s,s.hand,id,Zone::Hand);break;}
    }else{
        int innate=prioritize_innate(s);
        draw_cards(s,min(MaxHand,max(innate,5+((s.relics&BagOfPreparation)?2:0)+((s.relics&RingOfTheSnake)?2:0))));
    }
    relic_hook(s,RelicHook::AfterHandDraw);
    event_card_auto_preplay(s);resolve_pending(s);
    record(s,LogKind::Turn,1);finish_combat_if_won(s);s.revision++;
}
void new_run(State&s,uint32_t seed,bool practice,Character character){
    const EventRunFacts previous_facts=s.event_run;
    s=State{};
    s.event_run=previous_facts;
    if((unsigned)character>=CharacterKinds){s.phase=Phase::Error;return;}
    s.character=character;s.seed=seed?seed:1;
    s.rng_cards=s.seed^0xA341316Cu;s.rng_world=s.seed^0xC8013EA4u;
    s.hp=s.max_hp=(int16_t)character_definitions[(unsigned)character].starting_hp;
    s.potion_chance=40;s.floor=1;s.practice=practice;s.playing=-1;s.gold=99;s.current_node=-1;s.event_first=-1;
    if(practice){
        for(unsigned i=0;i<CardKinds;i++)if(card_in_pool(i,character)||card_colorless(i)){
            if(s.deck_count==MaxDeck){fail(s);return;}
            s.deck[s.deck_count++]={(CardKind)i,(uint8_t)(i==DarkEmbrace||i==TrueGrit)};
        }
    }else if(character==Character::Silent){
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={StrikeSilent,0};
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={DefendSilent,0};
        s.deck[s.deck_count++]={Neutralize,0};s.deck[s.deck_count++]={Survivor,0};
    }else if(character==Character::Defect||character==Character::Regent||character==Character::Necrobinder){
        CardKind strike=character==Character::Defect?DefectStrike:character==Character::Regent?RegentStrike:NecroStrikeNecrobinder;
        CardKind defend=character==Character::Defect?DefectDefend:character==Character::Regent?RegentDefend:NecroDefendNecrobinder;
        for(int i=0;i<4;i++)s.deck[s.deck_count++]={strike,0};
        for(int i=0;i<4;i++)s.deck[s.deck_count++]={defend,0};
        s.deck[s.deck_count++]={CardKind(character==Character::Defect?DefectZap:character==Character::Regent?RegentFallingStar:NecroBodyguard),0};
        s.deck[s.deck_count++]={CardKind(character==Character::Defect?DefectDualcast:character==Character::Regent?RegentVenerate:NecroUnleash),0};
    }else{
        for(int i=0;i<5;i++)s.deck[s.deck_count++]={Strike,0};
        for(int i=0;i<4;i++)s.deck[s.deck_count++]={Defend,0};s.deck[s.deck_count++]={Bash,0};
    }
    gain_relic(s,character_definitions[(unsigned)character].starting_relic);
    create_map(s);
    if(practice){s.current_node=1;s.floor=1;s.encounter=0;enter_combat(s);}
}
static void queue_legacy(State&s,Card c,int target,int energy){
    const Definition&d=definitions[c.kind];
    int hit=card_base_damage(s,c,target,s.playing),block=d.block+(c.upgraded?d.block_up:0),cards=d.draw+(c.upgraded?d.draw_up:0),power=d.power+(c.upgraded?d.power_up:0);
    if(d.type==CardType::Attack)relic_attack_begin(s);
    if(block)block=powered_block(s,c,block);
    record(s,LogKind::Play,c.kind,c.upgraded);
    if(d.hp_loss)enqueue(s,EffectKind::LoseHp,d.hp_loss);
    if(d.energy_gain)enqueue(s,EffectKind::Energy,d.energy_gain+(c.upgraded?d.energy_up:0));
    if(c.kind==LegSweep)enqueue(s,EffectKind::Weak,power,target);
    if(c.kind==DodgeAndRoll)enqueue(s,EffectKind::DodgeBlock,block);
    else if(c.kind==EscapePlan)enqueue(s,EffectKind::EscapeSkillBlock,block);
    else if(block)enqueue_card_block(s,block);
    if(c.kind==FlameBarrier)s.flame_barrier=(int16_t)min(9999,s.flame_barrier+power);
    if(c.kind==DemonForm)enqueue(s,EffectKind::Demon,power);
    if(c.kind==InfiniteBlades)enqueue(s,EffectKind::Blades,1);
    if(c.kind==FiendFire){
        const int count=s.hand.count;
        for(int i=0;i<count;i++)enqueue(s,EffectKind::ExhaustCard,s.hand.ids[i]);
        for(int i=0;i<count;i++)enqueue(s,EffectKind::Damage,hit,target);
    }else if(c.kind==SwordBoomerang){
        for(int i=0;i<power;i++)enqueue(s,EffectKind::RandomDamage,hit);
    }else if(c.kind==DaggerSpray){
        for(int repeat=0;repeat<2;repeat++)for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
    }else if(c.kind==Whirlwind)enqueue(s,EffectKind::WhirlwindHits,hit,energy);
    else if(c.kind==Thunderclap){
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Damage,hit,i);
        for(int i=0;i<s.enemy_count;i++)enqueue(s,EffectKind::Vulnerable,power,i);
    }else if(hit||c.kind==BodySlam){
        enqueue(s,c.kind==HandOfGreed?EffectKind::GreedDamage:EffectKind::Damage,hit,target);
        if(c.kind==TwinStrike)enqueue(s,EffectKind::Damage,hit,target);
    }
    if(c.kind==Headbutt)enqueue(s,EffectKind::ChooseReturn);
    if(c.kind==Predator)enqueue(s,EffectKind::DrawNextTurn,2);
    if(c.kind==Neutralize||c.kind==SuckerPunch)enqueue(s,EffectKind::Weak,power,target);
    if(c.kind==DeadlyPoison||c.kind==PoisonedStab)enqueue(s,EffectKind::Poison,power,target);
    if(c.kind==BouncingFlask)for(int i=0;i<(c.upgraded?4:3);i++)enqueue(s,EffectKind::RandomPoison,power);
    if(d.creates)enqueue(s,EffectKind::CreateShivs,d.creates+(c.upgraded?d.creates_up:0));
    if(c.kind==Footwork)enqueue(s,EffectKind::Dexterity,power);
    if(c.kind==NoxiousFumes)enqueue(s,EffectKind::Noxious,power);
    if(c.kind==Accuracy)enqueue(s,EffectKind::Accuracy,power);
    if(c.kind==Uppercut){enqueue(s,EffectKind::Weak,power,target);enqueue(s,EffectKind::Vulnerable,power,target);}
    if(c.kind==Bash)enqueue(s,EffectKind::Vulnerable,power,target);
    if(c.kind==BurningPact||(c.kind==TrueGrit&&c.upgraded))enqueue(s,EffectKind::ChooseExhaust);
    if(c.kind==TrueGrit&&!c.upgraded)enqueue(s,EffectKind::RandomExhaust);
    if(c.kind==Impatience)enqueue(s,EffectKind::DrawIfNoAttacks,cards);
    else if(c.kind==Purity)enqueue(s,EffectKind::ChooseExhaustMany,cards);
    else if(c.kind==Scrawl)enqueue(s,EffectKind::FillHand);
    else if(cards)enqueue(s,EffectKind::Draw,cards);
    if(c.kind==PanicButton)enqueue(s,EffectKind::NoBlock,2);
    if(c.kind==Shockwave)for(int i=0;i<s.enemy_count;i++){
        enqueue(s,EffectKind::Weak,power,i);enqueue(s,EffectKind::Vulnerable,power,i);
    }
    if(c.kind==Survivor||c.kind==Acrobatics||c.kind==DaggerThrow)enqueue(s,EffectKind::ChooseDiscard,1);
    if(c.kind==Prepared)enqueue(s,EffectKind::ChooseDiscard,cards);
    if(c.kind==BattleTrance)enqueue(s,EffectKind::NoDraw);
    if(c.kind==Anger)enqueue(s,EffectKind::CopyDiscard);
    if(c.kind==Inflame)enqueue(s,EffectKind::Strength,power);
    if(c.kind==FeelNoPain)enqueue(s,EffectKind::Pain,power);
    if(c.kind==DarkEmbrace)enqueue(s,EffectKind::Embrace,1);
    enqueue(s,EffectKind::Finish);
}
bool play(State&s,int h,int target){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1||h<0||h>=s.hand.count)return false;
    int id=s.hand.ids[h];if(id>=s.card_count||s.cards[id].kind>=CardKinds){fail(s);return false;}
    // All resource and card-specific rejection checks precede mutable play
    // setup, including clearing the previous relic's presentation signal.
    if(!can_play_card(s,id))return false;
    Card c=s.cards[id];const auto&d=definitions[c.kind];int price=cost(s,id);
    if(price>s.energy||(card_targets_enemy(s,c,id)&&!enemy_allow_target(s,target)))return false;
    if((c.kind==Anger&&s.card_count==MaxCards)||s.card_count+d.creates+(c.upgraded?d.creates_up:0)>MaxCards){fail(s);return false;}
    s.relic_flash=0;if(!start_instance(s,id,target,false))return false;
    resolve_pending(s);++s.revision;return true;
}

bool choose_exhaust(State&s,int index){
    if(s.phase!=Phase::ExhaustChoice||index<0||index>=s.hand.count)return false;
    s.phase=Phase::Combat;exhaust(s,index);resolve(s);s.revision++;return true;
}
bool choose_discard(State&s,const uint16_t* ids,unsigned count){
    if(s.phase!=Phase::DiscardChoice||!ids||count!=s.discard_required||!count||count>MaxHand)return false;
    // Validate the entire choice before moving any instance; invalid requests
    // cannot partially discard cards or consume RNG.
    uint16_t selected[MaxHand];
    for(unsigned i=0;i<count;i++){
        bool in_hand=false;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i])in_hand=true;
        if(!in_hand)return false;
        for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;
        selected[i]=ids[i];
    }
    for(unsigned i=0;i<count;i++)for(int j=0;j<s.hand.count;j++)if(s.hand.ids[j]==selected[i]){
        int id=remove(s.hand,j);put(s,s.discard,id,Zone::Discard);record(s,LogKind::Discard,s.cards[id].kind);break;
    }
    for(unsigned i=count;i>0;i--)silent_card_discarded(s,selected[i-1]);
    s.discard_required=0;s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
unsigned choice_min(const State&s){
    return s.phase==Phase::ExtendedChoice?s.expansion.choice_min:s.phase==Phase::ExhaustManyChoice?0:s.phase==Phase::DiscardChoice?s.discard_required:1;
}
unsigned choice_max(const State&s){return s.phase==Phase::ExtendedChoice?s.expansion.choice_max:s.phase==Phase::ExhaustManyChoice?s.exhaust_limit:choice_min(s);}
bool choose_exhaust_many(State&s,const uint16_t* ids,unsigned count){
    if(s.phase!=Phase::ExhaustManyChoice||count>s.exhaust_limit||(count&&!ids))return false;
    for(unsigned i=0;i<count;i++){
        bool found=false;for(unsigned j=0;j<s.hand.count;j++)if(s.hand.ids[j]==ids[i])found=true;
        if(!found)return false;for(unsigned j=0;j<i;j++)if(ids[i]==ids[j])return false;
    }
    if(s.effect_count+count>MaxEffects)return false;
    // Prepend captured instances in selection order. Each exhaust's hooks resolve
    // before the next captured card, without including newly drawn cards.
    for(unsigned i=count;i>0;i--)enqueue(s,EffectKind::ExhaustCard,ids[i-1],0,true);
    s.exhaust_limit=0;s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
bool choose_return_to_draw(State&s,int card_id){
    if(s.phase!=Phase::ReturnChoice||card_id<0||card_id>=s.card_count)return false;
    int index=-1;for(int i=0;i<s.discard.count;i++)if(s.discard.ids[i]==card_id){index=i;break;}
    if(index<0)return false;
    int id=remove(s.discard,index);put(s,s.draw,id,Zone::Draw);
    s.phase=Phase::Combat;resolve(s);s.revision++;return true;
}
bool end_turn(State&s){
    if(s.phase!=Phase::Combat||s.effect_count||s.playing!=-1)return false;
    s.relic_flash=0;queue_extension(s,XOp::TurnEnd);resolve_pending(s);++s.revision;return true;
}
bool choose_reward(State&s,int index){
    if(s.world.active)return world_claim_reward_card(s,index);
    if(s.phase!=Phase::Reward||(s.reward_claimed&2)||index< -1||index>2)return false;
    if(index>=0){if(s.deck_count==MaxDeck){fail(s);return false;}notify_deck_replaced(s,s.deck_count);s.deck[s.deck_count++]={s.reward[index],0};record(s,LogKind::Reward,s.reward[index]);}
    if(silent_next_reward(s)){s.revision++;return true;}
    s.reward_claimed|=2;s.revision++;return true;
}
bool camp(State&s,bool upgrading){
    if(s.phase!=Phase::Camp)return false;
    if(s.world.active){for(unsigned i=0;i<world_rest_option_count(s);i++)if(s.world.rest_options[i]==unsigned(upgrading?WorldRestOption::Smith:WorldRestOption::Heal))return world_rest_choose(s,i);return false;}
    if(upgrading){if(!has_upgrade(s))return false;s.phase=Phase::Upgrade;s.revision++;}
    else{heal(s,rest_heal_amount(s));finish_room(s);}
    return true;
}
bool upgrade_card(State&s,int index){
    if(s.phase!=Phase::Upgrade||index<0||index>=s.deck_count||!card_upgradeable(s.deck[index]))return false;
    s.deck[index].upgraded=1;record(s,LogKind::Upgrade,s.deck[index].kind);
    if(s.event_deck_action==EventDeckAction::Upgrade)complete_event_selection(s);else finish_room(s);
    return true;
}
const Pile& pile(const State&s,Zone z){switch(z){case Zone::Draw:return s.draw;case Zone::Hand:return s.hand;case Zone::Discard:return s.discard;case Zone::Exhaust:return s.exhaust;default:return s.powers;}}
