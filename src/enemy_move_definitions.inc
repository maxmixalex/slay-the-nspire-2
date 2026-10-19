// Numeric intent schema; state transitions and commands live in enemy_moves.inc.
struct EnemyMoveSpec {const char*key;uint16_t flags;int16_t damage,damage_9,hits;};
static const EnemyMoveSpec em_Aeonglass[]={
 {"EBB_MOVE",3,22,26,1},
 {"EYE_LASERS_MOVE",1,11,12,2},
 {"INCREASING_INTENSITY_MOVE",20,0,0,0},
};
static const EnemyMoveSpec em_Architect[]={
 {"NOTHING",2048,0,0,0},
};
static const EnemyMoveSpec em_AssassinRubyRaider[]={
 {"KILLSHOT_MOVE",1,10,11,1},
};
static const EnemyMoveSpec em_AxeRubyRaider[]={
 {"SWING_1",3,5,6,1},
 {"SWING_2",3,5,6,1},
 {"BIG_SWING",1,12,13,1},
};
static const EnemyMoveSpec em_Axebot[]={
 {"BOOT_UP_MOVE",6,0,0,0},
 {"ONE_TWO_MOVE",1,10,11,2},
 {"HAMMER_UPPERCUT_MOVE",9,14,18,1},
};
static const EnemyMoveSpec em_BattleFriendV1[]={
 {"NOTHING_MOVE",0,0,0,0},
};
static const EnemyMoveSpec em_BattleFriendV2[]={
 {"NOTHING_MOVE",0,0,0,0},
};
static const EnemyMoveSpec em_BattleFriendV3[]={
 {"NOTHING_MOVE",0,0,0,0},
};
static const EnemyMoveSpec em_BowlbugEgg[]={
 {"BITE_MOVE",3,7,8,1},
};
static const EnemyMoveSpec em_BowlbugNectar[]={
 {"THRASH_MOVE",1,3,3,1},
 {"BUFF_MOVE",4,0,0,0},
 {"THRASH2_MOVE",1,3,3,1},
};
static const EnemyMoveSpec em_BowlbugRock[]={
 {"HEADBUTT_MOVE",1,15,16,1},
 {"DIZZY_MOVE",256,0,0,0},
};
static const EnemyMoveSpec em_BowlbugSilk[]={
 {"THRASH_MOVE",1,4,5,2},
 {"TOXIC_SPIT_MOVE",8,0,0,0},
};
static const EnemyMoveSpec em_BruteRubyRaider[]={
 {"BEAT_MOVE",1,7,8,1},
 {"ROAR_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_BygoneEffigy[]={
 {"SLEEP_MOVE",512,0,0,0},
 {"WAKE_MOVE",4,0,0,0},
 {"SLEEP_MOVE_2",512,0,0,0},
 {"SLASHES_MOVE",1,13,15,1},
};
static const EnemyMoveSpec em_Byrdonis[]={
 {"PECK_MOVE",1,3,4,3},
 {"SWOOP_MOVE",1,17,19,1},
};
static const EnemyMoveSpec em_CalcifiedCultist[]={
 {"INCANTATION_MOVE",4,0,0,0},
 {"DARK_STRIKE_MOVE",1,9,11,1},
};
static const EnemyMoveSpec em_CeremonialBeast[]={
 {"STAMP_MOVE",4,0,0,0},
 {"PLOW_MOVE",5,18,20,1},
 {"STUN_MOVE",256,0,0,0},
 {"BEAST_CRY_MOVE",8,0,0,0},
 {"STOMP_MOVE",1,15,17,1},
 {"CRUSH_MOVE",5,17,19,1},
};
static const EnemyMoveSpec em_Chomper[]={
 {"CLAMP_MOVE",1,8,9,2},
 {"SCREECH_MOVE",16,0,0,0},
};
static const EnemyMoveSpec em_CorpseSlug[]={
 {"WHIP_SLAP_MOVE",1,3,3,2},
 {"GLOMP_MOVE",1,8,9,1},
 {"GOOP_MOVE",8,0,0,0},
};
static const EnemyMoveSpec em_CrossbowRubyRaider[]={
 {"FIRE_MOVE",1,14,16,1},
 {"RELOAD_MOVE",2,0,0,0},
};
static const EnemyMoveSpec em_Crusher[]={
 {"THRASH_MOVE",1,12,14,1},
 {"ENLARGING_STRIKE_MOVE",1,4,4,1},
 {"BUG_STING_MOVE",9,6,7,2},
 {"ADAPT_MOVE",4,0,0,0},
 {"GUARDED_STRIKE_MOVE",3,12,14,1},
};
static const EnemyMoveSpec em_CubexConstruct[]={
 {"CHARGE_UP_MOVE",4,0,0,0},
 {"REPEATER_BLAST_MOVE",5,7,8,1},
 {"REPEATER_BLAST_MOVE_2",5,7,8,1},
 {"EXPEL_MOVE",1,5,6,2},
};
static const EnemyMoveSpec em_DampCultist[]={
 {"INCANTATION_MOVE",4,0,0,0},
 {"DARK_STRIKE_MOVE",1,1,3,1},
};
static const EnemyMoveSpec em_DecimillipedeSegmentBack[]={
 {"WRITHE_MOVE",1,5,6,2},
 {"BULK_MOVE",5,6,7,1},
 {"CONSTRICT_MOVE",9,8,9,1},
 {"DEAD_MOVE",0,0,0,0},
 {"REATTACH_MOVE",32,0,0,0},
};
static const EnemyMoveSpec em_DecimillipedeSegmentFront[]={
 {"WRITHE_MOVE",1,5,6,2},
 {"BULK_MOVE",5,6,7,1},
 {"CONSTRICT_MOVE",9,8,9,1},
 {"DEAD_MOVE",0,0,0,0},
 {"REATTACH_MOVE",32,0,0,0},
};
static const EnemyMoveSpec em_DecimillipedeSegmentMiddle[]={
 {"WRITHE_MOVE",1,5,6,2},
 {"BULK_MOVE",5,6,7,1},
 {"CONSTRICT_MOVE",9,8,9,1},
 {"DEAD_MOVE",0,0,0,0},
 {"REATTACH_MOVE",32,0,0,0},
};
static const EnemyMoveSpec em_DevotedSculptor[]={
 {"FORBIDDEN_INCANTATION_MOVE",4,0,0,0},
 {"SAVAGE_MOVE",1,12,15,1},
};
static const EnemyMoveSpec em_Entomancer[]={
 {"PHEROMONE_SPIT_MOVE",4,0,0,0},
 {"BEES_MOVE",1,3,3,7},
 {"SPEAR_MOVE",1,18,20,1},
};
static const EnemyMoveSpec em_Exoskeleton[]={
 {"SKITTER_MOVE",1,1,1,3},
 {"MANDIBLES_MOVE",1,8,9,1},
 {"ENRAGE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_EyeWithTeeth[]={
 {"DISTRACT_MOVE",16,0,0,0},
};
static const EnemyMoveSpec em_Fabricator[]={
 {"FABRICATE_MOVE",64,0,0,0},
 {"FABRICATING_STRIKE_MOVE",65,18,21,1},
 {"DISINTEGRATE_MOVE",1,11,13,1},
};
static const EnemyMoveSpec em_FakeMerchantMonster[]={
 {"SWIPE_MOVE",1,13,15,1},
 {"SPEW_COINS_MOVE",1,2,2,8},
 {"THROW_RELIC_MOVE",9,9,10,1},
 {"ENRAGE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_FatGremlin[]={
 {"SPAWNED_MOVE",256,0,0,0},
 {"FLEE_MOVE",128,0,0,0},
};
static const EnemyMoveSpec em_FlailKnight[]={
 {"WAR_CHANT",4,0,0,0},
 {"FLAIL_MOVE",1,9,10,2},
 {"RAM_MOVE",1,15,17,1},
};
static const EnemyMoveSpec em_Flyconid[]={
 {"VULNERABLE_SPORES_MOVE",8,0,0,0},
 {"FRAIL_SPORES_MOVE",9,8,9,1},
 {"SMASH_MOVE",1,11,12,1},
};
static const EnemyMoveSpec em_Fogmog[]={
 {"ILLUSION_MOVE",64,0,0,0},
 {"SWIPE_MOVE",5,8,9,1},
 {"SWIPE_RANDOM_MOVE",5,8,9,1},
 {"HEADBUTT_MOVE",1,14,16,1},
};
static const EnemyMoveSpec em_FossilStalker[]={
 {"TACKLE_MOVE",9,9,11,1},
 {"LATCH_MOVE",1,12,14,1},
 {"LASH_MOVE",1,3,4,2},
};
static const EnemyMoveSpec em_FrogKnight[]={
 {"FOR_THE_QUEEN",4,0,0,0},
 {"STRIKE_DOWN_EVIL",1,21,23,1},
 {"TONGUE_LASH",9,13,14,1},
 {"BEETLE_CHARGE",1,35,40,1},
};
static const EnemyMoveSpec em_FuzzyWurmCrawler[]={
 {"FIRST_ACID_GOOP",1,4,6,1},
 {"ACID_GOOP",1,4,6,1},
 {"INHALE",4,0,0,0},
};
static const EnemyMoveSpec em_GasBomb[]={
 {"EXPLODE_MOVE",1025,8,9,1},
};
static const EnemyMoveSpec em_GlobeHead[]={
 {"THUNDER_STRIKE",1,6,7,3},
 {"SHOCKING_SLAP",9,13,14,1},
 {"GALVANIC_BURST",5,16,17,1},
};
static const EnemyMoveSpec em_GremlinMerc[]={
 {"GIMME_MOVE",1,7,8,2},
 {"DOUBLE_SMASH_MOVE",9,6,7,2},
 {"HEHE_MOVE",5,8,9,1},
};
static const EnemyMoveSpec em_Guardbot[]={
 {"GUARD_MOVE",2,0,0,0},
};
static const EnemyMoveSpec em_HauntedShip[]={
 {"SWIPE_MOVE",1,13,14,1},
 {"STOMP_MOVE",1,4,5,3},
 {"HAUNT_MOVE",24,0,0,0},
};
static const EnemyMoveSpec em_HunterKiller[]={
 {"TENDERIZING_GOOP_MOVE",8,0,0,0},
 {"BITE_MOVE",1,17,19,1},
 {"PUNCTURE_MOVE",1,7,8,3},
};
static const EnemyMoveSpec em_InfestedPrism[]={
 {"JAB_MOVE",1,15,17,1},
 {"RADIATE_MOVE",3,11,13,1},
 {"WHIRLWIND_MOVE",1,5,6,3},
 {"PULSATE_MOVE",7,8,10,1},
};
static const EnemyMoveSpec em_Inklet[]={
 {"JAB_MOVE",1,3,4,1},
 {"WHIRLWIND_MOVE",1,2,3,3},
 {"PIERCING_GAZE_MOVE",1,10,11,1},
};
static const EnemyMoveSpec em_KinFollower[]={
 {"QUICK_SLASH_MOVE",1,5,5,1},
 {"BOOMERANG_MOVE",1,2,2,2},
 {"POWER_DANCE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_KinPriest[]={
 {"ORB_OF_FRAILTY_MOVE",9,8,9,1},
 {"ORB_OF_WEAKNESS_MOVE",9,8,9,1},
 {"BEAM_MOVE",1,3,3,3},
 {"RITUAL_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_KnowledgeDemon[]={
 {"CURSE_OF_KNOWLEDGE_MOVE",8,0,0,0},
 {"SLAP_MOVE",1,17,18,1},
 {"KNOWLEDGE_OVERWHELMING_MOVE",1,8,9,3},
 {"PONDER_MOVE",37,11,13,1},
};
static const EnemyMoveSpec em_LagavulinMatriarch[]={
 {"SLEEP_MOVE",512,0,0,0},
 {"SLASH_MOVE",1,19,21,1},
 {"SLASH2_MOVE",3,12,14,1},
 {"DISEMBOWEL_MOVE",1,9,10,2},
 {"SOUL_SIPHON_MOVE",12,0,0,0},
};
static const EnemyMoveSpec em_LeafSlimeM[]={
 {"CLUMP_SHOT",1,8,9,1},
 {"STICKY_SHOT",16,0,0,0},
};
static const EnemyMoveSpec em_LeafSlimeS[]={
 {"TACKLE_MOVE",1,3,4,1},
 {"GOOP_MOVE",16,0,0,0},
};
static const EnemyMoveSpec em_LivingFog[]={
 {"ADVANCED_GAS_MOVE",17,8,9,1},
 {"BLOAT_MOVE",65,5,6,1},
 {"SUPER_GAS_BLAST_MOVE",1,8,9,1},
};
static const EnemyMoveSpec em_LivingShield[]={
 {"SHIELD_SLAM_MOVE",1,6,6,1},
 {"SMASH_MOVE",5,16,18,1},
};
static const EnemyMoveSpec em_LouseProgenitor[]={
 {"WEB_CANNON_MOVE",9,9,10,1},
 {"POUNCE_MOVE",1,14,16,1},
 {"CURL_AND_GROW_MOVE",6,0,0,0},
};
static const EnemyMoveSpec em_MagiKnight[]={
 {"POWER_SHIELD_MOVE",3,6,7,1},
 {"DAMPEN_MOVE",8,0,0,0},
 {"PREP_MOVE",2,0,0,0},
 {"MAGIC_BOMB",1,35,40,1},
 {"RAM_MOVE",1,10,11,1},
};
static const EnemyMoveSpec em_Mawler[]={
 {"RIP_AND_TEAR_MOVE",1,14,16,1},
 {"ROAR_MOVE",8,0,0,0},
 {"CLAW_MOVE",1,4,5,2},
};
static const EnemyMoveSpec em_MechaKnight[]={
 {"CHARGE_MOVE",1,25,30,1},
 {"FLAMETHROWER_MOVE",17,8,12,1},
 {"WINDUP_MOVE",6,0,0,0},
 {"HEAVY_CLEAVE_MOVE",1,35,40,1},
};
static const EnemyMoveSpec em_MysteriousKnight[]={
 {"WAR_CHANT",4,0,0,0},
 {"FLAIL_MOVE",1,9,10,2},
 {"RAM_MOVE",1,15,17,1},
};
static const EnemyMoveSpec em_Myte[]={
 {"TOXIC_MOVE",16,0,0,0},
 {"BITE_MOVE",1,13,15,1},
 {"SUCK_MOVE",5,4,6,1},
};
static const EnemyMoveSpec em_Nibbit[]={
 {"BUTT_MOVE",1,12,13,1},
 {"SLICE_MOVE",3,6,7,1},
 {"HISS_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_Noisebot[]={
 {"NOISE_MOVE",16,0,0,0},
};
static const EnemyMoveSpec em_Ovicopter[]={
 {"LAY_EGGS_MOVE",64,0,0,0},
 {"SMASH_MOVE",1,16,17,1},
 {"TENDERIZER_MOVE",9,7,8,1},
 {"NUTRITIONAL_PASTE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_OwlMagistrate[]={
 {"MAGISTRATE_SCRUTINY",1,16,17,1},
 {"PECK_ASSAULT",1,4,4,6},
 {"JUDICIAL_FLIGHT",4,0,0,0},
 {"VERDICT",9,33,36,1},
};
static const EnemyMoveSpec em_Parafright[]={
 {"SLAM_MOVE",1,16,17,1},
};
static const EnemyMoveSpec em_PhantasmalGardener[]={
 {"BITE_MOVE",1,5,5,1},
 {"LASH_MOVE",1,7,7,1},
 {"FLAIL_MOVE",1,1,1,3},
 {"ENLARGE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_PhrogParasite[]={
 {"INFECT_MOVE",16,0,0,0},
 {"LASH_MOVE",1,4,5,4},
};
static const EnemyMoveSpec em_PunchConstruct[]={
 {"READY_MOVE",2,0,0,0},
 {"STRONG_PUNCH_MOVE",1,14,16,1},
 {"FAST_PUNCH_MOVE",9,5,6,2},
};
static const EnemyMoveSpec em_Queen[]={
 {"PUPPET_STRINGS_MOVE",16,0,0,0},
 {"YOU_ARE_MINE_MOVE",8,0,0,0},
 {"BURN_BRIGHT_FOR_ME_MOVE",6,0,0,0},
 {"OFF_WITH_YOUR_HEAD_MOVE",1,3,4,5},
 {"EXECUTION_MOVE",1,15,18,1},
 {"ENRAGE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_Rocket[]={
 {"TARGETING_RETICLE_MOVE",1,3,4,1},
 {"PRECISION_BEAM_MOVE",1,18,20,1},
 {"CHARGE_UP_MOVE",4,0,0,0},
 {"LASER_MOVE",1,31,35,1},
 {"RECHARGE_MOVE",512,0,0,0},
};
static const EnemyMoveSpec em_ScrollOfBiting[]={
 {"CHOMP",1,14,16,1},
 {"CHEW",1,5,6,2},
 {"MORE_TEETH",4,0,0,0},
};
static const EnemyMoveSpec em_Seapunk[]={
 {"SEA_KICK_MOVE",1,11,13,1},
 {"SPINNING_KICK_MOVE",1,2,2,4},
 {"BUBBLE_BURP_MOVE",6,0,0,0},
};
static const EnemyMoveSpec em_SewerClam[]={
 {"PRESSURIZE_MOVE",4,0,0,0},
 {"JET_MOVE",1,10,11,1},
};
static const EnemyMoveSpec em_ShrinkerBeetle[]={
 {"SHRINKER_MOVE",8,0,0,0},
 {"CHOMP_MOVE",1,7,8,1},
 {"STOMP_MOVE",1,13,14,1},
};
static const EnemyMoveSpec em_SkulkingColony[]={
 {"ZOOM_MOVE",1,14,16,1},
 {"ZOOM_MOVE_2",1,14,16,1},
 {"INERTIA_MOVE",5,9,11,1},
 {"PIERCING_STABS_MOVE",1,7,8,2},
};
static const EnemyMoveSpec em_SlimedBerserker[]={
 {"VOMIT_ICHOR_MOVE",16,0,0,0},
 {"LEECHING_HUG_MOVE",12,0,0,0},
 {"SMOTHER_MOVE",1,30,33,1},
 {"FURIOUS_PUMMELING_MOVE",1,4,5,4},
};
static const EnemyMoveSpec em_SlitheringStrangler[]={
 {"CONSTRICT",8,0,0,0},
 {"THWACK",3,7,8,1},
 {"LASH",1,12,13,1},
};
static const EnemyMoveSpec em_SludgeSpinner[]={
 {"OIL_SPRAY_MOVE",9,8,9,1},
 {"SLAM_MOVE",1,11,12,1},
 {"RAGE_MOVE",5,6,7,1},
};
static const EnemyMoveSpec em_SlumberingBeetle[]={
 {"SNORE_MOVE",512,0,0,0},
 {"ROLL_OUT_MOVE",5,16,18,1},
};
static const EnemyMoveSpec em_SnappingJaxfruit[]={
 {"ENERGY_ORB_MOVE",5,3,4,1},
};
static const EnemyMoveSpec em_SneakyGremlin[]={
 {"SPAWNED_MOVE",256,0,0,0},
 {"TACKLE_MOVE",1,9,10,1},
};
static const EnemyMoveSpec em_SoulFysh[]={
 {"BECKON_MOVE",16,0,0,0},
 {"DE_GAS_MOVE",1,16,18,1},
 {"GAZE_MOVE",17,7,8,1},
 {"FADE_MOVE",4,0,0,0},
 {"SCREAM_MOVE",9,13,15,1},
};
static const EnemyMoveSpec em_SoulNexus[]={
 {"SOUL_BURN_MOVE",1,29,31,1},
 {"MAELSTROM_MOVE",1,6,7,4},
 {"DRAIN_LIFE_MOVE",9,18,19,1},
};
static const EnemyMoveSpec em_SpectralKnight[]={
 {"HEX",8,0,0,0},
 {"SOUL_SLASH",1,15,17,1},
 {"SOUL_FLAME",1,3,4,3},
};
static const EnemyMoveSpec em_SpinyToad[]={
 {"PROTRUDING_SPIKES_MOVE",4,0,0,0},
 {"SPIKE_EXPLOSION_MOVE",1,23,25,1},
 {"TONGUE_LASH_MOVE",1,17,19,1},
};
static const EnemyMoveSpec em_Stabbot[]={
 {"STAB_MOVE",9,11,12,1},
};
static const EnemyMoveSpec em_TerrorEel[]={
 {"CRASH_MOVE",1,16,18,1},
 {"THRASH_MOVE",5,3,4,3},
 {"STUN_MOVE",256,0,0,0},
 {"TERROR_MOVE",8,0,0,0},
};
static const EnemyMoveSpec em_TestSubject[]={
 {"RESPAWN_MOVE",36,0,0,0},
 {"BITE_MOVE",1,20,22,1},
 {"SKULL_BASH_MOVE",9,14,16,1},
 {"MULTI_CLAW_MOVE",1,10,11,3},
 {"PHASE3_LACERATE_MOVE",1,10,11,3},
 {"BIG_POUNCE",1,45,45,1},
 {"BURNING_GROWL_MOVE",20,0,0,0},
};
static const EnemyMoveSpec em_TheForgotten[]={
 {"MIASMA",14,0,0,0},
 {"DREAD",1,13,15,1},
};
static const EnemyMoveSpec em_TheInsatiable[]={
 {"LIQUIFY_GROUND_MOVE",20,0,0,0},
 {"THRASH_MOVE",1,8,9,2},
 {"THRASH_MOVE_2",1,8,9,2},
 {"LUNGING_BITE_MOVE",1,28,31,1},
 {"SALIVATE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_TheLost[]={
 {"DEBILITATING_SMOG",12,0,0,0},
 {"EYE_LASERS",1,4,5,2},
};
static const EnemyMoveSpec em_TheObscura[]={
 {"ILLUSION_MOVE",64,0,0,0},
 {"PIERCING_GAZE_MOVE",1,10,11,1},
 {"SAIL_MOVE",4,0,0,0},
 {"HARDENING_STRIKE_MOVE",3,6,7,1},
};
static const EnemyMoveSpec em_ThievingHopper[]={
 {"THIEVERY_MOVE",17,17,19,1},
 {"NAB_MOVE",1,14,16,1},
 {"HAT_TRICK_MOVE",1,21,23,1},
 {"FLUTTER_MOVE",4,0,0,0},
 {"ESCAPE_MOVE",128,0,0,0},
};
static const EnemyMoveSpec em_Toadpole[]={
 {"SPIKE_SPIT_MOVE",1,3,4,3},
 {"WHIRL_MOVE",1,7,8,1},
 {"SPIKEN_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_TorchHeadAmalgam[]={
 {"STRONG_TACKLE_MOVE",1,26,32,1},
 {"TACKLE_2_MOVE",1,18,22,1},
 {"BEAM_MOVE",1,8,8,3},
 {"TACKLE_3_MOVE",1,14,16,1},
 {"TACKLE_4_MOVE",1,14,16,1},
};
static const EnemyMoveSpec em_ToughEgg[]={
 {"HATCH_MOVE",64,0,0,0},
 {"NIBBLE_MOVE",1,4,5,1},
};
static const EnemyMoveSpec em_TrackerRubyRaider[]={
 {"TRACK_MOVE",8,0,0,0},
 {"HOUNDS_MOVE",1,1,1,8},
};
static const EnemyMoveSpec em_Tunneler[]={
 {"BITE_MOVE",1,13,15,1},
 {"BURROW_MOVE",6,0,0,0},
 {"BELOW_MOVE",1,23,26,1},
 {"DIZZY_MOVE",256,0,0,0},
};
static const EnemyMoveSpec em_TurretOperator[]={
 {"UNLOAD_MOVE",1,3,4,5},
 {"UNLOAD_MOVE_2",1,3,4,5},
 {"RELOAD_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_TwigSlimeM[]={
 {"POKEY_POUNCE_MOVE",1,11,12,1},
 {"STICKY_SHOT_MOVE",16,0,0,0},
};
static const EnemyMoveSpec em_TwigSlimeS[]={
 {"TACKLE_MOVE",1,4,5,1},
};
static const EnemyMoveSpec em_TwoTailedRat[]={
 {"SCRATCH_MOVE",1,8,9,1},
 {"DISEASE_BITE_MOVE",1,6,7,1},
 {"SCREECH_MOVE",8,0,0,0},
 {"CALL_FOR_BACKUP_MOVE",64,0,0,0},
};
static const EnemyMoveSpec em_Vantom[]={
 {"INK_BLOT_MOVE",1,7,8,1},
 {"INKY_LANCE_MOVE",1,6,7,2},
 {"DISMEMBER_MOVE",17,26,30,1},
 {"PREPARE_MOVE",4,0,0,0},
};
static const EnemyMoveSpec em_VineShambler[]={
 {"GRASPING_VINES_MOVE",17,8,9,1},
 {"SWIPE_MOVE",1,6,7,2},
 {"CHOMP_MOVE",1,16,18,1},
};
static const EnemyMoveSpec em_WaterfallGiant[]={
 {"PRESSURIZE_MOVE",4,0,0,0},
 {"STOMP_MOVE",13,15,16,1},
 {"RAM_MOVE",5,10,11,1},
 {"SIPHON_MOVE",36,0,0,0},
 {"PRESSURE_GUN_MOVE",5,20,23,1},
 {"PRESSURE_UP_MOVE",5,13,14,1},
 {"ABOUT_TO_BLOW_MOVE",256,0,0,0},
 {"EXPLODE_MOVE",1025,0,0,1},
};
static const EnemyMoveSpec em_Wriggler[]={
 {"NASTY_BITE_MOVE",1,6,7,1},
 {"WRIGGLE_MOVE",20,0,0,0},
 {"SPAWNED_MOVE",256,0,0,0},
};
static const EnemyMoveSpec em_Zapbot[]={
 {"ZAP",1,14,15,1},
};
static const EnemyMoveSpec*enemy_move_spec(EnemyId id,unsigned node){
 switch(id){
 case EnemyId::Aeonglass:return node<3?&em_Aeonglass[node]:nullptr;
 case EnemyId::Architect:return node<1?&em_Architect[node]:nullptr;
 case EnemyId::AssassinRubyRaider:return node<1?&em_AssassinRubyRaider[node]:nullptr;
 case EnemyId::AxeRubyRaider:return node<3?&em_AxeRubyRaider[node]:nullptr;
 case EnemyId::Axebot:return node<3?&em_Axebot[node]:nullptr;
 case EnemyId::BattleFriendV1:return node<1?&em_BattleFriendV1[node]:nullptr;
 case EnemyId::BattleFriendV2:return node<1?&em_BattleFriendV2[node]:nullptr;
 case EnemyId::BattleFriendV3:return node<1?&em_BattleFriendV3[node]:nullptr;
 case EnemyId::BowlbugEgg:return node<1?&em_BowlbugEgg[node]:nullptr;
 case EnemyId::BowlbugNectar:return node<3?&em_BowlbugNectar[node]:nullptr;
 case EnemyId::BowlbugRock:return node<2?&em_BowlbugRock[node]:nullptr;
 case EnemyId::BowlbugSilk:return node<2?&em_BowlbugSilk[node]:nullptr;
 case EnemyId::BruteRubyRaider:return node<2?&em_BruteRubyRaider[node]:nullptr;
 case EnemyId::BygoneEffigy:return node<4?&em_BygoneEffigy[node]:nullptr;
 case EnemyId::Byrdonis:return node<2?&em_Byrdonis[node]:nullptr;
 case EnemyId::CalcifiedCultist:return node<2?&em_CalcifiedCultist[node]:nullptr;
 case EnemyId::CeremonialBeast:return node<6?&em_CeremonialBeast[node]:nullptr;
 case EnemyId::Chomper:return node<2?&em_Chomper[node]:nullptr;
 case EnemyId::CorpseSlug:return node<3?&em_CorpseSlug[node]:nullptr;
 case EnemyId::CrossbowRubyRaider:return node<2?&em_CrossbowRubyRaider[node]:nullptr;
 case EnemyId::Crusher:return node<5?&em_Crusher[node]:nullptr;
 case EnemyId::CubexConstruct:return node<4?&em_CubexConstruct[node]:nullptr;
 case EnemyId::DampCultist:return node<2?&em_DampCultist[node]:nullptr;
 case EnemyId::DecimillipedeSegmentBack:return node<5?&em_DecimillipedeSegmentBack[node]:nullptr;
 case EnemyId::DecimillipedeSegmentFront:return node<5?&em_DecimillipedeSegmentFront[node]:nullptr;
 case EnemyId::DecimillipedeSegmentMiddle:return node<5?&em_DecimillipedeSegmentMiddle[node]:nullptr;
 case EnemyId::DevotedSculptor:return node<2?&em_DevotedSculptor[node]:nullptr;
 case EnemyId::Entomancer:return node<3?&em_Entomancer[node]:nullptr;
 case EnemyId::Exoskeleton:return node<3?&em_Exoskeleton[node]:nullptr;
 case EnemyId::EyeWithTeeth:return node<1?&em_EyeWithTeeth[node]:nullptr;
 case EnemyId::Fabricator:return node<3?&em_Fabricator[node]:nullptr;
 case EnemyId::FakeMerchantMonster:return node<4?&em_FakeMerchantMonster[node]:nullptr;
 case EnemyId::FatGremlin:return node<2?&em_FatGremlin[node]:nullptr;
 case EnemyId::FlailKnight:return node<3?&em_FlailKnight[node]:nullptr;
 case EnemyId::Flyconid:return node<3?&em_Flyconid[node]:nullptr;
 case EnemyId::Fogmog:return node<4?&em_Fogmog[node]:nullptr;
 case EnemyId::FossilStalker:return node<3?&em_FossilStalker[node]:nullptr;
 case EnemyId::FrogKnight:return node<4?&em_FrogKnight[node]:nullptr;
 case EnemyId::FuzzyWurmCrawler:return node<3?&em_FuzzyWurmCrawler[node]:nullptr;
 case EnemyId::GasBomb:return node<1?&em_GasBomb[node]:nullptr;
 case EnemyId::GlobeHead:return node<3?&em_GlobeHead[node]:nullptr;
 case EnemyId::GremlinMerc:return node<3?&em_GremlinMerc[node]:nullptr;
 case EnemyId::Guardbot:return node<1?&em_Guardbot[node]:nullptr;
 case EnemyId::HauntedShip:return node<3?&em_HauntedShip[node]:nullptr;
 case EnemyId::HunterKiller:return node<3?&em_HunterKiller[node]:nullptr;
 case EnemyId::InfestedPrism:return node<4?&em_InfestedPrism[node]:nullptr;
 case EnemyId::Inklet:return node<3?&em_Inklet[node]:nullptr;
 case EnemyId::KinFollower:return node<3?&em_KinFollower[node]:nullptr;
 case EnemyId::KinPriest:return node<4?&em_KinPriest[node]:nullptr;
 case EnemyId::KnowledgeDemon:return node<4?&em_KnowledgeDemon[node]:nullptr;
 case EnemyId::LagavulinMatriarch:return node<5?&em_LagavulinMatriarch[node]:nullptr;
 case EnemyId::LeafSlimeM:return node<2?&em_LeafSlimeM[node]:nullptr;
 case EnemyId::LeafSlimeS:return node<2?&em_LeafSlimeS[node]:nullptr;
 case EnemyId::LivingFog:return node<3?&em_LivingFog[node]:nullptr;
 case EnemyId::LivingShield:return node<2?&em_LivingShield[node]:nullptr;
 case EnemyId::LouseProgenitor:return node<3?&em_LouseProgenitor[node]:nullptr;
 case EnemyId::MagiKnight:return node<5?&em_MagiKnight[node]:nullptr;
 case EnemyId::Mawler:return node<3?&em_Mawler[node]:nullptr;
 case EnemyId::MechaKnight:return node<4?&em_MechaKnight[node]:nullptr;
 case EnemyId::MysteriousKnight:return node<3?&em_MysteriousKnight[node]:nullptr;
 case EnemyId::Myte:return node<3?&em_Myte[node]:nullptr;
 case EnemyId::Nibbit:return node<3?&em_Nibbit[node]:nullptr;
 case EnemyId::Noisebot:return node<1?&em_Noisebot[node]:nullptr;
 case EnemyId::Ovicopter:return node<4?&em_Ovicopter[node]:nullptr;
 case EnemyId::OwlMagistrate:return node<4?&em_OwlMagistrate[node]:nullptr;
 case EnemyId::Parafright:return node<1?&em_Parafright[node]:nullptr;
 case EnemyId::PhantasmalGardener:return node<4?&em_PhantasmalGardener[node]:nullptr;
 case EnemyId::PhrogParasite:return node<2?&em_PhrogParasite[node]:nullptr;
 case EnemyId::PunchConstruct:return node<3?&em_PunchConstruct[node]:nullptr;
 case EnemyId::Queen:return node<6?&em_Queen[node]:nullptr;
 case EnemyId::Rocket:return node<5?&em_Rocket[node]:nullptr;
 case EnemyId::ScrollOfBiting:return node<3?&em_ScrollOfBiting[node]:nullptr;
 case EnemyId::Seapunk:return node<3?&em_Seapunk[node]:nullptr;
 case EnemyId::SewerClam:return node<2?&em_SewerClam[node]:nullptr;
 case EnemyId::ShrinkerBeetle:return node<3?&em_ShrinkerBeetle[node]:nullptr;
 case EnemyId::SkulkingColony:return node<4?&em_SkulkingColony[node]:nullptr;
 case EnemyId::SlimedBerserker:return node<4?&em_SlimedBerserker[node]:nullptr;
 case EnemyId::SlitheringStrangler:return node<3?&em_SlitheringStrangler[node]:nullptr;
 case EnemyId::SludgeSpinner:return node<3?&em_SludgeSpinner[node]:nullptr;
 case EnemyId::SlumberingBeetle:return node<2?&em_SlumberingBeetle[node]:nullptr;
 case EnemyId::SnappingJaxfruit:return node<1?&em_SnappingJaxfruit[node]:nullptr;
 case EnemyId::SneakyGremlin:return node<2?&em_SneakyGremlin[node]:nullptr;
 case EnemyId::SoulFysh:return node<5?&em_SoulFysh[node]:nullptr;
 case EnemyId::SoulNexus:return node<3?&em_SoulNexus[node]:nullptr;
 case EnemyId::SpectralKnight:return node<3?&em_SpectralKnight[node]:nullptr;
 case EnemyId::SpinyToad:return node<3?&em_SpinyToad[node]:nullptr;
 case EnemyId::Stabbot:return node<1?&em_Stabbot[node]:nullptr;
 case EnemyId::TerrorEel:return node<4?&em_TerrorEel[node]:nullptr;
 case EnemyId::TestSubject:return node<7?&em_TestSubject[node]:nullptr;
 case EnemyId::TheForgotten:return node<2?&em_TheForgotten[node]:nullptr;
 case EnemyId::TheInsatiable:return node<5?&em_TheInsatiable[node]:nullptr;
 case EnemyId::TheLost:return node<2?&em_TheLost[node]:nullptr;
 case EnemyId::TheObscura:return node<4?&em_TheObscura[node]:nullptr;
 case EnemyId::ThievingHopper:return node<5?&em_ThievingHopper[node]:nullptr;
 case EnemyId::Toadpole:return node<3?&em_Toadpole[node]:nullptr;
 case EnemyId::TorchHeadAmalgam:return node<5?&em_TorchHeadAmalgam[node]:nullptr;
 case EnemyId::ToughEgg:return node<2?&em_ToughEgg[node]:nullptr;
 case EnemyId::TrackerRubyRaider:return node<2?&em_TrackerRubyRaider[node]:nullptr;
 case EnemyId::Tunneler:return node<4?&em_Tunneler[node]:nullptr;
 case EnemyId::TurretOperator:return node<3?&em_TurretOperator[node]:nullptr;
 case EnemyId::TwigSlimeM:return node<2?&em_TwigSlimeM[node]:nullptr;
 case EnemyId::TwigSlimeS:return node<1?&em_TwigSlimeS[node]:nullptr;
 case EnemyId::TwoTailedRat:return node<4?&em_TwoTailedRat[node]:nullptr;
 case EnemyId::Vantom:return node<4?&em_Vantom[node]:nullptr;
 case EnemyId::VineShambler:return node<3?&em_VineShambler[node]:nullptr;
 case EnemyId::WaterfallGiant:return node<8?&em_WaterfallGiant[node]:nullptr;
 case EnemyId::Wriggler:return node<3?&em_Wriggler[node]:nullptr;
 case EnemyId::Zapbot:return node<1?&em_Zapbot[node]:nullptr;
 default:return nullptr;}
}
static unsigned enemy_move_count(EnemyId id){switch(id){
 case EnemyId::Aeonglass:return 3;
 case EnemyId::Architect:return 1;
 case EnemyId::AssassinRubyRaider:return 1;
 case EnemyId::AxeRubyRaider:return 3;
 case EnemyId::Axebot:return 3;
 case EnemyId::BattleFriendV1:return 1;
 case EnemyId::BattleFriendV2:return 1;
 case EnemyId::BattleFriendV3:return 1;
 case EnemyId::BowlbugEgg:return 1;
 case EnemyId::BowlbugNectar:return 3;
 case EnemyId::BowlbugRock:return 2;
 case EnemyId::BowlbugSilk:return 2;
 case EnemyId::BruteRubyRaider:return 2;
 case EnemyId::BygoneEffigy:return 4;
 case EnemyId::Byrdonis:return 2;
 case EnemyId::CalcifiedCultist:return 2;
 case EnemyId::CeremonialBeast:return 6;
 case EnemyId::Chomper:return 2;
 case EnemyId::CorpseSlug:return 3;
 case EnemyId::CrossbowRubyRaider:return 2;
 case EnemyId::Crusher:return 5;
 case EnemyId::CubexConstruct:return 4;
 case EnemyId::DampCultist:return 2;
 case EnemyId::DecimillipedeSegmentBack:return 5;
 case EnemyId::DecimillipedeSegmentFront:return 5;
 case EnemyId::DecimillipedeSegmentMiddle:return 5;
 case EnemyId::DevotedSculptor:return 2;
 case EnemyId::Entomancer:return 3;
 case EnemyId::Exoskeleton:return 3;
 case EnemyId::EyeWithTeeth:return 1;
 case EnemyId::Fabricator:return 3;
 case EnemyId::FakeMerchantMonster:return 4;
 case EnemyId::FatGremlin:return 2;
 case EnemyId::FlailKnight:return 3;
 case EnemyId::Flyconid:return 3;
 case EnemyId::Fogmog:return 4;
 case EnemyId::FossilStalker:return 3;
 case EnemyId::FrogKnight:return 4;
 case EnemyId::FuzzyWurmCrawler:return 3;
 case EnemyId::GasBomb:return 1;
 case EnemyId::GlobeHead:return 3;
 case EnemyId::GremlinMerc:return 3;
 case EnemyId::Guardbot:return 1;
 case EnemyId::HauntedShip:return 3;
 case EnemyId::HunterKiller:return 3;
 case EnemyId::InfestedPrism:return 4;
 case EnemyId::Inklet:return 3;
 case EnemyId::KinFollower:return 3;
 case EnemyId::KinPriest:return 4;
 case EnemyId::KnowledgeDemon:return 4;
 case EnemyId::LagavulinMatriarch:return 5;
 case EnemyId::LeafSlimeM:return 2;
 case EnemyId::LeafSlimeS:return 2;
 case EnemyId::LivingFog:return 3;
 case EnemyId::LivingShield:return 2;
 case EnemyId::LouseProgenitor:return 3;
 case EnemyId::MagiKnight:return 5;
 case EnemyId::Mawler:return 3;
 case EnemyId::MechaKnight:return 4;
 case EnemyId::MysteriousKnight:return 3;
 case EnemyId::Myte:return 3;
 case EnemyId::Nibbit:return 3;
 case EnemyId::Noisebot:return 1;
 case EnemyId::Ovicopter:return 4;
 case EnemyId::OwlMagistrate:return 4;
 case EnemyId::Parafright:return 1;
 case EnemyId::PhantasmalGardener:return 4;
 case EnemyId::PhrogParasite:return 2;
 case EnemyId::PunchConstruct:return 3;
 case EnemyId::Queen:return 6;
 case EnemyId::Rocket:return 5;
 case EnemyId::ScrollOfBiting:return 3;
 case EnemyId::Seapunk:return 3;
 case EnemyId::SewerClam:return 2;
 case EnemyId::ShrinkerBeetle:return 3;
 case EnemyId::SkulkingColony:return 4;
 case EnemyId::SlimedBerserker:return 4;
 case EnemyId::SlitheringStrangler:return 3;
 case EnemyId::SludgeSpinner:return 3;
 case EnemyId::SlumberingBeetle:return 2;
 case EnemyId::SnappingJaxfruit:return 1;
 case EnemyId::SneakyGremlin:return 2;
 case EnemyId::SoulFysh:return 5;
 case EnemyId::SoulNexus:return 3;
 case EnemyId::SpectralKnight:return 3;
 case EnemyId::SpinyToad:return 3;
 case EnemyId::Stabbot:return 1;
 case EnemyId::TerrorEel:return 4;
 case EnemyId::TestSubject:return 7;
 case EnemyId::TheForgotten:return 2;
 case EnemyId::TheInsatiable:return 5;
 case EnemyId::TheLost:return 2;
 case EnemyId::TheObscura:return 4;
 case EnemyId::ThievingHopper:return 5;
 case EnemyId::Toadpole:return 3;
 case EnemyId::TorchHeadAmalgam:return 5;
 case EnemyId::ToughEgg:return 2;
 case EnemyId::TrackerRubyRaider:return 2;
 case EnemyId::Tunneler:return 4;
 case EnemyId::TurretOperator:return 3;
 case EnemyId::TwigSlimeM:return 2;
 case EnemyId::TwigSlimeS:return 1;
 case EnemyId::TwoTailedRat:return 4;
 case EnemyId::Vantom:return 4;
 case EnemyId::VineShambler:return 3;
 case EnemyId::WaterfallGiant:return 8;
 case EnemyId::Wriggler:return 3;
 case EnemyId::Zapbot:return 1;
 default:return 3;}}
