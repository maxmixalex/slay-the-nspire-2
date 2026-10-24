#pragma once
#include "core.hpp"
namespace spire {
// AttackCommand facts from v0.111.0 / 41cef1ea, joined to the existing
// audited move/node directory. Earlier awaits inside a move stay separate.
struct CombatAttackTiming {unsigned delay=0,wait=0;bool once=false,known=false;};
inline CombatAttackTiming combat_enemy_attack_timing(EnemyId kind,int move,bool fast){
 struct Row {uint16_t key,delay,normal_wait,fast_wait;bool once;};
 static constexpr Row rows[]={
 {uint16_t(unsigned(EnemyId::Aeonglass)*32+0),300,0,0,false}, // EbbMove
 {uint16_t(unsigned(EnemyId::Aeonglass)*32+1),400,0,0,true}, // EyeLasersMove
 {uint16_t(unsigned(EnemyId::AssassinRubyRaider)*32+0),500,0,0,false}, // KillshotMove
 {uint16_t(unsigned(EnemyId::AxeRubyRaider)*32+0),500,0,0,false}, // SwingMove
 {uint16_t(unsigned(EnemyId::AxeRubyRaider)*32+1),500,0,0,false}, // SwingMove
 {uint16_t(unsigned(EnemyId::AxeRubyRaider)*32+2),500,0,0,false}, // BigSwingMove
 {uint16_t(unsigned(EnemyId::Axebot)*32+1),350,0,0,true}, // OneTwoMove
 {uint16_t(unsigned(EnemyId::Axebot)*32+2),800,0,0,false}, // HammerUppercutMove
 {uint16_t(unsigned(EnemyId::BowlbugEgg)*32+0),300,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::BowlbugNectar)*32+0),300,0,0,false}, // ThrashMove
 {uint16_t(unsigned(EnemyId::BowlbugNectar)*32+2),300,0,0,false}, // ThrashMove
 {uint16_t(unsigned(EnemyId::BowlbugRock)*32+0),300,0,0,false}, // HeadbuttMove
 {uint16_t(unsigned(EnemyId::BowlbugSilk)*32+0),300,0,0,true}, // ThrashMove
 {uint16_t(unsigned(EnemyId::BruteRubyRaider)*32+0),600,0,0,false}, // BeatMove
 {uint16_t(unsigned(EnemyId::BygoneEffigy)*32+3),100,0,0,false}, // SlashMove
 {uint16_t(unsigned(EnemyId::Byrdonis)*32+0),400,0,0,false}, // PeckMove
 {uint16_t(unsigned(EnemyId::Byrdonis)*32+1),400,0,0,false}, // SwoopMove
 {uint16_t(unsigned(EnemyId::CalcifiedCultist)*32+1),200,0,0,false}, // DarkStrikeMove
 {uint16_t(unsigned(EnemyId::CeremonialBeast)*32+1),0,0,0,false}, // PlowMove
 {uint16_t(unsigned(EnemyId::CeremonialBeast)*32+4),1000,0,0,false}, // StompMove
 {uint16_t(unsigned(EnemyId::CeremonialBeast)*32+5),1000,0,0,false}, // CrushMove
 {uint16_t(unsigned(EnemyId::Chomper)*32+0),300,0,0,false}, // ClampMove
 {uint16_t(unsigned(EnemyId::CorpseSlug)*32+0),300,0,0,true}, // WhipSlapMove
 {uint16_t(unsigned(EnemyId::CorpseSlug)*32+1),300,0,0,false}, // GlompMove
 {uint16_t(unsigned(EnemyId::CrossbowRubyRaider)*32+0),250,0,0,false}, // FireMove
 {uint16_t(unsigned(EnemyId::CubexConstruct)*32+1),0,0,0,false}, // RepeaterBlastMove
 {uint16_t(unsigned(EnemyId::CubexConstruct)*32+2),0,0,0,false}, // RepeaterBlastMove
 {uint16_t(unsigned(EnemyId::CubexConstruct)*32+3),0,0,0,false}, // ExpelBlastMove
 {uint16_t(unsigned(EnemyId::DampCultist)*32+1),200,0,0,false}, // DarkStrikeMove
 {uint16_t(unsigned(EnemyId::DevotedSculptor)*32+1),300,0,0,false}, // SavageMove
 {uint16_t(unsigned(EnemyId::Entomancer)*32+1),300,0,0,true}, // BeesMove
 {uint16_t(unsigned(EnemyId::Entomancer)*32+2),250,0,0,false}, // SpearMove
 {uint16_t(unsigned(EnemyId::Exoskeleton)*32+0),150,0,0,true}, // SkitterMove
 {uint16_t(unsigned(EnemyId::Exoskeleton)*32+1),300,0,0,false}, // MandiblesMove
 {uint16_t(unsigned(EnemyId::Fabricator)*32+1),600,0,0,false}, // FabricatingStrikeMove
 {uint16_t(unsigned(EnemyId::Fabricator)*32+2),600,0,0,false}, // DisintegrateMove
 {uint16_t(unsigned(EnemyId::FakeMerchantMonster)*32+0),150,0,0,false}, // SwipeMove
 {uint16_t(unsigned(EnemyId::FakeMerchantMonster)*32+1),150,0,0,true}, // SpewCoinsMove
 {uint16_t(unsigned(EnemyId::FakeMerchantMonster)*32+2),150,0,0,false}, // ThrowRelicMove
 {uint16_t(unsigned(EnemyId::FlailKnight)*32+1),500,0,0,true}, // FlailMove
 {uint16_t(unsigned(EnemyId::FlailKnight)*32+2),500,0,0,false}, // RamMove
 {uint16_t(unsigned(EnemyId::Flyconid)*32+1),500,600,0,false}, // FrailSporesMove
 {uint16_t(unsigned(EnemyId::Flyconid)*32+2),300,0,0,false}, // SmashMove
 {uint16_t(unsigned(EnemyId::Fogmog)*32+1),500,0,0,false}, // SwipeMove
 {uint16_t(unsigned(EnemyId::Fogmog)*32+2),500,0,0,false}, // SwipeMove
 {uint16_t(unsigned(EnemyId::Fogmog)*32+3),500,0,0,false}, // HeadbuttMove
 {uint16_t(unsigned(EnemyId::FossilStalker)*32+0),350,0,0,false}, // TackleMove
 {uint16_t(unsigned(EnemyId::FossilStalker)*32+1),200,0,0,false}, // LatchMove
 {uint16_t(unsigned(EnemyId::FossilStalker)*32+2),200,0,0,true}, // LashAttack
 {uint16_t(unsigned(EnemyId::FrogKnight)*32+1),500,0,0,false}, // StrikeDownEvilMove
 {uint16_t(unsigned(EnemyId::FrogKnight)*32+2),300,0,0,false}, // TongueLashMove
 {uint16_t(unsigned(EnemyId::FrogKnight)*32+3),600,0,0,false}, // BeetleChargeMove
 {uint16_t(unsigned(EnemyId::FuzzyWurmCrawler)*32+0),1000,0,0,false}, // AcidGoop
 {uint16_t(unsigned(EnemyId::FuzzyWurmCrawler)*32+1),1000,0,0,false}, // AcidGoop
 {uint16_t(unsigned(EnemyId::GasBomb)*32+0),100,0,0,false}, // ExplodeMove
 {uint16_t(unsigned(EnemyId::GlobeHead)*32+0),500,0,0,true}, // ThunderStrike
 {uint16_t(unsigned(EnemyId::GlobeHead)*32+1),500,0,0,false}, // ShockingSlap
 {uint16_t(unsigned(EnemyId::GlobeHead)*32+2),500,0,0,false}, // GalvanicBurstMove
 {uint16_t(unsigned(EnemyId::GremlinMerc)*32+0),150,0,0,true}, // GimmeMove
 {uint16_t(unsigned(EnemyId::GremlinMerc)*32+1),150,0,0,true}, // DoubleSmashMove
 {uint16_t(unsigned(EnemyId::GremlinMerc)*32+2),150,0,0,false}, // HeheMove
 {uint16_t(unsigned(EnemyId::HauntedShip)*32+0),150,0,0,false}, // SwipeMove
 {uint16_t(unsigned(EnemyId::HauntedShip)*32+1),150,0,0,true}, // StompMove
 {uint16_t(unsigned(EnemyId::HunterKiller)*32+1),300,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::HunterKiller)*32+2),300,0,0,true}, // PunctureMove
 {uint16_t(unsigned(EnemyId::InfestedPrism)*32+0),100,0,0,false}, // JabMove
 {uint16_t(unsigned(EnemyId::InfestedPrism)*32+1),250,0,0,false}, // RadiateMove
 {uint16_t(unsigned(EnemyId::InfestedPrism)*32+2),200,0,0,true}, // WhirlwindMove
 {uint16_t(unsigned(EnemyId::InfestedPrism)*32+3),250,0,0,false}, // PulsateMove
 {uint16_t(unsigned(EnemyId::Inklet)*32+0),750,0,0,false}, // JabMove
 {uint16_t(unsigned(EnemyId::Inklet)*32+1),300,0,0,true}, // WhirlwindMove
 {uint16_t(unsigned(EnemyId::Inklet)*32+2),750,0,0,false}, // PiercingGazeMove
 {uint16_t(unsigned(EnemyId::KinFollower)*32+0),200,0,0,false}, // QuickSlashMove
 {uint16_t(unsigned(EnemyId::KinFollower)*32+1),200,0,0,true}, // BoomerangMove
 {uint16_t(unsigned(EnemyId::KinPriest)*32+0),0,1000,1000,false}, // OrbOfFrailtyMove
 {uint16_t(unsigned(EnemyId::KinPriest)*32+1),0,1000,1000,false}, // OrbOfWeaknessMove
 {uint16_t(unsigned(EnemyId::KinPriest)*32+2),400,0,0,true}, // BeamMove
 {uint16_t(unsigned(EnemyId::KnowledgeDemon)*32+1),500,0,0,false}, // SlapMove
 {uint16_t(unsigned(EnemyId::KnowledgeDemon)*32+2),850,0,0,true}, // KnowledgeOverwhelmingMove
 {uint16_t(unsigned(EnemyId::LagavulinMatriarch)*32+1),300,0,0,false}, // SlashMove
 {uint16_t(unsigned(EnemyId::LagavulinMatriarch)*32+2),200,0,0,false}, // Slash2Move
 {uint16_t(unsigned(EnemyId::LagavulinMatriarch)*32+3),150,0,0,true}, // DisembowelMove
 {uint16_t(unsigned(EnemyId::LeafSlimeM)*32+0),150,0,0,false}, // ClumpShotMove
 {uint16_t(unsigned(EnemyId::LeafSlimeS)*32+0),150,0,0,false}, // TackleMove
 {uint16_t(unsigned(EnemyId::LivingFog)*32+0),1250,0,0,false}, // AdvancedGasMove
 {uint16_t(unsigned(EnemyId::LivingFog)*32+1),100,0,0,false}, // BloatMove
 {uint16_t(unsigned(EnemyId::LivingFog)*32+2),100,0,0,false}, // SuperGasBlastMove
 {uint16_t(unsigned(EnemyId::LivingShield)*32+0),300,0,0,false}, // ShieldSlamMove
 {uint16_t(unsigned(EnemyId::LivingShield)*32+1),300,0,0,false}, // SmashMove
 {uint16_t(unsigned(EnemyId::LouseProgenitor)*32+0),200,0,0,false}, // WebMove
 {uint16_t(unsigned(EnemyId::LouseProgenitor)*32+1),200,0,0,false}, // PounceMove
 {uint16_t(unsigned(EnemyId::MagiKnight)*32+0),600,0,0,false}, // PowerShieldMove
 {uint16_t(unsigned(EnemyId::MagiKnight)*32+3),1200,0,0,false}, // MagicBombMove
 {uint16_t(unsigned(EnemyId::MagiKnight)*32+4),1200,0,0,false}, // SpearMove
 {uint16_t(unsigned(EnemyId::Mawler)*32+0),350,0,0,false}, // RipAndTearMove
 {uint16_t(unsigned(EnemyId::Mawler)*32+2),350,0,0,true}, // ClawMove
 {uint16_t(unsigned(EnemyId::MechaKnight)*32+0),250,1000,500,false}, // ChargeMove
 {uint16_t(unsigned(EnemyId::MechaKnight)*32+1),1500,0,0,false}, // FlamethrowerMove
 {uint16_t(unsigned(EnemyId::MechaKnight)*32+3),400,0,0,false}, // HeavyCleaveMove
 {uint16_t(unsigned(EnemyId::Myte)*32+1),500,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::Myte)*32+2),400,0,0,false}, // SuckMove
 {uint16_t(unsigned(EnemyId::Nibbit)*32+0),150,0,0,false}, // ButtMove
 {uint16_t(unsigned(EnemyId::Nibbit)*32+1),150,0,0,false}, // SliceMove
 {uint16_t(unsigned(EnemyId::Ovicopter)*32+1),300,0,0,false}, // SmashMove
 {uint16_t(unsigned(EnemyId::Ovicopter)*32+2),300,0,0,false}, // TenderizerMove
 {uint16_t(unsigned(EnemyId::OwlMagistrate)*32+0),500,0,0,false}, // MagistrateScrutinyMove
 {uint16_t(unsigned(EnemyId::OwlMagistrate)*32+1),500,0,0,true}, // PeckAssaultMove
 {uint16_t(unsigned(EnemyId::OwlMagistrate)*32+3),500,0,0,false}, // VerdictMove
 {uint16_t(unsigned(EnemyId::Parafright)*32+0),150,0,0,false}, // SlamMove
 {uint16_t(unsigned(EnemyId::PhantasmalGardener)*32+0),300,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::PhantasmalGardener)*32+1),300,0,0,false}, // LashMove
 {uint16_t(unsigned(EnemyId::PhantasmalGardener)*32+2),300,0,0,true}, // FlailMove
 {uint16_t(unsigned(EnemyId::PhrogParasite)*32+1),550,0,0,true}, // LashMove
 {uint16_t(unsigned(EnemyId::PunchConstruct)*32+1),250,0,0,false}, // StrongPunchMove
 {uint16_t(unsigned(EnemyId::PunchConstruct)*32+2),200,0,0,true}, // FastPunchMove
 {uint16_t(unsigned(EnemyId::Queen)*32+3),600,0,0,true}, // OffWithYourHeadMove
 {uint16_t(unsigned(EnemyId::Queen)*32+4),600,0,0,true}, // ExecutionMove
 {uint16_t(unsigned(EnemyId::ScrollOfBiting)*32+0),200,0,0,false}, // ChompMove
 {uint16_t(unsigned(EnemyId::ScrollOfBiting)*32+1),200,0,0,true}, // ChewState
 {uint16_t(unsigned(EnemyId::Seapunk)*32+0),150,0,0,false}, // SeaKickMove
 {uint16_t(unsigned(EnemyId::Seapunk)*32+1),150,0,0,true}, // SpinningKickMove
 {uint16_t(unsigned(EnemyId::SewerClam)*32+1),450,0,0,false}, // JetMove
 {uint16_t(unsigned(EnemyId::ShrinkerBeetle)*32+1),250,0,0,false}, // ChompMove
 {uint16_t(unsigned(EnemyId::ShrinkerBeetle)*32+2),250,0,0,false}, // StompMove
 {uint16_t(unsigned(EnemyId::SkulkingColony)*32+0),250,0,0,false}, // ZoomMove
 {uint16_t(unsigned(EnemyId::SkulkingColony)*32+1),250,0,0,false}, // ZoomMove
 {uint16_t(unsigned(EnemyId::SkulkingColony)*32+2),500,0,0,false}, // InertiaMove
 {uint16_t(unsigned(EnemyId::SkulkingColony)*32+3),450,0,0,true}, // PiercingStabsMove
 {uint16_t(unsigned(EnemyId::SlimedBerserker)*32+2),200,0,0,false}, // SmotherMove
 {uint16_t(unsigned(EnemyId::SlimedBerserker)*32+3),200,0,0,true}, // FuriousPummelingMove
 {uint16_t(unsigned(EnemyId::SlitheringStrangler)*32+1),200,0,0,false}, // ThwackMove
 {uint16_t(unsigned(EnemyId::SlitheringStrangler)*32+2),200,0,0,false}, // LashMove
 {uint16_t(unsigned(EnemyId::SludgeSpinner)*32+0),500,0,0,false}, // OilSprayMove
 {uint16_t(unsigned(EnemyId::SludgeSpinner)*32+1),150,0,0,false}, // SlamMove
 {uint16_t(unsigned(EnemyId::SludgeSpinner)*32+2),500,0,0,false}, // RageMove
 {uint16_t(unsigned(EnemyId::SlumberingBeetle)*32+1),500,0,0,false}, // RolloutMove
 {uint16_t(unsigned(EnemyId::SnappingJaxfruit)*32+0),250,0,0,false}, // EnergyOrb
 {uint16_t(unsigned(EnemyId::SneakyGremlin)*32+1),100,0,0,false}, // TackleMove
 {uint16_t(unsigned(EnemyId::SoulFysh)*32+1),450,0,0,false}, // DeGasMove
 {uint16_t(unsigned(EnemyId::SoulFysh)*32+2),600,0,0,false}, // GazeMove
 {uint16_t(unsigned(EnemyId::SoulFysh)*32+4),650,0,0,false}, // ScreamMove
 {uint16_t(unsigned(EnemyId::SoulNexus)*32+0),600,0,0,false}, // SoulBurnMove
 {uint16_t(unsigned(EnemyId::SoulNexus)*32+1),200,0,0,true}, // MaelstromMove
 {uint16_t(unsigned(EnemyId::SoulNexus)*32+2),1000,0,0,false}, // DrainLifeMove
 {uint16_t(unsigned(EnemyId::SpectralKnight)*32+1),250,0,0,false}, // SoulSlashMove
 {uint16_t(unsigned(EnemyId::SpectralKnight)*32+2),250,0,0,true}, // SoulFlameMove
 {uint16_t(unsigned(EnemyId::SpinyToad)*32+1),700,0,0,false}, // ExplosionMove
 {uint16_t(unsigned(EnemyId::SpinyToad)*32+2),300,0,0,false}, // LashMove
 {uint16_t(unsigned(EnemyId::Stabbot)*32+0),600,0,0,false}, // StabMove
 {uint16_t(unsigned(EnemyId::TerrorEel)*32+0),200,0,0,false}, // CrashMove
 {uint16_t(unsigned(EnemyId::TerrorEel)*32+1),250,0,0,true}, // ThrashMove
 {uint16_t(unsigned(EnemyId::TestSubject)*32+1),250,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::TestSubject)*32+2),250,0,0,false}, // SkullBashMove
 {uint16_t(unsigned(EnemyId::TestSubject)*32+3),200,0,0,true}, // MultiClawMove
 {uint16_t(unsigned(EnemyId::TestSubject)*32+4),200,0,0,true}, // Phase3LacerateMove
 {uint16_t(unsigned(EnemyId::TestSubject)*32+5),250,0,0,false}, // BigPounceMove
 {uint16_t(unsigned(EnemyId::TheForgotten)*32+1),150,0,0,true}, // DreadMove
 {uint16_t(unsigned(EnemyId::TheInsatiable)*32+1),300,0,0,true}, // ThrashMove
 {uint16_t(unsigned(EnemyId::TheInsatiable)*32+2),300,0,0,true}, // ThrashMove
 {uint16_t(unsigned(EnemyId::TheInsatiable)*32+3),250,0,0,true}, // BiteMove
 {uint16_t(unsigned(EnemyId::TheLost)*32+1),150,0,0,true}, // EyeLasersMove
 {uint16_t(unsigned(EnemyId::TheObscura)*32+1),300,0,0,false}, // PiercingGazeMove
 {uint16_t(unsigned(EnemyId::TheObscura)*32+3),300,0,0,false}, // HardeningStrikeMove
 {uint16_t(unsigned(EnemyId::ThievingHopper)*32+0),0,0,0,false}, // ThieveryMove
 {uint16_t(unsigned(EnemyId::ThievingHopper)*32+1),300,0,0,false}, // NabMove
 {uint16_t(unsigned(EnemyId::ThievingHopper)*32+2),300,0,0,false}, // HatTrickMove
 {uint16_t(unsigned(EnemyId::Toadpole)*32+0),300,0,0,true}, // SpikeSpitMove
 {uint16_t(unsigned(EnemyId::Toadpole)*32+1),150,0,0,false}, // WhirlMove
 {uint16_t(unsigned(EnemyId::TorchHeadAmalgam)*32+0),600,0,0,false}, // StrongTackleMove
 {uint16_t(unsigned(EnemyId::TorchHeadAmalgam)*32+1),600,0,0,false}, // TackleMove
 {uint16_t(unsigned(EnemyId::TorchHeadAmalgam)*32+2),800,0,0,true}, // SoulBeamMove
 {uint16_t(unsigned(EnemyId::TorchHeadAmalgam)*32+3),600,0,0,false}, // WeakTackleMove
 {uint16_t(unsigned(EnemyId::TorchHeadAmalgam)*32+4),600,0,0,false}, // WeakTackleMove
 {uint16_t(unsigned(EnemyId::ToughEgg)*32+1),200,0,0,false}, // NibbleMove
 {uint16_t(unsigned(EnemyId::TrackerRubyRaider)*32+1),500,0,0,true}, // HoundsMove
 {uint16_t(unsigned(EnemyId::Tunneler)*32+0),250,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::Tunneler)*32+2),0,0,0,false}, // BelowMove
 {uint16_t(unsigned(EnemyId::TurretOperator)*32+0),400,0,0,true}, // UnloadMove
 {uint16_t(unsigned(EnemyId::TurretOperator)*32+1),400,0,0,true}, // UnloadMove
 {uint16_t(unsigned(EnemyId::TwigSlimeM)*32+0),150,0,0,false}, // ClumpShotMove
 {uint16_t(unsigned(EnemyId::TwigSlimeS)*32+0),150,0,0,false}, // TackleMove
 {uint16_t(unsigned(EnemyId::TwoTailedRat)*32+0),250,0,0,false}, // ScratchMove
 {uint16_t(unsigned(EnemyId::TwoTailedRat)*32+1),250,0,0,false}, // DiseaseBiteMove
 {uint16_t(unsigned(EnemyId::Vantom)*32+0),350,0,0,false}, // InkBlotMove
 {uint16_t(unsigned(EnemyId::Vantom)*32+1),400,0,0,true}, // InkyLanceMove
 {uint16_t(unsigned(EnemyId::Vantom)*32+2),0,0,0,false}, // DismemberMove
 {uint16_t(unsigned(EnemyId::VineShambler)*32+0),500,0,0,false}, // GraspingVinesMove
 {uint16_t(unsigned(EnemyId::VineShambler)*32+1),400,0,0,true}, // SwipeMove
 {uint16_t(unsigned(EnemyId::VineShambler)*32+2),400,0,0,false}, // ChompMove
 {uint16_t(unsigned(EnemyId::WaterfallGiant)*32+1),300,0,0,false}, // StompMove
 {uint16_t(unsigned(EnemyId::WaterfallGiant)*32+2),300,0,0,false}, // RamMove
 {uint16_t(unsigned(EnemyId::WaterfallGiant)*32+4),300,0,0,false}, // PressureGunMove
 {uint16_t(unsigned(EnemyId::WaterfallGiant)*32+5),150,0,0,false}, // PressureUpMove
 {uint16_t(unsigned(EnemyId::WaterfallGiant)*32+7),100,0,0,false}, // ExplodeMove
 {uint16_t(unsigned(EnemyId::Wriggler)*32+0),150,0,0,false}, // BiteMove
 {uint16_t(unsigned(EnemyId::Zapbot)*32+0),600,0,0,false}, // ZapMove
 };
 const unsigned key=unsigned(kind)*32+unsigned(move);
 for(const auto&r:rows)if(r.key==key)return {fast?(r.delay/2u<250u?r.delay/2u:250u):unsigned(r.delay),fast?r.fast_wait:r.normal_wait,r.once,true};
 return {};
}
inline CombatAttackTiming combat_card_attack_timing(const State&s,CardKind kind,bool fast){
 unsigned delay=150;const bool once=kind==DefectRefract;
 switch(kind){
 case Bludgeon:case Break:case Cinder:case Mangle:case PactsEnd:case PerfectedStrike:case Stomp:case Unrelenting:case Uppercut:
  if(s.character==Character::Ironclad)delay=200;break;
 case FlashOfSteel:case Neutralize:case Slice:case Suppress:delay=fast?150:350;break;
 case HowlFromBeyond:case SovereignBlade:delay=s.character==Character::Necrobinder?400:250;break;
 case DefectHyperbeam:case DefectSweepingBeam:delay=500;break;
 case MinionDiveBomb:delay=1000;break;
 case Shiv:if(s.character==Character::Silent)delay=200;break;
 case RegentComet:case RegentGuidingStar:delay=0;break;
 default:break;
 }
 return {fast?(delay/2<250?delay/2:250):delay,0,once,true};
}
}
