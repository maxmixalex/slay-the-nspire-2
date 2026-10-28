// Ordered, independently encoded numeric commands. Branch/stateful actions live in enemy_moves.inc.
enum class EnemyFixedOp:uint8_t { Attack,Power,Block,Heal,Card };
struct EnemyFixedAction {EnemyFixedOp op;int8_t target;int16_t base,tough,deadly,extra;};
static const EnemyFixedAction enemy_fixed_actions[]={
 {EnemyFixedOp::Attack,-2,22,22,26,1}, // Aeonglass 0
 {EnemyFixedOp::Block,-2,33,33,33,1}, // Aeonglass 0
 {EnemyFixedOp::Attack,-2,11,11,12,2}, // Aeonglass 1
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // AssassinRubyRaider 0
 {EnemyFixedOp::Attack,-2,5,5,6,1}, // AxeRubyRaider 0
 {EnemyFixedOp::Block,-2,5,5,6,1}, // AxeRubyRaider 0
 {EnemyFixedOp::Attack,-2,5,5,6,1}, // AxeRubyRaider 1
 {EnemyFixedOp::Block,-2,5,5,6,1}, // AxeRubyRaider 1
 {EnemyFixedOp::Attack,-2,12,12,13,1}, // AxeRubyRaider 2
 {EnemyFixedOp::Attack,-2,10,10,11,2}, // Axebot 1
 {EnemyFixedOp::Attack,-2,14,14,18,1}, // Axebot 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Weak}, // Axebot 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // Axebot 2
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // BowlbugEgg 0
 {EnemyFixedOp::Block,-2,7,7,8,1}, // BowlbugEgg 0
 {EnemyFixedOp::Attack,-2,3,3,3,1}, // BowlbugNectar 0
 {EnemyFixedOp::Power,-2,15,15,16,(int)EnemyPower::Strength}, // BowlbugNectar 1
 {EnemyFixedOp::Attack,-2,3,3,3,1}, // BowlbugNectar 2
 {EnemyFixedOp::Attack,-2,4,4,5,2}, // BowlbugSilk 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // BowlbugSilk 1
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // BruteRubyRaider 0
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // BruteRubyRaider 1
 {EnemyFixedOp::Power,-2,10,10,10,(int)EnemyPower::Strength}, // BygoneEffigy 1
 {EnemyFixedOp::Attack,-2,13,13,15,1}, // BygoneEffigy 3
 {EnemyFixedOp::Attack,-2,3,3,4,3}, // Byrdonis 0
 {EnemyFixedOp::Attack,-2,17,17,19,1}, // Byrdonis 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Ritual}, // CalcifiedCultist 0
 {EnemyFixedOp::Attack,-2,9,9,11,1}, // CalcifiedCultist 1
 {EnemyFixedOp::Power,-2,150,150,160,(int)EnemyPower::Plow}, // CeremonialBeast 0
 {EnemyFixedOp::Attack,-2,18,18,20,1}, // CeremonialBeast 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // CeremonialBeast 1
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Ringing}, // CeremonialBeast 3
 {EnemyFixedOp::Attack,-2,15,15,17,1}, // CeremonialBeast 4
 {EnemyFixedOp::Attack,-2,17,17,19,1}, // CeremonialBeast 5
 {EnemyFixedOp::Power,-2,3,3,4,(int)EnemyPower::Strength}, // CeremonialBeast 5
 {EnemyFixedOp::Attack,-2,8,8,9,2}, // Chomper 0
 {EnemyFixedOp::Card,2,3,3,3,Dazed}, // Chomper 1
 {EnemyFixedOp::Attack,-2,3,3,3,2}, // CorpseSlug 0
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // CorpseSlug 1
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // CorpseSlug 2
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // CrossbowRubyRaider 0
 {EnemyFixedOp::Block,-2,3,3,3,1}, // CrossbowRubyRaider 1
 {EnemyFixedOp::Attack,-2,12,12,14,1}, // Crusher 0
 {EnemyFixedOp::Attack,-2,4,4,4,1}, // Crusher 1
 {EnemyFixedOp::Attack,-2,6,6,7,2}, // Crusher 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Weak}, // Crusher 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // Crusher 2
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // Crusher 3
 {EnemyFixedOp::Attack,-2,12,12,14,1}, // Crusher 4
 {EnemyFixedOp::Block,-2,18,18,18,1}, // Crusher 4
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // CubexConstruct 0
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // CubexConstruct 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // CubexConstruct 1
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // CubexConstruct 2
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // CubexConstruct 2
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // CubexConstruct 3
 {EnemyFixedOp::Power,-2,5,5,6,(int)EnemyPower::Ritual}, // DampCultist 0
 {EnemyFixedOp::Attack,-2,1,1,3,1}, // DampCultist 1
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // DecimillipedeSegmentBack 0
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // DecimillipedeSegmentBack 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // DecimillipedeSegmentBack 1
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // DecimillipedeSegmentBack 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // DecimillipedeSegmentBack 2
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // DecimillipedeSegmentFront 0
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // DecimillipedeSegmentFront 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // DecimillipedeSegmentFront 1
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // DecimillipedeSegmentFront 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // DecimillipedeSegmentFront 2
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // DecimillipedeSegmentMiddle 0
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // DecimillipedeSegmentMiddle 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // DecimillipedeSegmentMiddle 1
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // DecimillipedeSegmentMiddle 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // DecimillipedeSegmentMiddle 2
 {EnemyFixedOp::Power,-2,9,9,9,(int)EnemyPower::Ritual}, // DevotedSculptor 0
 {EnemyFixedOp::Attack,-2,12,12,15,1}, // DevotedSculptor 1
 {EnemyFixedOp::Attack,-2,3,3,3,7}, // Entomancer 1
 {EnemyFixedOp::Attack,-2,18,18,20,1}, // Entomancer 2
 {EnemyFixedOp::Attack,-2,1,1,1,3}, // Exoskeleton 0
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // Exoskeleton 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // Exoskeleton 2
 {EnemyFixedOp::Card,2,3,3,3,Dazed}, // EyeWithTeeth 0
 {EnemyFixedOp::Attack,-2,11,11,13,1}, // Fabricator 2
 {EnemyFixedOp::Attack,-2,13,13,15,1}, // FakeMerchantMonster 0
 {EnemyFixedOp::Attack,-2,2,2,2,8}, // FakeMerchantMonster 1
 {EnemyFixedOp::Attack,-2,9,9,10,1}, // FakeMerchantMonster 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Frail}, // FakeMerchantMonster 2
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // FakeMerchantMonster 3
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // FlailKnight 0
 {EnemyFixedOp::Attack,-2,9,9,10,2}, // FlailKnight 1
 {EnemyFixedOp::Attack,-2,15,15,17,1}, // FlailKnight 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Vulnerable}, // Flyconid 0
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // Flyconid 1
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // Flyconid 1
 {EnemyFixedOp::Attack,-2,11,11,12,1}, // Flyconid 2
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // Fogmog 1
 {EnemyFixedOp::Power,-2,1,1,1,(int)EnemyPower::Strength}, // Fogmog 1
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // Fogmog 2
 {EnemyFixedOp::Power,-2,1,1,1,(int)EnemyPower::Strength}, // Fogmog 2
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // Fogmog 3
 {EnemyFixedOp::Attack,-2,9,9,11,1}, // FossilStalker 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Frail}, // FossilStalker 0
 {EnemyFixedOp::Attack,-2,12,12,14,1}, // FossilStalker 1
 {EnemyFixedOp::Attack,-2,3,3,4,2}, // FossilStalker 2
 {EnemyFixedOp::Power,-2,5,5,5,(int)EnemyPower::Strength}, // FrogKnight 0
 {EnemyFixedOp::Attack,-2,21,21,23,1}, // FrogKnight 1
 {EnemyFixedOp::Attack,-2,13,13,14,1}, // FrogKnight 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // FrogKnight 2
 {EnemyFixedOp::Attack,-2,35,35,40,1}, // FrogKnight 3
 {EnemyFixedOp::Attack,-2,4,4,6,1}, // FuzzyWurmCrawler 0
 {EnemyFixedOp::Attack,-2,4,4,6,1}, // FuzzyWurmCrawler 1
 {EnemyFixedOp::Power,-2,7,7,7,(int)EnemyPower::Strength}, // FuzzyWurmCrawler 2
 {EnemyFixedOp::Attack,-2,6,6,7,3}, // GlobeHead 0
 {EnemyFixedOp::Attack,-2,13,13,14,1}, // GlobeHead 1
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // GlobeHead 1
 {EnemyFixedOp::Attack,-2,16,16,17,1}, // GlobeHead 2
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // GlobeHead 2
 {EnemyFixedOp::Attack,-2,13,13,14,1}, // HauntedShip 0
 {EnemyFixedOp::Attack,-2,4,4,5,3}, // HauntedShip 1
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::Weak}, // HauntedShip 2
 {EnemyFixedOp::Card,2,5,5,5,Dazed}, // HauntedShip 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Tender}, // HunterKiller 0
 {EnemyFixedOp::Attack,-2,17,17,19,1}, // HunterKiller 1
 {EnemyFixedOp::Attack,-2,7,7,8,3}, // HunterKiller 2
 {EnemyFixedOp::Attack,-2,15,15,17,1}, // InfestedPrism 0
 {EnemyFixedOp::Attack,-2,11,11,13,1}, // InfestedPrism 1
 {EnemyFixedOp::Block,-2,11,11,13,1}, // InfestedPrism 1
 {EnemyFixedOp::Attack,-2,5,5,6,3}, // InfestedPrism 2
 {EnemyFixedOp::Attack,-2,8,8,10,1}, // InfestedPrism 3
 {EnemyFixedOp::Block,-2,20,22,22,1}, // InfestedPrism 3
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::VitalSpark}, // InfestedPrism 3
 {EnemyFixedOp::Attack,-2,3,3,4,1}, // Inklet 0
 {EnemyFixedOp::Attack,-2,2,2,3,3}, // Inklet 1
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // Inklet 2
 {EnemyFixedOp::Attack,-2,5,5,5,1}, // KinFollower 0
 {EnemyFixedOp::Attack,-2,2,2,2,2}, // KinFollower 1
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // KinFollower 2
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // KinPriest 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Frail}, // KinPriest 0
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // KinPriest 1
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // KinPriest 1
 {EnemyFixedOp::Attack,-2,3,3,3,3}, // KinPriest 2
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // KinPriest 3
 {EnemyFixedOp::Attack,-2,17,17,18,1}, // KnowledgeDemon 1
 {EnemyFixedOp::Attack,-2,8,8,9,3}, // KnowledgeDemon 2
 {EnemyFixedOp::Attack,-2,11,11,13,1}, // KnowledgeDemon 3
 {EnemyFixedOp::Heal,-2,30,30,30,0}, // KnowledgeDemon 3
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // KnowledgeDemon 3
 {EnemyFixedOp::Attack,-2,19,19,21,1}, // LagavulinMatriarch 1
 {EnemyFixedOp::Attack,-2,12,12,14,1}, // LagavulinMatriarch 2
 {EnemyFixedOp::Block,-2,12,14,14,1}, // LagavulinMatriarch 2
 {EnemyFixedOp::Attack,-2,9,9,10,2}, // LagavulinMatriarch 3
 {EnemyFixedOp::Power,-1,-2,-2,-2,(int)EnemyPower::Strength}, // LagavulinMatriarch 4
 {EnemyFixedOp::Power,-1,-2,-2,-2,(int)EnemyPower::Dexterity}, // LagavulinMatriarch 4
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // LagavulinMatriarch 4
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // LeafSlimeM 0
 {EnemyFixedOp::Card,2,2,2,2,Slimed}, // LeafSlimeM 1
 {EnemyFixedOp::Attack,-2,3,3,4,1}, // LeafSlimeS 0
 {EnemyFixedOp::Card,2,1,1,1,Slimed}, // LeafSlimeS 1
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // LivingFog 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Smoggy}, // LivingFog 0
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // LivingFog 2
 {EnemyFixedOp::Attack,-2,6,6,6,1}, // LivingShield 0
 {EnemyFixedOp::Attack,-2,16,16,18,1}, // LivingShield 1
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // LivingShield 1
 {EnemyFixedOp::Attack,-2,9,9,10,1}, // LouseProgenitor 0
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // LouseProgenitor 0
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // LouseProgenitor 1
 {EnemyFixedOp::Block,-2,14,18,18,1}, // LouseProgenitor 2
 {EnemyFixedOp::Power,-2,5,5,7,(int)EnemyPower::Strength}, // LouseProgenitor 2
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // MagiKnight 0
 {EnemyFixedOp::Block,-2,5,9,9,1}, // MagiKnight 0
 {EnemyFixedOp::Block,-2,5,9,9,1}, // MagiKnight 2
 {EnemyFixedOp::Attack,-2,35,35,40,1}, // MagiKnight 3
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // MagiKnight 4
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // Mawler 0
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::Vulnerable}, // Mawler 1
 {EnemyFixedOp::Attack,-2,4,4,5,2}, // Mawler 2
 {EnemyFixedOp::Attack,-2,25,25,30,1}, // MechaKnight 0
 {EnemyFixedOp::Attack,-2,8,8,12,1}, // MechaKnight 1
 {EnemyFixedOp::Card,1,4,4,4,Burn}, // MechaKnight 1
 {EnemyFixedOp::Block,-2,15,15,15,1}, // MechaKnight 2
 {EnemyFixedOp::Power,-2,5,5,5,(int)EnemyPower::Strength}, // MechaKnight 2
 {EnemyFixedOp::Attack,-2,35,35,40,1}, // MechaKnight 3
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // MysteriousKnight 0
 {EnemyFixedOp::Attack,-2,9,9,10,2}, // MysteriousKnight 1
 {EnemyFixedOp::Attack,-2,15,15,17,1}, // MysteriousKnight 2
 {EnemyFixedOp::Card,1,2,2,2,EventCardToxic}, // Myte 0
 {EnemyFixedOp::Attack,-2,13,13,15,1}, // Myte 1
 {EnemyFixedOp::Attack,-2,4,4,6,1}, // Myte 2
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // Myte 2
 {EnemyFixedOp::Attack,-2,12,12,13,1}, // Nibbit 0
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // Nibbit 1
 {EnemyFixedOp::Block,-2,5,6,6,1}, // Nibbit 1
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // Nibbit 2
 {EnemyFixedOp::Attack,-2,16,16,17,1}, // Ovicopter 1
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // Ovicopter 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Vulnerable}, // Ovicopter 2
 {EnemyFixedOp::Power,-2,3,3,4,(int)EnemyPower::Strength}, // Ovicopter 3
 {EnemyFixedOp::Attack,-2,16,16,17,1}, // OwlMagistrate 0
 {EnemyFixedOp::Attack,-2,4,4,4,6}, // OwlMagistrate 1
 {EnemyFixedOp::Power,-2,1,1,1,(int)EnemyPower::Soar}, // OwlMagistrate 2
 {EnemyFixedOp::Attack,-2,16,16,17,1}, // Parafright 0
 {EnemyFixedOp::Attack,-2,5,5,5,1}, // PhantasmalGardener 0
 {EnemyFixedOp::Attack,-2,7,7,7,1}, // PhantasmalGardener 1
 {EnemyFixedOp::Attack,-2,1,1,1,3}, // PhantasmalGardener 2
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // PhantasmalGardener 3
 {EnemyFixedOp::Card,2,3,3,3,EventCardInfection}, // PhrogParasite 0
 {EnemyFixedOp::Attack,-2,4,4,5,4}, // PhrogParasite 1
 {EnemyFixedOp::Block,-2,10,10,10,1}, // PunchConstruct 0
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // PunchConstruct 1
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // PunchConstruct 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Frail}, // PunchConstruct 2
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::ChainsOfBinding}, // Queen 0
 {EnemyFixedOp::Power,-1,99,99,99,(int)EnemyPower::Frail}, // Queen 1
 {EnemyFixedOp::Power,-1,99,99,99,(int)EnemyPower::Weak}, // Queen 1
 {EnemyFixedOp::Power,-1,99,99,99,(int)EnemyPower::Vulnerable}, // Queen 1
 {EnemyFixedOp::Attack,-2,3,3,4,5}, // Queen 3
 {EnemyFixedOp::Attack,-2,15,15,18,1}, // Queen 4
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // Queen 5
 {EnemyFixedOp::Attack,-2,3,3,4,1}, // Rocket 0
 {EnemyFixedOp::Attack,-2,18,18,20,1}, // Rocket 1
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // Rocket 2
 {EnemyFixedOp::Attack,-2,31,31,35,1}, // Rocket 3
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // ScrollOfBiting 0
 {EnemyFixedOp::Attack,-2,5,5,6,2}, // ScrollOfBiting 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // ScrollOfBiting 2
 {EnemyFixedOp::Attack,-2,11,11,13,1}, // Seapunk 0
 {EnemyFixedOp::Attack,-2,2,2,2,4}, // Seapunk 1
 {EnemyFixedOp::Block,-2,7,8,8,1}, // Seapunk 2
 {EnemyFixedOp::Power,-2,1,1,2,(int)EnemyPower::Strength}, // Seapunk 2
 {EnemyFixedOp::Power,-2,4,4,4,(int)EnemyPower::Strength}, // SewerClam 0
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // SewerClam 1
 {EnemyFixedOp::Power,-1,-1,-1,-1,(int)EnemyPower::Shrink}, // ShrinkerBeetle 0
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // ShrinkerBeetle 1
 {EnemyFixedOp::Attack,-2,13,13,14,1}, // ShrinkerBeetle 2
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // SkulkingColony 0
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // SkulkingColony 1
 {EnemyFixedOp::Attack,-2,9,9,11,1}, // SkulkingColony 2
 {EnemyFixedOp::Power,-2,2,2,4,(int)EnemyPower::Strength}, // SkulkingColony 2
 {EnemyFixedOp::Attack,-2,7,7,8,2}, // SkulkingColony 3
 {EnemyFixedOp::Card,2,10,10,10,Slimed}, // SlimedBerserker 0
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::Weak}, // SlimedBerserker 1
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // SlimedBerserker 1
 {EnemyFixedOp::Attack,-2,30,30,33,1}, // SlimedBerserker 2
 {EnemyFixedOp::Attack,-2,4,4,5,4}, // SlimedBerserker 3
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::Constrict}, // SlitheringStrangler 0
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // SlitheringStrangler 1
 {EnemyFixedOp::Block,-2,5,5,5,1}, // SlitheringStrangler 1
 {EnemyFixedOp::Attack,-2,12,12,13,1}, // SlitheringStrangler 2
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // SludgeSpinner 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // SludgeSpinner 0
 {EnemyFixedOp::Attack,-2,11,11,12,1}, // SludgeSpinner 1
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // SludgeSpinner 2
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::Strength}, // SludgeSpinner 2
 {EnemyFixedOp::Attack,-2,16,16,18,1}, // SlumberingBeetle 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // SlumberingBeetle 1
 {EnemyFixedOp::Attack,-2,3,3,4,1}, // SnappingJaxfruit 0
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // SnappingJaxfruit 0
 {EnemyFixedOp::Attack,-2,9,9,10,1}, // SneakyGremlin 1
 {EnemyFixedOp::Attack,-2,16,16,18,1}, // SoulFysh 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Intangible}, // SoulFysh 3
 {EnemyFixedOp::Attack,-2,13,13,15,1}, // SoulFysh 4
 {EnemyFixedOp::Power,-1,3,3,3,(int)EnemyPower::Vulnerable}, // SoulFysh 4
 {EnemyFixedOp::Attack,-2,29,29,31,1}, // SoulNexus 0
 {EnemyFixedOp::Attack,-2,6,6,7,4}, // SoulNexus 1
 {EnemyFixedOp::Attack,-2,18,18,19,1}, // SoulNexus 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Vulnerable}, // SoulNexus 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Weak}, // SoulNexus 2
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Hex}, // SpectralKnight 0
 {EnemyFixedOp::Attack,-2,15,15,17,1}, // SpectralKnight 1
 {EnemyFixedOp::Attack,-2,3,3,4,3}, // SpectralKnight 2
 {EnemyFixedOp::Power,-2,5,5,5,(int)EnemyPower::Thorns}, // SpinyToad 0
 {EnemyFixedOp::Attack,-2,23,23,25,1}, // SpinyToad 1
 {EnemyFixedOp::Power,-2,-5,-5,-5,(int)EnemyPower::Thorns}, // SpinyToad 1
 {EnemyFixedOp::Attack,-2,17,17,19,1}, // SpinyToad 2
 {EnemyFixedOp::Attack,-2,11,11,12,1}, // Stabbot 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Frail}, // Stabbot 0
 {EnemyFixedOp::Attack,-2,16,16,18,1}, // TerrorEel 0
 {EnemyFixedOp::Attack,-2,3,3,4,3}, // TerrorEel 1
 {EnemyFixedOp::Power,-2,6,6,6,(int)EnemyPower::Vigor}, // TerrorEel 1
 {EnemyFixedOp::Power,-1,99,99,99,(int)EnemyPower::Vulnerable}, // TerrorEel 3
 {EnemyFixedOp::Attack,-2,20,20,22,1}, // TestSubject 1
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // TestSubject 2
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Vulnerable}, // TestSubject 2
 {EnemyFixedOp::Attack,-2,10,10,11,3}, // TestSubject 4
 {EnemyFixedOp::Attack,-2,45,45,45,1}, // TestSubject 5
 {EnemyFixedOp::Card,2,3,3,5,Burn}, // TestSubject 6
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // TestSubject 6
 {EnemyFixedOp::Power,-1,-2,-2,-2,(int)EnemyPower::Dexterity}, // TheForgotten 0
 {EnemyFixedOp::Block,-2,8,8,8,1}, // TheForgotten 0
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Dexterity}, // TheForgotten 0
 {EnemyFixedOp::Attack,-2,8,8,9,2}, // TheInsatiable 1
 {EnemyFixedOp::Attack,-2,8,8,9,2}, // TheInsatiable 2
 {EnemyFixedOp::Attack,-2,28,28,31,1}, // TheInsatiable 3
 {EnemyFixedOp::Power,-2,2,2,3,(int)EnemyPower::Strength}, // TheInsatiable 4
 {EnemyFixedOp::Power,-1,-2,-2,-2,(int)EnemyPower::Strength}, // TheLost 0
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // TheLost 0
 {EnemyFixedOp::Attack,-2,4,4,5,2}, // TheLost 1
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // TheObscura 1
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // TheObscura 3
 {EnemyFixedOp::Block,-2,6,6,7,1}, // TheObscura 3
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // ThievingHopper 1
 {EnemyFixedOp::Attack,-2,21,21,23,1}, // ThievingHopper 2
 {EnemyFixedOp::Power,-2,5,5,5,(int)EnemyPower::Flutter}, // ThievingHopper 3
 {EnemyFixedOp::Power,-2,-2,-2,-2,(int)EnemyPower::Thorns}, // Toadpole 0
 {EnemyFixedOp::Attack,-2,3,3,4,3}, // Toadpole 0
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // Toadpole 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Thorns}, // Toadpole 2
 {EnemyFixedOp::Attack,-2,26,26,32,1}, // TorchHeadAmalgam 0
 {EnemyFixedOp::Attack,-2,18,18,22,1}, // TorchHeadAmalgam 1
 {EnemyFixedOp::Attack,-2,8,8,8,3}, // TorchHeadAmalgam 2
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // TorchHeadAmalgam 3
 {EnemyFixedOp::Attack,-2,14,14,16,1}, // TorchHeadAmalgam 4
 {EnemyFixedOp::Attack,-2,4,4,5,1}, // ToughEgg 1
 {EnemyFixedOp::Power,-1,2,2,2,(int)EnemyPower::Frail}, // TrackerRubyRaider 0
 {EnemyFixedOp::Attack,-2,1,1,1,8}, // TrackerRubyRaider 1
 {EnemyFixedOp::Attack,-2,13,13,15,1}, // Tunneler 0
 {EnemyFixedOp::Power,-2,1,1,1,(int)EnemyPower::Burrowed}, // Tunneler 1
 {EnemyFixedOp::Block,-2,32,37,37,1}, // Tunneler 1
 {EnemyFixedOp::Attack,-2,23,23,26,1}, // Tunneler 2
 {EnemyFixedOp::Attack,-2,3,3,4,5}, // TurretOperator 0
 {EnemyFixedOp::Attack,-2,3,3,4,5}, // TurretOperator 1
 {EnemyFixedOp::Power,-2,1,1,1,(int)EnemyPower::Strength}, // TurretOperator 2
 {EnemyFixedOp::Attack,-2,11,11,12,1}, // TwigSlimeM 0
 {EnemyFixedOp::Card,2,1,1,1,Slimed}, // TwigSlimeM 1
 {EnemyFixedOp::Attack,-2,4,4,5,1}, // TwigSlimeS 0
 {EnemyFixedOp::Attack,-2,7,7,8,1}, // Vantom 0
 {EnemyFixedOp::Attack,-2,6,6,7,2}, // Vantom 1
 {EnemyFixedOp::Attack,-2,26,26,30,1}, // Vantom 2
 {EnemyFixedOp::Card,2,3,3,3,Wound}, // Vantom 2
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // Vantom 3
 {EnemyFixedOp::Attack,-2,8,8,9,1}, // VineShambler 0
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Tangled}, // VineShambler 0
 {EnemyFixedOp::Attack,-2,6,6,7,2}, // VineShambler 1
 {EnemyFixedOp::Attack,-2,16,16,18,1}, // VineShambler 2
 {EnemyFixedOp::Power,-2,15,15,20,(int)EnemyPower::SteamEruption}, // WaterfallGiant 0
 {EnemyFixedOp::Attack,-2,15,15,16,1}, // WaterfallGiant 1
 {EnemyFixedOp::Power,-1,1,1,1,(int)EnemyPower::Weak}, // WaterfallGiant 1
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::SteamEruption}, // WaterfallGiant 1
 {EnemyFixedOp::Attack,-2,10,10,11,1}, // WaterfallGiant 2
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::SteamEruption}, // WaterfallGiant 2
 {EnemyFixedOp::Heal,-2,10,15,15,0}, // WaterfallGiant 3
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::SteamEruption}, // WaterfallGiant 3
 {EnemyFixedOp::Attack,-2,13,13,14,1}, // WaterfallGiant 5
 {EnemyFixedOp::Power,-2,3,3,3,(int)EnemyPower::SteamEruption}, // WaterfallGiant 5
 {EnemyFixedOp::Attack,-2,6,6,7,1}, // Wriggler 0
 {EnemyFixedOp::Card,2,1,1,1,EventCardInfection}, // Wriggler 1
 {EnemyFixedOp::Power,-2,2,2,2,(int)EnemyPower::Strength}, // Wriggler 1
 {EnemyFixedOp::Attack,-2,14,14,15,1}, // Zapbot 0
};
struct EnemyFixedSpan {uint16_t first;uint8_t count;};
static const EnemyFixedSpan ef_Aeonglass[]={{0,2},{2,1},{0,255}};
static const EnemyFixedSpan ef_Architect[]={{3,0}};
static const EnemyFixedSpan ef_AssassinRubyRaider[]={{3,1}};
static const EnemyFixedSpan ef_AxeRubyRaider[]={{4,2},{6,2},{8,1}};
static const EnemyFixedSpan ef_Axebot[]={{0,255},{9,1},{10,3}};
static const EnemyFixedSpan ef_BattleFriendV1[]={{13,0}};
static const EnemyFixedSpan ef_BattleFriendV2[]={{13,0}};
static const EnemyFixedSpan ef_BattleFriendV3[]={{13,0}};
static const EnemyFixedSpan ef_BowlbugEgg[]={{13,2}};
static const EnemyFixedSpan ef_BowlbugNectar[]={{15,1},{16,1},{17,1}};
static const EnemyFixedSpan ef_BowlbugRock[]={{0,255},{18,0}};
static const EnemyFixedSpan ef_BowlbugSilk[]={{18,1},{19,1}};
static const EnemyFixedSpan ef_BruteRubyRaider[]={{20,1},{21,1}};
static const EnemyFixedSpan ef_BygoneEffigy[]={{22,0},{22,1},{23,0},{23,1}};
static const EnemyFixedSpan ef_Byrdonis[]={{24,1},{25,1}};
static const EnemyFixedSpan ef_CalcifiedCultist[]={{26,1},{27,1}};
static const EnemyFixedSpan ef_CeremonialBeast[]={{28,1},{29,2},{31,0},{31,1},{32,1},{33,2}};
static const EnemyFixedSpan ef_Chomper[]={{35,1},{36,1}};
static const EnemyFixedSpan ef_CorpseSlug[]={{37,1},{38,1},{39,1}};
static const EnemyFixedSpan ef_CrossbowRubyRaider[]={{40,1},{41,1}};
static const EnemyFixedSpan ef_Crusher[]={{42,1},{43,1},{44,3},{47,1},{48,2}};
static const EnemyFixedSpan ef_CubexConstruct[]={{50,1},{51,2},{53,2},{55,1}};
static const EnemyFixedSpan ef_DampCultist[]={{56,1},{57,1}};
static const EnemyFixedSpan ef_DecimillipedeSegmentBack[]={{58,1},{59,2},{61,2},{63,0},{0,255}};
static const EnemyFixedSpan ef_DecimillipedeSegmentFront[]={{63,1},{64,2},{66,2},{68,0},{0,255}};
static const EnemyFixedSpan ef_DecimillipedeSegmentMiddle[]={{68,1},{69,2},{71,2},{73,0},{0,255}};
static const EnemyFixedSpan ef_DevotedSculptor[]={{73,1},{74,1}};
static const EnemyFixedSpan ef_Entomancer[]={{0,255},{75,1},{76,1}};
static const EnemyFixedSpan ef_Exoskeleton[]={{77,1},{78,1},{79,1}};
static const EnemyFixedSpan ef_EyeWithTeeth[]={{80,1}};
static const EnemyFixedSpan ef_Fabricator[]={{0,255},{0,255},{81,1}};
static const EnemyFixedSpan ef_FakeMerchantMonster[]={{82,1},{83,1},{84,2},{86,1}};
static const EnemyFixedSpan ef_FatGremlin[]={{87,0},{0,255}};
static const EnemyFixedSpan ef_FlailKnight[]={{87,1},{88,1},{89,1}};
static const EnemyFixedSpan ef_Flyconid[]={{90,1},{91,2},{93,1}};
static const EnemyFixedSpan ef_Fogmog[]={{0,255},{94,2},{96,2},{98,1}};
static const EnemyFixedSpan ef_FossilStalker[]={{99,2},{101,1},{102,1}};
static const EnemyFixedSpan ef_FrogKnight[]={{103,1},{104,1},{105,2},{107,1}};
static const EnemyFixedSpan ef_FuzzyWurmCrawler[]={{108,1},{109,1},{110,1}};
static const EnemyFixedSpan ef_GasBomb[]={{0,255}};
static const EnemyFixedSpan ef_GlobeHead[]={{111,1},{112,2},{114,2}};
static const EnemyFixedSpan ef_GremlinMerc[]={{0,255},{0,255},{0,255}};
static const EnemyFixedSpan ef_Guardbot[]={{0,255}};
static const EnemyFixedSpan ef_HauntedShip[]={{116,1},{117,1},{118,2}};
static const EnemyFixedSpan ef_HunterKiller[]={{120,1},{121,1},{122,1}};
static const EnemyFixedSpan ef_InfestedPrism[]={{123,1},{124,2},{126,1},{127,3}};
static const EnemyFixedSpan ef_Inklet[]={{130,1},{131,1},{132,1}};
static const EnemyFixedSpan ef_KinFollower[]={{133,1},{134,1},{135,1}};
static const EnemyFixedSpan ef_KinPriest[]={{136,2},{138,2},{140,1},{141,1}};
static const EnemyFixedSpan ef_KnowledgeDemon[]={{0,255},{142,1},{143,1},{144,3}};
static const EnemyFixedSpan ef_LagavulinMatriarch[]={{147,0},{147,1},{148,2},{150,1},{151,3}};
static const EnemyFixedSpan ef_LeafSlimeM[]={{154,1},{155,1}};
static const EnemyFixedSpan ef_LeafSlimeS[]={{156,1},{157,1}};
static const EnemyFixedSpan ef_LivingFog[]={{158,2},{0,255},{160,1}};
static const EnemyFixedSpan ef_LivingShield[]={{161,1},{162,2}};
static const EnemyFixedSpan ef_LouseProgenitor[]={{164,2},{166,1},{167,2}};
static const EnemyFixedSpan ef_MagiKnight[]={{169,2},{0,255},{171,1},{172,1},{173,1}};
static const EnemyFixedSpan ef_Mawler[]={{174,1},{175,1},{176,1}};
static const EnemyFixedSpan ef_MechaKnight[]={{177,1},{178,2},{180,2},{182,1}};
static const EnemyFixedSpan ef_MysteriousKnight[]={{183,1},{184,1},{185,1}};
static const EnemyFixedSpan ef_Myte[]={{186,1},{187,1},{188,2}};
static const EnemyFixedSpan ef_Nibbit[]={{190,1},{191,2},{193,1}};
static const EnemyFixedSpan ef_Noisebot[]={{0,255}};
static const EnemyFixedSpan ef_Ovicopter[]={{0,255},{194,1},{195,2},{197,1}};
static const EnemyFixedSpan ef_OwlMagistrate[]={{198,1},{199,1},{200,1},{0,255}};
static const EnemyFixedSpan ef_Parafright[]={{201,1}};
static const EnemyFixedSpan ef_PhantasmalGardener[]={{202,1},{203,1},{204,1},{205,1}};
static const EnemyFixedSpan ef_PhrogParasite[]={{206,1},{207,1}};
static const EnemyFixedSpan ef_PunchConstruct[]={{208,1},{209,1},{210,2}};
static const EnemyFixedSpan ef_Queen[]={{212,1},{213,3},{0,255},{216,1},{217,1},{218,1}};
static const EnemyFixedSpan ef_Rocket[]={{219,1},{220,1},{221,1},{222,1},{223,0}};
static const EnemyFixedSpan ef_ScrollOfBiting[]={{223,1},{224,1},{225,1}};
static const EnemyFixedSpan ef_Seapunk[]={{226,1},{227,1},{228,2}};
static const EnemyFixedSpan ef_SewerClam[]={{230,1},{231,1}};
static const EnemyFixedSpan ef_ShrinkerBeetle[]={{232,1},{233,1},{234,1}};
static const EnemyFixedSpan ef_SkulkingColony[]={{235,1},{236,1},{237,2},{239,1}};
static const EnemyFixedSpan ef_SlimedBerserker[]={{240,1},{241,2},{243,1},{244,1}};
static const EnemyFixedSpan ef_SlitheringStrangler[]={{245,1},{246,2},{248,1}};
static const EnemyFixedSpan ef_SludgeSpinner[]={{249,2},{251,1},{252,2}};
static const EnemyFixedSpan ef_SlumberingBeetle[]={{254,0},{254,2}};
static const EnemyFixedSpan ef_SnappingJaxfruit[]={{256,2}};
static const EnemyFixedSpan ef_SneakyGremlin[]={{258,0},{258,1}};
static const EnemyFixedSpan ef_SoulFysh[]={{0,255},{259,1},{0,255},{260,1},{261,2}};
static const EnemyFixedSpan ef_SoulNexus[]={{263,1},{264,1},{265,3}};
static const EnemyFixedSpan ef_SpectralKnight[]={{268,1},{269,1},{270,1}};
static const EnemyFixedSpan ef_SpinyToad[]={{271,1},{272,2},{274,1}};
static const EnemyFixedSpan ef_Stabbot[]={{275,2}};
static const EnemyFixedSpan ef_TerrorEel[]={{277,1},{278,2},{280,0},{280,1}};
static const EnemyFixedSpan ef_TestSubject[]={{0,255},{281,1},{282,2},{0,255},{284,1},{285,1},{286,2}};
static const EnemyFixedSpan ef_TheForgotten[]={{288,3},{0,255}};
static const EnemyFixedSpan ef_TheInsatiable[]={{0,255},{291,1},{292,1},{293,1},{294,1}};
static const EnemyFixedSpan ef_TheLost[]={{295,2},{297,1}};
static const EnemyFixedSpan ef_TheObscura[]={{0,255},{298,1},{0,255},{299,2}};
static const EnemyFixedSpan ef_ThievingHopper[]={{0,255},{301,1},{302,1},{303,1},{0,255}};
static const EnemyFixedSpan ef_Toadpole[]={{304,2},{306,1},{307,1}};
static const EnemyFixedSpan ef_TorchHeadAmalgam[]={{308,1},{309,1},{310,1},{311,1},{312,1}};
static const EnemyFixedSpan ef_ToughEgg[]={{0,255},{313,1}};
static const EnemyFixedSpan ef_TrackerRubyRaider[]={{314,1},{315,1}};
static const EnemyFixedSpan ef_Tunneler[]={{316,1},{317,2},{319,1},{320,0}};
static const EnemyFixedSpan ef_TurretOperator[]={{320,1},{321,1},{322,1}};
static const EnemyFixedSpan ef_TwigSlimeM[]={{323,1},{324,1}};
static const EnemyFixedSpan ef_TwigSlimeS[]={{325,1}};
static const EnemyFixedSpan ef_TwoTailedRat[]={{0,255},{0,255},{0,255},{0,255}};
static const EnemyFixedSpan ef_Vantom[]={{326,1},{327,1},{328,2},{330,1}};
static const EnemyFixedSpan ef_VineShambler[]={{331,2},{333,1},{334,1}};
static const EnemyFixedSpan ef_WaterfallGiant[]={{335,1},{336,3},{339,2},{341,2},{0,255},{343,2},{0,255},{0,255}};
static const EnemyFixedSpan ef_Wriggler[]={{345,1},{346,2},{348,0}};
static const EnemyFixedSpan ef_Zapbot[]={{348,1}};
static EnemyFixedSpan enemy_fixed_span(EnemyId k,unsigned n){switch(k){
 case EnemyId::Aeonglass:return n<3?ef_Aeonglass[n]:EnemyFixedSpan{0,255};
 case EnemyId::Architect:return n<1?ef_Architect[n]:EnemyFixedSpan{0,255};
 case EnemyId::AssassinRubyRaider:return n<1?ef_AssassinRubyRaider[n]:EnemyFixedSpan{0,255};
 case EnemyId::AxeRubyRaider:return n<3?ef_AxeRubyRaider[n]:EnemyFixedSpan{0,255};
 case EnemyId::Axebot:return n<3?ef_Axebot[n]:EnemyFixedSpan{0,255};
 case EnemyId::BattleFriendV1:return n<1?ef_BattleFriendV1[n]:EnemyFixedSpan{0,255};
 case EnemyId::BattleFriendV2:return n<1?ef_BattleFriendV2[n]:EnemyFixedSpan{0,255};
 case EnemyId::BattleFriendV3:return n<1?ef_BattleFriendV3[n]:EnemyFixedSpan{0,255};
 case EnemyId::BowlbugEgg:return n<1?ef_BowlbugEgg[n]:EnemyFixedSpan{0,255};
 case EnemyId::BowlbugNectar:return n<3?ef_BowlbugNectar[n]:EnemyFixedSpan{0,255};
 case EnemyId::BowlbugRock:return n<2?ef_BowlbugRock[n]:EnemyFixedSpan{0,255};
 case EnemyId::BowlbugSilk:return n<2?ef_BowlbugSilk[n]:EnemyFixedSpan{0,255};
 case EnemyId::BruteRubyRaider:return n<2?ef_BruteRubyRaider[n]:EnemyFixedSpan{0,255};
 case EnemyId::BygoneEffigy:return n<4?ef_BygoneEffigy[n]:EnemyFixedSpan{0,255};
 case EnemyId::Byrdonis:return n<2?ef_Byrdonis[n]:EnemyFixedSpan{0,255};
 case EnemyId::CalcifiedCultist:return n<2?ef_CalcifiedCultist[n]:EnemyFixedSpan{0,255};
 case EnemyId::CeremonialBeast:return n<6?ef_CeremonialBeast[n]:EnemyFixedSpan{0,255};
 case EnemyId::Chomper:return n<2?ef_Chomper[n]:EnemyFixedSpan{0,255};
 case EnemyId::CorpseSlug:return n<3?ef_CorpseSlug[n]:EnemyFixedSpan{0,255};
 case EnemyId::CrossbowRubyRaider:return n<2?ef_CrossbowRubyRaider[n]:EnemyFixedSpan{0,255};
 case EnemyId::Crusher:return n<5?ef_Crusher[n]:EnemyFixedSpan{0,255};
 case EnemyId::CubexConstruct:return n<4?ef_CubexConstruct[n]:EnemyFixedSpan{0,255};
 case EnemyId::DampCultist:return n<2?ef_DampCultist[n]:EnemyFixedSpan{0,255};
 case EnemyId::DecimillipedeSegmentBack:return n<5?ef_DecimillipedeSegmentBack[n]:EnemyFixedSpan{0,255};
 case EnemyId::DecimillipedeSegmentFront:return n<5?ef_DecimillipedeSegmentFront[n]:EnemyFixedSpan{0,255};
 case EnemyId::DecimillipedeSegmentMiddle:return n<5?ef_DecimillipedeSegmentMiddle[n]:EnemyFixedSpan{0,255};
 case EnemyId::DevotedSculptor:return n<2?ef_DevotedSculptor[n]:EnemyFixedSpan{0,255};
 case EnemyId::Entomancer:return n<3?ef_Entomancer[n]:EnemyFixedSpan{0,255};
 case EnemyId::Exoskeleton:return n<3?ef_Exoskeleton[n]:EnemyFixedSpan{0,255};
 case EnemyId::EyeWithTeeth:return n<1?ef_EyeWithTeeth[n]:EnemyFixedSpan{0,255};
 case EnemyId::Fabricator:return n<3?ef_Fabricator[n]:EnemyFixedSpan{0,255};
 case EnemyId::FakeMerchantMonster:return n<4?ef_FakeMerchantMonster[n]:EnemyFixedSpan{0,255};
 case EnemyId::FatGremlin:return n<2?ef_FatGremlin[n]:EnemyFixedSpan{0,255};
 case EnemyId::FlailKnight:return n<3?ef_FlailKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::Flyconid:return n<3?ef_Flyconid[n]:EnemyFixedSpan{0,255};
 case EnemyId::Fogmog:return n<4?ef_Fogmog[n]:EnemyFixedSpan{0,255};
 case EnemyId::FossilStalker:return n<3?ef_FossilStalker[n]:EnemyFixedSpan{0,255};
 case EnemyId::FrogKnight:return n<4?ef_FrogKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::FuzzyWurmCrawler:return n<3?ef_FuzzyWurmCrawler[n]:EnemyFixedSpan{0,255};
 case EnemyId::GasBomb:return n<1?ef_GasBomb[n]:EnemyFixedSpan{0,255};
 case EnemyId::GlobeHead:return n<3?ef_GlobeHead[n]:EnemyFixedSpan{0,255};
 case EnemyId::GremlinMerc:return n<3?ef_GremlinMerc[n]:EnemyFixedSpan{0,255};
 case EnemyId::Guardbot:return n<1?ef_Guardbot[n]:EnemyFixedSpan{0,255};
 case EnemyId::HauntedShip:return n<3?ef_HauntedShip[n]:EnemyFixedSpan{0,255};
 case EnemyId::HunterKiller:return n<3?ef_HunterKiller[n]:EnemyFixedSpan{0,255};
 case EnemyId::InfestedPrism:return n<4?ef_InfestedPrism[n]:EnemyFixedSpan{0,255};
 case EnemyId::Inklet:return n<3?ef_Inklet[n]:EnemyFixedSpan{0,255};
 case EnemyId::KinFollower:return n<3?ef_KinFollower[n]:EnemyFixedSpan{0,255};
 case EnemyId::KinPriest:return n<4?ef_KinPriest[n]:EnemyFixedSpan{0,255};
 case EnemyId::KnowledgeDemon:return n<4?ef_KnowledgeDemon[n]:EnemyFixedSpan{0,255};
 case EnemyId::LagavulinMatriarch:return n<5?ef_LagavulinMatriarch[n]:EnemyFixedSpan{0,255};
 case EnemyId::LeafSlimeM:return n<2?ef_LeafSlimeM[n]:EnemyFixedSpan{0,255};
 case EnemyId::LeafSlimeS:return n<2?ef_LeafSlimeS[n]:EnemyFixedSpan{0,255};
 case EnemyId::LivingFog:return n<3?ef_LivingFog[n]:EnemyFixedSpan{0,255};
 case EnemyId::LivingShield:return n<2?ef_LivingShield[n]:EnemyFixedSpan{0,255};
 case EnemyId::LouseProgenitor:return n<3?ef_LouseProgenitor[n]:EnemyFixedSpan{0,255};
 case EnemyId::MagiKnight:return n<5?ef_MagiKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::Mawler:return n<3?ef_Mawler[n]:EnemyFixedSpan{0,255};
 case EnemyId::MechaKnight:return n<4?ef_MechaKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::MysteriousKnight:return n<3?ef_MysteriousKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::Myte:return n<3?ef_Myte[n]:EnemyFixedSpan{0,255};
 case EnemyId::Nibbit:return n<3?ef_Nibbit[n]:EnemyFixedSpan{0,255};
 case EnemyId::Noisebot:return n<1?ef_Noisebot[n]:EnemyFixedSpan{0,255};
 case EnemyId::Ovicopter:return n<4?ef_Ovicopter[n]:EnemyFixedSpan{0,255};
 case EnemyId::OwlMagistrate:return n<4?ef_OwlMagistrate[n]:EnemyFixedSpan{0,255};
 case EnemyId::Parafright:return n<1?ef_Parafright[n]:EnemyFixedSpan{0,255};
 case EnemyId::PhantasmalGardener:return n<4?ef_PhantasmalGardener[n]:EnemyFixedSpan{0,255};
 case EnemyId::PhrogParasite:return n<2?ef_PhrogParasite[n]:EnemyFixedSpan{0,255};
 case EnemyId::PunchConstruct:return n<3?ef_PunchConstruct[n]:EnemyFixedSpan{0,255};
 case EnemyId::Queen:return n<6?ef_Queen[n]:EnemyFixedSpan{0,255};
 case EnemyId::Rocket:return n<5?ef_Rocket[n]:EnemyFixedSpan{0,255};
 case EnemyId::ScrollOfBiting:return n<3?ef_ScrollOfBiting[n]:EnemyFixedSpan{0,255};
 case EnemyId::Seapunk:return n<3?ef_Seapunk[n]:EnemyFixedSpan{0,255};
 case EnemyId::SewerClam:return n<2?ef_SewerClam[n]:EnemyFixedSpan{0,255};
 case EnemyId::ShrinkerBeetle:return n<3?ef_ShrinkerBeetle[n]:EnemyFixedSpan{0,255};
 case EnemyId::SkulkingColony:return n<4?ef_SkulkingColony[n]:EnemyFixedSpan{0,255};
 case EnemyId::SlimedBerserker:return n<4?ef_SlimedBerserker[n]:EnemyFixedSpan{0,255};
 case EnemyId::SlitheringStrangler:return n<3?ef_SlitheringStrangler[n]:EnemyFixedSpan{0,255};
 case EnemyId::SludgeSpinner:return n<3?ef_SludgeSpinner[n]:EnemyFixedSpan{0,255};
 case EnemyId::SlumberingBeetle:return n<2?ef_SlumberingBeetle[n]:EnemyFixedSpan{0,255};
 case EnemyId::SnappingJaxfruit:return n<1?ef_SnappingJaxfruit[n]:EnemyFixedSpan{0,255};
 case EnemyId::SneakyGremlin:return n<2?ef_SneakyGremlin[n]:EnemyFixedSpan{0,255};
 case EnemyId::SoulFysh:return n<5?ef_SoulFysh[n]:EnemyFixedSpan{0,255};
 case EnemyId::SoulNexus:return n<3?ef_SoulNexus[n]:EnemyFixedSpan{0,255};
 case EnemyId::SpectralKnight:return n<3?ef_SpectralKnight[n]:EnemyFixedSpan{0,255};
 case EnemyId::SpinyToad:return n<3?ef_SpinyToad[n]:EnemyFixedSpan{0,255};
 case EnemyId::Stabbot:return n<1?ef_Stabbot[n]:EnemyFixedSpan{0,255};
 case EnemyId::TerrorEel:return n<4?ef_TerrorEel[n]:EnemyFixedSpan{0,255};
 case EnemyId::TestSubject:return n<7?ef_TestSubject[n]:EnemyFixedSpan{0,255};
 case EnemyId::TheForgotten:return n<2?ef_TheForgotten[n]:EnemyFixedSpan{0,255};
 case EnemyId::TheInsatiable:return n<5?ef_TheInsatiable[n]:EnemyFixedSpan{0,255};
 case EnemyId::TheLost:return n<2?ef_TheLost[n]:EnemyFixedSpan{0,255};
 case EnemyId::TheObscura:return n<4?ef_TheObscura[n]:EnemyFixedSpan{0,255};
 case EnemyId::ThievingHopper:return n<5?ef_ThievingHopper[n]:EnemyFixedSpan{0,255};
 case EnemyId::Toadpole:return n<3?ef_Toadpole[n]:EnemyFixedSpan{0,255};
 case EnemyId::TorchHeadAmalgam:return n<5?ef_TorchHeadAmalgam[n]:EnemyFixedSpan{0,255};
 case EnemyId::ToughEgg:return n<2?ef_ToughEgg[n]:EnemyFixedSpan{0,255};
 case EnemyId::TrackerRubyRaider:return n<2?ef_TrackerRubyRaider[n]:EnemyFixedSpan{0,255};
 case EnemyId::Tunneler:return n<4?ef_Tunneler[n]:EnemyFixedSpan{0,255};
 case EnemyId::TurretOperator:return n<3?ef_TurretOperator[n]:EnemyFixedSpan{0,255};
 case EnemyId::TwigSlimeM:return n<2?ef_TwigSlimeM[n]:EnemyFixedSpan{0,255};
 case EnemyId::TwigSlimeS:return n<1?ef_TwigSlimeS[n]:EnemyFixedSpan{0,255};
 case EnemyId::TwoTailedRat:return n<4?ef_TwoTailedRat[n]:EnemyFixedSpan{0,255};
 case EnemyId::Vantom:return n<4?ef_Vantom[n]:EnemyFixedSpan{0,255};
 case EnemyId::VineShambler:return n<3?ef_VineShambler[n]:EnemyFixedSpan{0,255};
 case EnemyId::WaterfallGiant:return n<8?ef_WaterfallGiant[n]:EnemyFixedSpan{0,255};
 case EnemyId::Wriggler:return n<3?ef_Wriggler[n]:EnemyFixedSpan{0,255};
 case EnemyId::Zapbot:return n<1?ef_Zapbot[n]:EnemyFixedSpan{0,255};
 default:return {0,255};}}
