// Original animator predicates have their own state: a selected intent is not
// evidence that an animation has executed. These two existing saved data cells
// hold the live visual bits and the bits at the latest actual death.
static unsigned enemy_visual_current_bits(const State&s,int t){
 const auto&m=s.enemy_world.models[t];
 if(m.kind==EnemyId::Toadpole)return enemy_power_value(s,t,EnemyPower::Thorns)>0;
 if(m.kind==EnemyId::Tunneler)return unsigned(m.data[8]&1)|(enemy_power_value(s,t,EnemyPower::Burrowed)?2u:0u);
 return unsigned(m.data[8]);
}
static unsigned enemy_visual_bits(const State&s,int t){
 const auto&m=s.enemy_world.models[t];
 return s.enemies[t].hp<=0&&m.data[9]?unsigned(m.data[9]-1):enemy_visual_current_bits(s,t);
}
static void enemy_visual_snapshot_death(State&s,int t){s.enemy_world.models[t].data[9]=(int16_t)(enemy_visual_current_bits(s,t)+1);}
static void enemy_visual_attack_end(State&s,int t){
 auto&m=s.enemy_world.models[t];
 if(m.kind==EnemyId::CeremonialBeast&&m.node==1)m.data[8]&=~2;
 if(m.kind==EnemyId::CrossbowRubyRaider&&m.node==0)m.data[8]=0;
}
static void enemy_visual_move_begin(State&s,int t,int node){
 auto&m=s.enemy_world.models[t];
 switch(m.kind){
 case EnemyId::CeremonialBeast:if(node==1)m.data[8]|=2;if(node==2||node==31){m.data[8]&=~1;if(node==31)notify_enemy_animation(s,t,"Unstun",600);}break;
 case EnemyId::CorpseSlug:if(node==31){notify_enemy_animation(s,t,"DevourEndkTrigger",500);m.data[8]=0;}break;
 case EnemyId::CubexConstruct:if(node==0)m.data[8]=2;break;
 case EnemyId::FuzzyWurmCrawler:if(node<=2)m.data[8]=(int16_t)(node==2);break;
 // Selecting an intent is not executing it: Whistle can interrupt the
 // first Gimme and select it again. Persist only the actual spoken fact.
 case EnemyId::GremlinMerc:if(node==0)m.data[0]=1;break;
 case EnemyId::KnowledgeDemon:if(node==2||node==3)m.data[8]=(int16_t)(node==2);break;
 case EnemyId::LagavulinMatriarch:if(node>=1&&node<=4)notify_enemy_animation(s,t,"SleepingVfxStop");break;
 case EnemyId::MechaKnight:if(node==2||node==3)m.data[8]=(int16_t)(node==2);break;
 case EnemyId::PhantasmalGardener:if(node>=0&&node<3){m.data[1]=1;notify_enemy_animation(s,t,"DefaultScaleChanged");}break;
 case EnemyId::Rocket:if(node>=2&&node<=4)m.data[8]=(int16_t)(node==2?1:node==3?2:0);break;
 case EnemyId::SlumberingBeetle:if(node==31){notify_enemy_animation(s,t,"SleepingVfxStop");m.data[8]=1;notify_enemy_animation(s,t,"WakeUp",600);}break;
 case EnemyId::SoulFysh:if(node==3||node==4)m.data[8]=(int16_t)(node==3);break;
 case EnemyId::SpinyToad:if(node==0||node==1)m.data[8]=(int16_t)(node==0);break;
 case EnemyId::ThievingHopper:if(node==3)m.data[8]=1;break;
 case EnemyId::Tunneler:if(node<=3||node==31){m.data[8]=0;if(node==31)notify_enemy_animation(s,t,"WakeUp",250);}break;
 default:break;
 }
}
static void enemy_visual_move_end(State&s,int t,int node){
 auto&m=s.enemy_world.models[t];
 switch(m.kind){
 case EnemyId::CeremonialBeast:if(node==1)m.data[8]&=~2;break;
 case EnemyId::CrossbowRubyRaider:if(node==0||node==1)m.data[8]=(int16_t)(node==1);break;
 case EnemyId::FatGremlin:case EnemyId::SneakyGremlin:if(node==0)m.data[8]=1;break;
 case EnemyId::SewerClam:if(node==0&&m.data[8]<7)++m.data[8];break;
 case EnemyId::TheObscura:if(node==0)m.data[8]=1;break;
 case EnemyId::ThievingHopper:if(node==4)m.data[8]=0;break;
 case EnemyId::Vantom:if(node>=0&&node<4)m.data[8]=(int16_t)(node==0?2:node==1?3:node==2?0:1);break;
 default:break;
 }
}
bool enemy_visual_fact(const State&s,int t,const char*key,int&out){
 if(!key||t<0||t>=s.enemy_count)return false;
 const auto&m=s.enemy_world.models[t];const unsigned bits=enemy_visual_bits(s,t);
 auto answer=[&](const char*name,int value){if(!enemy_text_equal(key,name))return false;out=value;return true;};
 if(answer("live_combat",s.enemy_world.active!=0))return true;
 if(answer("EnemyDown",(m.flags&EnemyDown)!=0)||answer("Shrink",enemy_power_value(s,t,EnemyPower::Shrink)!=0||s.expansion.beetle[t]!=0))return true;
 if(m.kind==EnemyId::Crusher||m.kind==EnemyId::Rocket){
  int left=-1,right=-1;for(int other=0;other<s.enemy_count;++other){if(s.enemy_world.models[other].kind==EnemyId::Crusher)left=other;if(s.enemy_world.models[other].kind==EnemyId::Rocket)right=other;}
  if(answer("kaiser_left_dead",left<0||s.enemies[left].hp<=0)||answer("kaiser_right_dead",right<0||s.enemies[right].hp<=0))return true;
  if(answer("right_arm_state",right>=0?(int)enemy_visual_bits(s,right):0))return true;
 }
 if(enemy_text_equal(key,"living_primary_teammates")){
  int count=0;for(int other=0;other<s.enemy_count;++other)if(other!=t&&enemy_slot_alive(s,other)&&!enemy_power_value(s,other,EnemyPower::Minion))++count;
  out=count;return true;
 }
 switch(m.kind){
 case EnemyId::Axebot:return answer("shouldPlaySpawnAnimation",m.data[0]>0);
 case EnemyId::BowlbugRock:return answer("IsOffBalance",m.data[0]!=0);
 case EnemyId::CeremonialBeast:return answer("InMidCharge",(bits&2)!=0)||answer("IsStunnedByPlowRemoval",(bits&1)!=0)||answer("ShouldPlayRegularHurtAnim",!(bits&3));
 case EnemyId::CorpseSlug:return answer("isRavenous",(bits&1)!=0);
 case EnemyId::CrossbowRubyRaider:return answer("IsCrossbowReloaded",(bits&1)!=0);
 case EnemyId::CubexConstruct:return answer("IsBurrowed",(bits&1)!=0)||answer("IsCharging",(bits&2)!=0);
 case EnemyId::FatGremlin:return answer("Heist",m.stolen_gold)||answer("IsAwake",(bits&1)!=0);
 case EnemyId::FuzzyWurmCrawler:return answer("IsPuffed",(bits&1)!=0);
 case EnemyId::GasBomb:return answer("HasExploded",m.data[0]!=0);
 case EnemyId::KnowledgeDemon:return answer("isBurnt",(bits&1)!=0);
 case EnemyId::LagavulinMatriarch:return answer("IsAwake",(bits&1)!=0)||answer("IsShellAwake",(bits&2)!=0);
 case EnemyId::SlumberingBeetle:case EnemyId::SneakyGremlin:return answer("IsAwake",(bits&1)!=0);
 case EnemyId::MechaKnight:return answer("IsWoundUp",(bits&1)!=0);
 case EnemyId::Parafright:return answer("IllusionReviving",(m.flags&EnemyDown)!=0);
 case EnemyId::PhantasmalGardener:return answer("SkittishUsed",m.power_aux[(unsigned)EnemyPower::Skittish]!=0);
 case EnemyId::Rocket:return answer("right_arm_state",(int)bits);
 case EnemyId::SewerClam:return answer("coral_growth_step",(int)bits);
 case EnemyId::SoulFysh:return answer("IsInvisible",(bits&1)!=0);
 case EnemyId::SpinyToad:return answer("IsSpiny",(bits&1)!=0);
 case EnemyId::TestSubject:return answer("Respawns",m.data[0]);
 case EnemyId::TheInsatiable:return answer("HasLiquified",m.data[0]!=0);
 case EnemyId::TheObscura:return answer("HasSummoned",(bits&1)!=0);
 case EnemyId::ThievingHopper:return answer("IsHovering",(bits&1)!=0);
 case EnemyId::Toadpole:return answer("Thorns",(bits&1)!=0);
 case EnemyId::ToughEgg:return answer("IsHatched",m.data[0]!=0);
 case EnemyId::Tunneler:return answer("Burrowed",(bits&2)!=0)||answer("IsStunned",(bits&1)!=0);
 case EnemyId::Vantom:return answer("tail_charge",(int)bits);
 case EnemyId::WaterfallGiant:return answer("IsAboutToBlow",m.data[2]!=0);
 default:return false;
 }
}
