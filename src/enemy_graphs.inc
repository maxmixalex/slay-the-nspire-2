struct EnemyGraph {uint8_t start,count,next[8];};
static const EnemyGraph enemy_graphs[]={
 {0,0,{0}},
 {0,3,{1,2,0}}, // Aeonglass
 {0,1,{0}}, // Architect
 {0,1,{0}}, // AssassinRubyRaider
 {0,3,{1,2,0}}, // AxeRubyRaider
 {2,3,{2,2,1}}, // Axebot
 {0,1,{0}}, // BattleFriendV1
 {0,1,{0}}, // BattleFriendV2
 {0,1,{0}}, // BattleFriendV3
 {0,1,{0}}, // BowlbugEgg
 {0,3,{1,2,2}}, // BowlbugNectar
 {0,2,{192,0}}, // BowlbugRock
 {1,2,{1,0}}, // BowlbugSilk
 {0,2,{1,0}}, // BruteRubyRaider
 {0,4,{1,3,3,3}}, // BygoneEffigy
 {1,2,{1,0}}, // Byrdonis
 {0,2,{1,1}}, // CalcifiedCultist
 {0,6,{1,1,3,4,5,3}}, // CeremonialBeast
 {255,2,{1,0}}, // Chomper
 {255,3,{1,2,0}}, // CorpseSlug
 {1,2,{1,0}}, // CrossbowRubyRaider
 {0,5,{1,2,3,4,0}}, // Crusher
 {0,4,{1,2,3,1}}, // CubexConstruct
 {0,2,{1,1}}, // DampCultist
 {255,5,{2,0,1,4,128}}, // DecimillipedeSegmentBack
 {255,5,{2,0,1,4,128}}, // DecimillipedeSegmentFront
 {255,5,{2,0,1,4,128}}, // DecimillipedeSegmentMiddle
 {0,2,{1,1}}, // DevotedSculptor
 {1,3,{1,2,0}}, // Entomancer
 {192,3,{128,2,128}}, // Exoskeleton
 {0,1,{0}}, // EyeWithTeeth
 {192,3,{192,192,192}}, // Fabricator
 {0,4,{128,128,129,128}}, // FakeMerchantMonster
 {0,2,{1,1}}, // FatGremlin
 {2,3,{128,128,128}}, // FlailKnight
 {129,3,{128,128,128}}, // Flyconid
 {0,4,{1,128,3,1}}, // Fogmog
 {1,3,{128,128,128}}, // FossilStalker
 {2,4,{192,0,1,2}}, // FrogKnight
 {0,3,{2,0,1}}, // FuzzyWurmCrawler
 {0,1,{255}}, // GasBomb
 {1,3,{2,0,1}}, // GlobeHead
 {0,3,{1,2,0}}, // GremlinMerc
 {0,1,{0}}, // Guardbot
 {2,3,{1,0,0}}, // HauntedShip
 {0,3,{128,128,128}}, // HunterKiller
 {0,4,{1,2,3,0}}, // InfestedPrism
 {255,3,{129,0,0}}, // Inklet
 {255,3,{1,2,0}}, // KinFollower
 {0,4,{1,2,3,0}}, // KinPriest
 {0,4,{1,2,3,192}}, // KnowledgeDemon
 {0,5,{192,3,4,2,1}}, // LagavulinMatriarch
 {1,2,{1,0}}, // LeafSlimeM
 {128,2,{128,128}}, // LeafSlimeS
 {0,3,{1,2,1}}, // LivingFog
 {0,2,{192,1}}, // LivingShield
 {0,3,{2,0,1}}, // LouseProgenitor
 {0,5,{1,4,3,4,2}}, // MagiKnight
 {2,3,{128,128,128}}, // Mawler
 {0,4,{1,2,3,1}}, // MechaKnight
 {2,3,{128,128,128}}, // MysteriousKnight
 {192,3,{1,2,0}}, // Myte
 {192,3,{1,2,0}}, // Nibbit
 {0,1,{0}}, // Noisebot
 {0,4,{1,2,192,1}}, // Ovicopter
 {0,4,{1,2,3,0}}, // OwlMagistrate
 {0,1,{0}}, // Parafright
 {192,4,{1,2,3,0}}, // PhantasmalGardener
 {0,2,{1,0}}, // PhrogParasite
 {255,3,{2,0,1}}, // PunchConstruct
 {0,6,{1,192,193,4,5,3}}, // Queen
 {0,5,{1,2,3,4,0}}, // Rocket
 {2,3,{2,128,1}}, // ScrollOfBiting
 {0,3,{1,2,0}}, // Seapunk
 {1,2,{1,0}}, // SewerClam
 {0,3,{1,2,1}}, // ShrinkerBeetle
 {0,4,{1,2,3,0}}, // SkulkingColony
 {0,4,{3,2,0,1}}, // SlimedBerserker
 {0,3,{128,0,0}}, // SlitheringStrangler
 {0,3,{128,128,128}}, // SludgeSpinner
 {0,2,{192,1}}, // SlumberingBeetle
 {0,1,{0}}, // SnappingJaxfruit
 {0,2,{1,1}}, // SneakyGremlin
 {0,5,{1,2,3,4,0}}, // SoulFysh
 {0,3,{128,128,128}}, // SoulNexus
 {0,3,{1,128,128}}, // SpectralKnight
 {0,3,{1,2,0}}, // SpinyToad
 {0,1,{0}}, // Stabbot
 {0,4,{1,0,3,0}}, // TerrorEel
 {1,7,{192,2,1,3,5,6,4}}, // TestSubject
 {0,2,{1,0}}, // TheForgotten
 {0,5,{1,3,1,4,2}}, // TheInsatiable
 {0,2,{1,0}}, // TheLost
 {0,4,{128,128,128,128}}, // TheObscura
 {0,5,{3,4,1,2,4}}, // ThievingHopper
 {192,3,{1,2,0}}, // Toadpole
 {0,5,{1,2,3,4,2}}, // TorchHeadAmalgam
 {0,2,{1,1}}, // ToughEgg
 {0,2,{1,1}}, // TrackerRubyRaider
 {0,4,{1,2,2,0}}, // Tunneler
 {0,3,{1,2,0}}, // TurretOperator
 {1,2,{128,128}}, // TwigSlimeM
 {0,1,{0}}, // TwigSlimeS
 {255,4,{128,128,128,128}}, // TwoTailedRat
 {0,4,{1,2,3,0}}, // Vantom
 {1,3,{2,0,1}}, // VineShambler
 {0,8,{1,2,3,4,5,1,7,7}}, // WaterfallGiant
 {255,3,{1,0,192}}, // Wriggler
 {0,1,{0}}, // Zapbot
};
struct EnemyBranch {uint8_t node,max_repeats,cooldown;uint8_t weight;};
static const EnemyBranch eb_DecimillipedeSegmentBack_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_DecimillipedeSegmentFront_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_DecimillipedeSegmentMiddle_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_Exoskeleton_0[]={{0,1,0,1},{1,1,0,1}};
static const EnemyBranch eb_Fabricator_0[]={{0,0,0,1},{1,0,0,1}};
static const EnemyBranch eb_FakeMerchantMonster_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1},{3,1,3,1}};
static const EnemyBranch eb_FakeMerchantMonster_1[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_FlailKnight_0[]={{0,1,0,1},{1,2,0,1},{2,2,0,1}};
static const EnemyBranch eb_Flyconid_0[]={{0,1,3,1},{1,1,2,1},{2,1,0,1}};
static const EnemyBranch eb_Flyconid_1[]={{1,1,2,1},{2,1,0,1}};
static const EnemyBranch eb_Fogmog_0[]={{2,1,0,4},{3,1,0,6}};
static const EnemyBranch eb_FossilStalker_0[]={{1,2,0,1},{0,2,0,1},{2,2,0,1}};
static const EnemyBranch eb_HunterKiller_0[]={{1,1,0,1},{2,2,0,1}};
static const EnemyBranch eb_Inklet_0[]={{0,2,0,1},{1,1,0,1}};
static const EnemyBranch eb_Inklet_1[]={{2,1,0,1},{1,1,0,1}};
static const EnemyBranch eb_LeafSlimeS_0[]={{0,1,0,1},{1,1,0,1}};
static const EnemyBranch eb_Mawler_0[]={{0,1,0,1},{1,255,0,1},{2,1,0,1}};
static const EnemyBranch eb_MysteriousKnight_0[]={{0,1,0,1},{1,2,0,1},{2,2,0,1}};
static const EnemyBranch eb_PhrogParasite_0[]={{0,1,0,1},{1,1,0,1}};
static const EnemyBranch eb_ScrollOfBiting_0[]={{0,1,0,1},{1,2,0,1}};
static const EnemyBranch eb_SlitheringStrangler_0[]={{1,0,0,1},{2,0,0,1}};
static const EnemyBranch eb_SludgeSpinner_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_SoulNexus_0[]={{0,1,0,1},{1,1,0,1},{2,1,0,1}};
static const EnemyBranch eb_SpectralKnight_0[]={{1,2,0,1},{2,1,0,1}};
static const EnemyBranch eb_TheObscura_0[]={{1,1,0,1},{2,1,0,1},{3,1,0,1}};
static const EnemyBranch eb_TwigSlimeM_0[]={{0,2,0,1},{1,1,0,1}};
static const EnemyBranch eb_TwoTailedRat_0[]={{0,1,0,1},{1,1,0,1},{2,1,3,1},{3,255,0,9}};
static const EnemyBranch* enemy_branches(EnemyId k,int branch,unsigned&count){switch(k){
 case EnemyId::DecimillipedeSegmentBack:
  if(branch==0){count=3;return eb_DecimillipedeSegmentBack_0;}
  break;
 case EnemyId::DecimillipedeSegmentFront:
  if(branch==0){count=3;return eb_DecimillipedeSegmentFront_0;}
  break;
 case EnemyId::DecimillipedeSegmentMiddle:
  if(branch==0){count=3;return eb_DecimillipedeSegmentMiddle_0;}
  break;
 case EnemyId::Exoskeleton:
  if(branch==0){count=2;return eb_Exoskeleton_0;}
  break;
 case EnemyId::Fabricator:
  if(branch==0){count=2;return eb_Fabricator_0;}
  break;
 case EnemyId::FakeMerchantMonster:
  if(branch==0){count=4;return eb_FakeMerchantMonster_0;}
  if(branch==1){count=3;return eb_FakeMerchantMonster_1;}
  break;
 case EnemyId::FlailKnight:
  if(branch==0){count=3;return eb_FlailKnight_0;}
  break;
 case EnemyId::Flyconid:
  if(branch==0){count=3;return eb_Flyconid_0;}
  if(branch==1){count=2;return eb_Flyconid_1;}
  break;
 case EnemyId::Fogmog:
  if(branch==0){count=2;return eb_Fogmog_0;}
  break;
 case EnemyId::FossilStalker:
  if(branch==0){count=3;return eb_FossilStalker_0;}
  break;
 case EnemyId::HunterKiller:
  if(branch==0){count=2;return eb_HunterKiller_0;}
  break;
 case EnemyId::Inklet:
  if(branch==0){count=2;return eb_Inklet_0;}
  if(branch==1){count=2;return eb_Inklet_1;}
  break;
 case EnemyId::LeafSlimeS:
  if(branch==0){count=2;return eb_LeafSlimeS_0;}
  break;
 case EnemyId::Mawler:
  if(branch==0){count=3;return eb_Mawler_0;}
  break;
 case EnemyId::MysteriousKnight:
  if(branch==0){count=3;return eb_MysteriousKnight_0;}
  break;
 case EnemyId::PhrogParasite:
  if(branch==0){count=2;return eb_PhrogParasite_0;}
  break;
 case EnemyId::ScrollOfBiting:
  if(branch==0){count=2;return eb_ScrollOfBiting_0;}
  break;
 case EnemyId::SlitheringStrangler:
  if(branch==0){count=2;return eb_SlitheringStrangler_0;}
  break;
 case EnemyId::SludgeSpinner:
  if(branch==0){count=3;return eb_SludgeSpinner_0;}
  break;
 case EnemyId::SoulNexus:
  if(branch==0){count=3;return eb_SoulNexus_0;}
  break;
 case EnemyId::SpectralKnight:
  if(branch==0){count=2;return eb_SpectralKnight_0;}
  break;
 case EnemyId::TheObscura:
  if(branch==0){count=3;return eb_TheObscura_0;}
  break;
 case EnemyId::TwigSlimeM:
  if(branch==0){count=2;return eb_TwigSlimeM_0;}
  break;
 case EnemyId::TwoTailedRat:
  if(branch==0){count=4;return eb_TwoTailedRat_0;}
  break;
 default:break;}count=0;return nullptr;}
