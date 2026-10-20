// Validate operands before a queued instruction from a save can address arrays.
static bool valid_extension_effect(const State&s,const Effect&e){
    if(e.target<0||e.target>=(int)XOp::Count||e.value< -9999||e.value>9999||e.aux< -9999||e.aux>9999)return false;
    const int a=e.value,b=e.aux;const auto op=(XOp)e.target;
    if(op==XOp::WorldStage){
        if(a<0||a>=int(WorldStage::Count))return false;
        const auto stage=WorldStage(a);
        if(stage==WorldStage::PlayerStart)return b==0||b==1;
        if(stage==WorldStage::DrawRelics||stage==WorldStage::DrawCards||stage==WorldStage::DrawEnemies)return b>=0&&b<s.card_count;
        if(stage==WorldStage::StarsSpent)return b>0&&b<=9999;
        if(stage==WorldStage::OrbPassive)return b>=0&&b<MaxOrbs*(MaxEnemies+1);
        return b==0;
    }
    if(op==XOp::SharedCheckWeakAfterHit)return a>=0&&a<s.enemy_count&&b>=0;
    if(op==XOp::SharedAfterCardBody||op==XOp::SharedAfflictionAfterBody||op==XOp::SharedAttackEnd||op==XOp::SharedAttackStart||op==XOp::SharedAttackReward)return a>=0&&a<s.card_count&&b==0;
    if(op==XOp::PostPlayListener)return s.playing>=0&&a>0&&a<(int)PostPlayActor::Count&&b==0;
    if(op==XOp::AllCharactersLate||op==XOp::AllCharactersBefore||op==XOp::AllCharactersBody)return s.playing>=0&&a==0&&b==0;
    if(op==XOp::AllCharactersFlush||op==XOp::AllCharactersSideEnd||op==XOp::AllCharactersDrawBefore)return a==0&&b==0;
    if(op==XOp::AllCharactersBeetle)return a>=0&&a<s.enemy_count&&b>0&&b<=9999;
    if(e.target>(int)XOp::TurnRelics)return valid_ironclad_effect(s,e)||valid_silent_effect(s,e)||valid_defect_effect(s,e)||valid_regent_effect(s,e)||valid_necro_effect(s,e)||valid_event_card_effect(s,e)||valid_enemy_effect(s,e)||(op==XOp::WRelicHook&&a>=0&&a<MaxWorldRelicHookFrames&&s.world_relics.hook_frames[a].active&&b==0);
    const bool actor=a>=0&&a<s.enemy_count,instance=a>=0&&a<s.card_count;
    switch(op){
    case XOp::Power:return a>=0&&a<XPowerCount&&b>0;
    case XOp::StrengthLoss:case XOp::Demise:return actor&&b>=0;
    case XOp::Offer:return a>=0&&a<=6&&b>=0&&b<=3;
    // Low two bits are free/upgraded; the upper bits request up to three
    // distinct models. Zero retains the original single-card operation.
    case XOp::Generate:return a>=0&&a<=6&&b>=0&&b<=15;
    case XOp::ChooseDraw:return a>=0&&a<=3&&b>=-MaxHand&&b<=MaxCards;
    case XOp::ChooseDiscard:return a==0&&(b==0||b==1);
    case XOp::Transform:return a>=0&&a<=MaxHand&&b==0;
    case XOp::AutoDraw:case XOp::AutoDiscard:return a>=0&&a<=3&&b==0;
    // AutoTop can suspend while reserving a batch. Low ten aux bits count
    // queued reserved instances; bit 10 prevents repeating a pending shuffle.
    case XOp::AutoTop:{
        const int reserved=b&1023;
        return a>=0&&a<=MaxCards&&b>=0&&(b&~2047)==0&&reserved<=MaxCards&&
               (a?a:1)+reserved<=MaxCards;
    }
    case XOp::AutoInstance:case XOp::ReturnNext:case XOp::AfterPlay:return instance&&b==0;
    case XOp::Draw:case XOp::DrawOne:return a>=0&&a<=MaxCards&&(b==0||b==1);
    case XOp::EnchantReplay:return a>=0&&a<=255&&b==0;
    case XOp::Create:{Card c=card_from_token(b);return a>=0&&a<=MaxCards&&b>=0&&c.kind<CardKinds&&card_token(c)==b;}
    case XOp::CopyDamage:return actor&&b==0;
    case XOp::Restless:return a>=0&&a<=MaxHand&&b>=0&&b<=999;
    case XOp::Bomb:return a>=0&&b>=1&&b<=3;
    case XOp::EnemyAct:return a>=0&&a<=s.enemy_count&&b==0;
    case XOp::BoulderTick:return a>=0&&a<MaxPowerInstances&&b==0&&s.expansion.boulder[a]>0;
    case XOp::RandomHits:return a>=0&&b>=0&&b<=999;
    case XOp::ChannelDark:return a>=0&&a<=MaxOrbs&&b==0;
    case XOp::Strength:case XOp::Dexterity:return a>=-9999&&a<=9999&&b==0;
    case XOp::AllDamage:case XOp::EscapeBlock:
    case XOp::PlayerDamage:case XOp::Summon:case XOp::Forge:return a>=0&&b==0;
    default:return a==0&&b==0;
    }
}
