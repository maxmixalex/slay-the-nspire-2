// NCardSmithVfx: the deck preview and the three independent particle roots
// share their authored clock. No gameplay state or gameplay RNG is sampled.
static unsigned card_smith_curve(const uint16_t*values,unsigned age,unsigned lifetime){
 if(age>=lifetime)return values[32];
 const unsigned position=age*32u,at=position/lifetime,fraction=position%lifetime;
 return (unsigned(values[at])*(lifetime-fraction)+unsigned(values[at+1])*fraction+lifetime/2)/lifetime;
}
static int card_smith_elastic(unsigned age){
 if(!age)return 0;if(age>=150)return 4096;
 const int phase=int(age*40960u/450u)-1024;
 return 4096+int(4096-room_curve::expo_out(age,150))*room_curve::sine_turn(unsigned(phase))/4096;
}
static int card_smith_angle(unsigned age){
 if(age<250)return 0;
 const unsigned hit=age<500?0:age<750?1:2,elapsed=age-(250u+hit*250u);
 const int from[]={0,20,-10},to[]={20,-10,5};
 return (from[hit]*4096+(to[hit]-from[hit])*card_smith_elastic(elapsed))/360;
}
static void card_smith_particles(Canvas&c,HandPose pose,unsigned elapsed,uint32_t seed){
 // Curves sampled from vfx_card_smith.tscn's authored Curve resources,
 // through Godot's100-point baked representation, then linearly sampled.
 static constexpr uint16_t hammer_scale[]={0,1186,3252,4096,4095,4092,4087,4081,4073,4064,4054,4043,4032,4019,4006,3993,3979,3965,3951,3937,3923,3909,3896,3884,3872,3862,3852,3843,3836,3830,3826,3823,3822};
 static constexpr uint16_t hammer_alpha[]={4096,4096,4095,4093,4091,4088,4085,4082,4079,4076,4073,4070,4067,4065,4064,4063,4062,4045,3996,3915,3803,3659,3483,3275,3036,2765,2463,2130,1766,1371,945,488,0};
 static constexpr uint16_t impact_scale[]={633,1272,2487,3427,3753,3943,4045,4087,4096,4095,4093,4089,4084,4078,4071,4063,4054,4045,4035,4025,4014,4004,3994,3984,3975,3966,3957,3950,3944,3939,3935,3932,3931};
 static constexpr uint16_t spark_scale[]={4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4096,4078,3982,3742,3291,2561,1486,0};
 static constexpr int source[][2]={{74,-114},{-94,5},{61,127}};
 const int width=HandFaceW*pose.scale/1024;if(width<=0)return;
 for(unsigned hit=0;hit<3;++hit){const unsigned delay=hit*250u;
  if(elapsed<delay)continue;const unsigned age=elapsed-delay;if(age>=600)continue;
  const uint32_t random=room_vfx_hash(seed^(hit+1u)*0x9e3779b9u);
  const int x=pose.cx+source[hit][0]*width/300,y=pose.cy+source[hit][1]*width/300;
  if(age<500){
   const unsigned hq=card_smith_curve(hammer_scale,age,500),alpha=card_smith_curve(hammer_alpha,age,500);
   const int side=mx(1,int(128u*unsigned(width)*hq/(300u*4096u)));
   const int angle=(int(random%201u)-100)*4096/360;
   Canvas hammer=c;hammer.tint_blue=231;
   card_effect_texture(hammer,c.a.content.ui_image(I14_SmithHammer),x,y,side,side,
    CombatCardMotion::sine(angle),CombatCardMotion::sine(angle+1024),alpha*mn(255u,age*255u/10u)/4096u,presentation::Add);
   const unsigned iq=card_smith_curve(impact_scale,age,500);
   const int impact=mx(1,int(uint64_t(1024u)*13u*unsigned(width)*iq/(3u*10u*300u*4096u)));
   const int rotation=(int((random>>12)%5553u)-2376)*4096/3600;
   Canvas ring=c;ring.tint_blue=220;
   card_effect_texture(ring,c.a.content.ui_image(I15_SmithImpactStart+smith_impact_frame(age)),x,y,impact,impact,
    CombatCardMotion::sine(rotation),CombatCardMotion::sine(rotation+1024),255,presentation::Add);
  }
  // The original emits20 particles once with600ms lifetime,44% lifetime
  // randomness,421.6..1000px/s velocity and1500px/s² downward gravity.
  for(unsigned particle=0;particle<20;++particle){
   const uint32_t hash=room_vfx_hash(random+particle*0x85ebca6bu);
   const unsigned lifetime=600u-(hash%265u);if(age>=lifetime)continue;
   const int angle=int((hash>>8)&4095u),velocity=422+int((hash>>20)%579u);
   const int vx=CombatCardMotion::sine(angle+1024)*velocity/1024,vy=CombatCardMotion::sine(angle)*velocity/1024;
   const int sx=vx*int(age)/1000,sy=vy*int(age)/1000+int(3u*age*age/4000u);
   const int y_velocity=vy+int(3u*age/2u),direction=CombatCardMotion::direction(y_velocity,vx)-1024;
   const unsigned q=card_smith_curve(spark_scale,age,lifetime);
   const int size=mx(1,int((65u*unsigned(width)*(120u+(hash>>16)%131u)/300u)*q/(1000u*4096u)));
   const int trail=mx(1,int((unsigned(CombatCardMotion::magnitude(vx))+unsigned(CombatCardMotion::magnitude(y_velocity)))*unsigned(width)*mn(age,50u)/(300u*1000u)));
   Canvas spark=c;spark.tint_green=242;spark.tint_blue=206;
   card_effect_texture(spark,c.a.content.ui_image(I14_SmithSpark),x+sx*width/300,y+sy*width/300,size,size+trail,
    CombatCardMotion::sine(direction),CombatCardMotion::sine(direction+1024),255,presentation::Add);
  }
 }
}
static void smith_card_animation(Canvas&c,RewardPresentation::Upgrades::CardView&card,unsigned age,int center,int base_w){
 auto&a=c.a;constexpr unsigned fly_at=1250;constexpr int y=130;
 bool first=true;for(const auto&previous:a.reward_presentation.upgrades.cards){
  if(&previous==&card)break;
  if(previous.style==RewardPresentation::Upgrades::Smith&&previous.start==card.start){first=false;break;}
 }
 if(age>=fly_at){if(!card.launched){
   const EventDeckSnapshot snapshot{card.card,card.facts,card.genetic,card.scythe};
   a.reward_presentation.append(RewardPickupKind::Card,card.card.kind,unsigned(card.deck),center,y,card.start+fly_at,card.card,&snapshot);card.launched=true;
  }
  if(first&&age<1350)card_smith_particles(c,{160,y,base_w*1024/HandFaceW,0,1024},age-250,card.start);
  return;}
 const unsigned scale=room_curve::cubic_out(age,250);const int angle=card_smith_angle(age);
 CombatCardMotion::Sprite sprite;sprite.card=card.card;sprite.energy=card.energy;sprite.stars=card.stars;
 const EventDeckSnapshot snapshot{card.card,card.facts,card.genetic,card.scythe};char description[1024]{};
 a.world_card_description(card.card,-1,description,sizeof description,-1,-1,&card.facts,&snapshot);
 const auto&tile=hand_tile(c,-1,&sprite,-1,&card.facts,true,description);
 draw_hand_tile(c,tile,{center,y,int(unsigned(base_w)*scale*1024u/(unsigned(HandFaceW)*4096u)),CombatCardMotion::sine(angle),CombatCardMotion::sine(angle+1024)},false);
 // The source owns one Spark1/2/3 set at the preview-container center;
 // a multi-card Smith rotates every card but does not duplicate the bursts.
 if(first&&age>=250)card_smith_particles(c,{160,y,base_w*1024/HandFaceW,0,1024},age-250,card.start);
}
