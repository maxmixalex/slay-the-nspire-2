void ancient_options(State&s,const RelicId*ids,unsigned count){auto&e=s.original_events;options(s);e.offer_count=uint16_t(count);
 for(unsigned i=0;i<count;i++){e.offers[i]=ids[i];option_key(s,i,"INITIAL.options.",ids[i]<WorldRelicKinds?world_relic_key(ids[i]):"OPTION_POOL_3_LOCKED");}}
bool ancient_begin(State&s){auto&e=s.original_events;
 world_relic_clear_offers(s);if(e.id==OriginalEventId::Neow)s.hp=0;
 int amount=s.max_hp-s.hp;if(world_ascension(s)>=2)amount=amount*8/10;heal(s,amount);
 if(e.id==OriginalEventId::Neow&&s.run_options.modifiers)return modifier_neow_begin(s);
 auto rng=event_random(s);RelicId offers[3]{};
 switch(e.id){
 case OriginalEventId::Neow:{
  const RelicId curses[]={WorldRelicCursedPearl,WorldRelicDowsingRod,WorldRelicHeftyTablet,WorldRelicLargeCapsule,WorldRelicLeafyPoultice,WorldRelicNeowsBones,WorldRelicNeowsSacrifice,WorldRelicPrecariousShears,WorldRelicSilkenTress,WorldRelicSilverCrucible};
  const RelicId base[]={WorldRelicArcaneScroll,WorldRelicBoomingConch,WorldRelicFishingRod,WorldRelicGoldenPearl,WorldRelicKaleidoscope,WorldRelicLeadPaperweight,WorldRelicLostCoffer,WorldRelicNeowsTorment,WorldRelicNewLeaf,WorldRelicPhialHolster,WorldRelicPreciseScissors,WorldRelicScrollBoxes,WorldRelicWingedBoots};
  RelicId valid[16];unsigned n=0;for(auto id:curses)if(world_relic_allowed_at_neow(s,id))valid[n++]=id;if(!n)return false;offers[2]=valid[rng.below(n)];n=0;
  for(auto id:base){if((offers[2]==WorldRelicCursedPearl&&id==WorldRelicGoldenPearl)||(offers[2]==WorldRelicHeftyTablet&&id==WorldRelicArcaneScroll)||(offers[2]==WorldRelicLeafyPoultice&&id==WorldRelicNewLeaf)||(offers[2]==WorldRelicPrecariousShears&&id==WorldRelicPreciseScissors)||(offers[2]==WorldRelicNeowsSacrifice&&(id==WorldRelicPhialHolster||id==WorldRelicLostCoffer)))continue;valid[n++]=id;}
  if(offers[2]!=WorldRelicLargeCapsule)valid[n++]=rng.below(2)==0?WorldRelicLavaRock:WorldRelicSmallCapsule;
  valid[n++]=rng.below(2)==0?WorldRelicNutritiousOyster:WorldRelicStoneHumidifier;valid[n++]=rng.below(2)==0?WorldRelicNeowsTalisman:WorldRelicPomander;
  unsigned used=0;for(unsigned i=0;i<n;i++)if(world_relic_allowed_at_neow(s,valid[i]))valid[used++]=valid[i];if(used<2)return false;reference::shuffle(valid,used,rng);offers[0]=valid[0];offers[1]=valid[1];break;}
 case OriginalEventId::Darv:{
  const RelicId base[]={WorldRelicAstrolabe,WorldRelicBlackStar,WorldRelicCallingBell,WorldRelicEmptyCage,WorldRelicPandorasBox,WorldRelicRunicPyramid,WorldRelicSneckoEye,WorldRelicEctoplasm,WorldRelicSozu,WorldRelicPhilosophersStone,WorldRelicVelvetChoker};
  RelicId pool[11];unsigned n=0;for(unsigned i=0;i<11;i++){if((i==7||i==8)&&e.act_index!=1)continue;if(i>=9&&e.act_index<1)continue;rng.below(1);pool[n++]=base[i];}
  reference::shuffle(pool,n,rng);offers[0]=pool[0];offers[1]=pool[1];if(rng.below(2)==0){if(!world_relic_setup_offer(s,WorldRelicDustyTome))return false;offers[2]=WorldRelicDustyTome;}else offers[2]=pool[2];break;}
 case OriginalEventId::Nonupeipe:{RelicId pool[]={WorldRelicBlessedAntler,WorldRelicBrilliantScarf,WorldRelicDelicateFrond,WorldRelicDiamondDiadem,WorldRelicFurCoat,WorldRelicGlitter,WorldRelicJewelryBox,WorldRelicLoomingFruit,WorldRelicSignetRing,WorldRelicBeautifulBracelet};unsigned n=count_enchantable(s,EventEnchantment::Swift)>=4?10:9;reference::shuffle(pool,n,rng);for(unsigned i=0;i<3;i++)offers[i]=pool[i];break;}
 case OriginalEventId::Tanx:{RelicId pool[]={WorldRelicClaws,WorldRelicCrossbow,WorldRelicIronClub,WorldRelicMeatCleaver,WorldRelicSai,WorldRelicSpikedGauntlets,WorldRelicTanxsWhistle,WorldRelicThrowingAxe,WorldRelicWarHammer,WorldRelicTriBoomerang};unsigned n=count_enchantable(s,EventEnchantment::Instinct)>=3?10:9;reference::shuffle(pool,n,rng);for(unsigned i=0;i<3;i++)offers[i]=pool[i];break;}
 case OriginalEventId::Orobas:{
  const unsigned character_order[]={0,1,3,4,2};unsigned chars[5],nc=0;for(auto c:character_order)if(c!=unsigned(s.character))chars[nc++]=c;unsigned character=chars[rng.below(nc)];
  RelicId pool1[]={WorldRelicElectricShrymp,WorldRelicGlassEye,NoWorldRelic};pool1[2]=world_random_float(rng)<world_float(0x3eaaaaaaU)?WorldRelicPrismaticGem:WorldRelicSeaGlass;
  if(pool1[2]==WorldRelicSeaGlass)world_relic_setup_offer(s,WorldRelicSeaGlass,int(character));offers[0]=pool1[rng.below(3)];
  const RelicId pool2[]={WorldRelicAlchemicalCoffer,WorldRelicDriftwood,WorldRelicRadiantPearl,WorldRelicSandCastle};offers[1]=pool2[rng.below(4)];
  RelicId pool3[2];unsigned n=0;if(world_relic_setup_offer(s,WorldRelicTouchOfOrobas))pool3[n++]=WorldRelicTouchOfOrobas;if(world_relic_setup_offer(s,WorldRelicArchaicTooth))pool3[n++]=WorldRelicArchaicTooth;
  if(n)offers[2]=pool3[rng.below(n)];else{rng.below(1);offers[2]=NoWorldRelic;}break;}
 case OriginalEventId::Pael:{const RelicId pool1[]={WorldRelicPaelsFlesh,WorldRelicPaelsHorn,WorldRelicPaelsTears};offers[0]=pool1[rng.below(3)];
  RelicId pool2[7]={WorldRelicPaelsWing};unsigned n=1;if(count_enchantable(s,EventEnchantment::Goopy)>=3)pool2[n++]=WorldRelicPaelsClaw;if(count_removable(s)>=5)pool2[n++]=WorldRelicPaelsTooth;unsigned count=n;for(unsigned i=0;i<count;i++)pool2[n++]=pool2[i];pool2[n++]=WorldRelicPaelsGrowth;offers[1]=pool2[rng.below(n)];
  const RelicId pool3[]={WorldRelicPaelsEye,WorldRelicPaelsBlood,WorldRelicPaelsLegion};offers[2]=pool3[rng.below(world_has_event_pet(s)?2:3)];break;}
 case OriginalEventId::Tezcatara:{const RelicId pool1[]={WorldRelicVeryHotCocoa,WorldRelicYummyCookie,WorldRelicNutritiousSoup};bool strike=false;for(unsigned i=0;i<s.deck_count;i++)if(basic_strike(s.deck[i].kind))strike=true;offers[0]=pool1[rng.below(strike?3:2)];
  const RelicId pool2[]={WorldRelicBiiigHug,WorldRelicStorybook,WorldRelicToastyMittens};offers[1]=pool2[rng.below(3)];const RelicId pool3[]={WorldRelicGoldenCompass,WorldRelicPumpkinCandle,WorldRelicToyBox,WorldRelicSealOfGold};offers[2]=pool3[rng.below(4)];break;}
 case OriginalEventId::Vakuu:{RelicId p1[]={WorldRelicBloodSoakedRose,WorldRelicWhisperingEarring,WorldRelicFiddle};RelicId p2[]={WorldRelicPreservedFog,WorldRelicSereTalon,WorldRelicDistinguishedCape};RelicId p3[]={WorldRelicChoicesParadox,WorldRelicMusicBox,WorldRelicLordsParasol,WorldRelicJeweledMask};reference::shuffle(p1,3,rng);reference::shuffle(p2,3,rng);reference::shuffle(p3,4,rng);offers[0]=p1[0];offers[1]=p2[0];offers[2]=p3[0];break;}
 default:return false;
 }
 keep_random(s,rng);ancient_options(s,offers,3);return true;
}
