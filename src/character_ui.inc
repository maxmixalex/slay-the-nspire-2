// Character presentation uses the original append-only art/clip directories.
static int character_portrait(Character character){
    const int ids[]={U_CharPortrait,U_SilentPortrait,U_DefectPortrait,U_RegentPortrait,U_NecrobinderPortrait};
    return ids[(unsigned)character<CharacterKinds?(unsigned)character:0];
}
static int character_background(Character character){
    const int ids[]={U_CharacterBackground,U_SilentBackground,U_DefectBackground,U_RegentBackground,U_NecrobinderBackground};
    return ids[(unsigned)character<CharacterKinds?(unsigned)character:0];
}
static int character_energy_image(const Content&content,Character character){
    const int ids[]={0,U_SilentOrb,U_DefectOrb,U_RegentOrb,U_NecrobinderOrb};
    return character==Character::Ironclad?A_Energy:content.ui_image(ids[(unsigned)character<CharacterKinds?(unsigned)character:0]);
}
static int character_map_image(const Content&content,Character character){
    const int ids[]={0,U_SilentMapMarker,U_DefectMapMarker,U_RegentMapMarker,U_NecrobinderMapMarker};
    return character==Character::Ironclad?A_MapMarker:content.ui_image(ids[(unsigned)character<CharacterKinds?(unsigned)character:0]);
}
static int character_animation(Character character,int clip){
    if(clip<AnimHeroIdle||clip>AnimHeroCast)return clip;
    const int first[]={AnimHeroIdle,AnimSilentIdle,AnimDefectIdle,AnimRegentIdle,AnimNecrobinderIdle};
    return first[(unsigned)character<CharacterKinds?(unsigned)character:0]+clip-AnimHeroIdle;
}
static int character_camp_animation(Character character){
    const int ids[]={AnimCampHero,AnimSilentCamp,AnimDefectCamp,AnimRegentCamp,AnimNecrobinderCamp};
    return ids[(unsigned)character<CharacterKinds?(unsigned)character:0];
}
static const char* character_summary(Character character){
    switch(character){
    case Character::Ironclad:return "铁甲军团的幸存者。以力量、格挡与消耗卡牌驾驭战斗。";
    case Character::Silent:return "来自尖塔之外的猎手。用小刀、中毒与灵活的弃牌击败敌人。";
    case Character::Defect:return "觉醒的机械造物。生成并激发充能球，用集中强化它们。";
    case Character::Regent:return "群星王位的继承人。积攒星星、号令仆从，铸造君王之剑。";
    case Character::Necrobinder:return "与亡灵结契的施法者。召唤奥斯提，操控灵魂、虚无与灾厄。";
    }
    return "";
}
static int orb_image(unsigned kind){
    const int ids[]={U_DarkOrb,U_LightningOrb,U_FrostOrb,U_PlasmaOrb,U_GlassOrb};
    return ids[kind<(unsigned)DOrb::Count?kind:0];
}
static const char* orb_name(unsigned kind){
    const char*names[]={"黑暗","闪电","冰霜","等离子","玻璃"};
    return names[kind<(unsigned)DOrb::Count?kind:0];
}
