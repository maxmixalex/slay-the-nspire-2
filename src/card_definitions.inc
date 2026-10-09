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
 {"FIEND_FIRE",2,7,0,0,0,3,0,0,0,CardType::Attack,true,0,0,0,true}
};
int card_rarity(unsigned kind){
    static constexpr uint8_t rarity[CardKinds]={1,1,1,2,2,2,3,2,2,3,2,3,3,4,3,3,4,4,3,2,3,2,3,4};
    return kind<CardKinds?rarity[kind]:0;
}
