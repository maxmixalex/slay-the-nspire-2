// v0.111.0 / 41cef1ea; inspected from the supplied assembly.
const Definition definitions[CardKinds]={
 {"STRIKE_IRONCLAD",1,6,0,0,0,3,0,0,0,CardType::Attack,true},
 {"DEFEND_IRONCLAD",1,0,5,0,0,0,3,0,0,CardType::Skill,false},
 {"BASH",2,8,0,0,2,2,0,0,1,CardType::Attack,true},
 {"ANGER",0,6,0,0,0,2,0,0,0,CardType::Attack,true},
 {"POMMEL_STRIKE",1,9,0,1,0,1,0,1,0,CardType::Attack,true},
 {"SHRUG_IT_OFF",1,0,8,1,0,0,3,0,0,CardType::Skill,false},
 {"BURNING_PACT",1,0,0,2,0,0,0,1,0,CardType::Skill,false},
 {"TRUE_GRIT",1,0,7,0,0,0,2,0,0,CardType::Skill,false},
 {"TWIN_STRIKE",1,5,0,0,0,2,0,0,0,CardType::Attack,true},
 {"INFLAME",1,0,0,0,2,0,0,0,1,CardType::Power,false},
 {"IRON_WAVE",1,5,5,0,0,2,2,0,0,CardType::Attack,true},
 {"WHIRLWIND",-1,5,0,0,0,3,0,0,0,CardType::Attack,false},
 {"FEEL_NO_PAIN",1,0,0,0,3,0,0,0,1,CardType::Power,false},
 {"DARK_EMBRACE",2,0,0,0,1,0,0,0,0,CardType::Power,false},
 // ID order is append-only; NSP2/4 resolves art and strings through its card directory.
 {"BLOODLETTING",0,0,0,0,0,0,0,0,0,CardType::Skill,false,3,2,1,false},
 {"HEMOKINESIS",1,15,0,0,0,5,0,0,0,CardType::Attack,true,2},
 {"IMPERVIOUS",2,0,30,0,0,0,10,0,0,CardType::Skill,false,0,0,0,true},
 {"OFFERING",0,0,0,3,0,0,0,2,0,CardType::Skill,false,6,2,0,true},
 {"BLUDGEON",3,32,0,0,0,10,0,0,0,CardType::Attack,true},
 {"THUNDERCLAP",1,4,0,0,1,3,0,0,0,CardType::Attack,false},
 {"UPPERCUT",2,13,0,0,1,0,0,0,1,CardType::Attack,true},
 {"BODY_SLAM",1,0,0,0,0,0,0,0,0,CardType::Attack,true},
 {"BATTLE_TRANCE",0,0,0,3,0,0,0,1,0,CardType::Skill,false},
 {"FIEND_FIRE",2,7,0,0,0,3,0,0,0,CardType::Attack,true,0,0,0,true},
 {"FINESSE",0,0,4,1,0,0,3,0,0,CardType::Skill,false},
 {"FLASH_OF_STEEL",0,5,0,1,0,3,0,0,0,CardType::Attack,true},
 {"STRIKE_SILENT",1,6,0,0,0,3,0,0,0,CardType::Attack,true},
 {"DEFEND_SILENT",1,0,5,0,0,0,3,0,0,CardType::Skill,false},
 {"NEUTRALIZE",0,3,0,0,1,1,0,0,1,CardType::Attack,true},
 {"SURVIVOR",1,0,8,0,0,0,3,0,0,CardType::Skill,false},
 {"ACROBATICS",1,0,0,3,0,0,0,1,0,CardType::Skill,false},
 {"BACKFLIP",1,0,5,2,0,0,3,0,0,CardType::Skill,false},
 {"DAGGER_THROW",1,9,0,1,0,3,0,0,0,CardType::Attack,true},
 {"DEADLY_POISON",1,0,0,0,5,0,0,0,2,CardType::Skill,true},
 {"POISONED_STAB",1,6,0,0,3,2,0,0,1,CardType::Attack,true},
 {"CLOAK_AND_DAGGER",1,0,6,0,0,0,0,0,0,CardType::Skill,false,0,0,0,false,1,1},
 // Blade Dance is self-exhausting in this pinned STS2 release.
 {"BLADE_DANCE",1,0,0,0,0,0,0,0,0,CardType::Skill,false,0,0,0,true,3,1},
 {"DAGGER_SPRAY",1,4,0,0,0,2,0,0,0,CardType::Attack,false},
 {"DASH",2,10,10,0,0,3,3,0,0,CardType::Attack,true},
 {"LEG_SWEEP",2,0,11,0,2,0,3,0,1,CardType::Skill,true},
 {"FOOTWORK",1,0,0,0,2,0,0,0,1,CardType::Power,false},
 {"NOXIOUS_FUMES",1,0,0,0,2,0,0,0,1,CardType::Power,false},
 {"BOUNCING_FLASK",2,0,0,0,3,0,0,0,0,CardType::Skill,false},
 {"PREPARED",0,0,0,1,0,0,0,1,0,CardType::Skill,false},
 {"SLICE",0,6,0,0,0,3,0,0,0,CardType::Attack,true},
 {"DEFLECT",0,0,4,0,0,0,3,0,0,CardType::Skill,false},
 {"SUCKER_PUNCH",1,8,0,0,1,2,0,0,1,CardType::Attack,true},
 {"ADRENALINE",0,0,0,2,0,0,0,0,0,CardType::Skill,false,0,1,1,true},
 {"ACCURACY",1,0,0,0,4,0,0,0,2,CardType::Power,false},
 {"LEADING_STRIKE",1,3,0,0,0,3,0,0,0,CardType::Attack,true,0,0,0,false,2,0},
 {"SHIV",0,4,0,0,0,2,0,0,0,CardType::Attack,true,0,0,0,true},
 {"IMPATIENCE",0,0,0,2,0,0,0,1,0,CardType::Skill,false},
 {"MASTER_OF_STRATEGY",0,0,0,3,0,0,0,1,0,CardType::Skill,false,0,0,0,true},
 {"SHOCKWAVE",2,0,0,0,3,0,0,0,2,CardType::Skill,false,0,0,0,true},
 {"SCRAWL",1,0,0,0,0,0,0,0,0,CardType::Skill,false,0,0,0,true},
 {"PURITY",0,0,0,3,0,0,0,2,0,CardType::Skill,false,0,0,0,true},
 {"HAND_OF_GREED",2,20,0,0,20,5,0,0,5,CardType::Attack,true},
 {"PANIC_BUTTON",0,0,30,0,2,0,10,0,0,CardType::Skill,false,0,0,0,true},
 {"HEADBUTT",1,9,0,0,0,3,0,0,0,CardType::Attack,true},
 {"SWORD_BOOMERANG",1,3,0,0,3,0,0,0,1,CardType::Attack,false},
 {"PERFECTED_STRIKE",2,6,0,0,2,0,0,0,1,CardType::Attack,true},
 {"FLAME_BARRIER",2,0,12,0,4,0,4,0,2,CardType::Skill,false},
 {"DEMON_FORM",3,0,0,0,3,0,0,0,1,CardType::Power,false},
 {"DODGE_AND_ROLL",1,0,4,0,0,0,2,0,0,CardType::Skill,false},
 {"INFINITE_BLADES",1,0,0,0,1,0,0,0,0,CardType::Power,false},
 {"BACKSTAB",0,11,0,0,0,4,0,0,0,CardType::Attack,true,0,0,0,true},
 {"ESCAPE_PLAN",0,0,3,0,0,0,2,0,0,CardType::Skill,false},
 {"PREDATOR",2,15,0,0,2,5,0,0,0,CardType::Attack,true},

#include "colorless_definitions.inc"
#include "ironclad_definitions.inc"
#include "silent_definitions.inc"
 {"GIANT_ROCK",1,20,0,0,0,4,0,0,0,CardType::Attack,true},
#include "defect_definitions.inc"
#include "regent_definitions.inc"
#include "necro_definitions.inc"
#include "common_token_definitions.inc"
};
int card_rarity(unsigned kind){
static constexpr uint8_t rarity[CardKinds]={
 1,1,1,2,2,2,3,2,2,3,2,3,3,4,3,3,4,4,3,2,3,2,3,4,3,3,1,1,1,1,3,2,2,2,2,2,2,2,3,3,3,3,3,2,2,2,2,4,3,2,7,3,4,3,4,3,4,3,2,2,2,3,4,2,3,3,3,2,4,4,3,4,4,4,3,3,3,3,4,3,4,3,3,4,4,3,4,4,3,4,3,3,3,3,3,3,4,3,4,3,4,4,3,4,3,3,4,3,3,3,3,3,7,7,
#include "character_rarities.inc"
,
#include "all_character_rarities.inc"
};
    return kind<CardKinds?rarity[kind]:0;
}

bool card_colorless(unsigned kind){return (kind>=Alchemize&&kind<=Volley)||kind==Finesse||kind==FlashOfSteel||(kind>=Impatience&&kind<=PanicButton);}
