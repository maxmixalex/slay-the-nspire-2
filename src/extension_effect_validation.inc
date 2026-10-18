// Validate operands before a queued instruction from a save can address arrays.
static bool valid_extension_effect(const State&s,const Effect&e){
    if(e.target<0||e.target>=(int)XOp::Count||e.value< -9999||e.value>9999||e.aux< -9999||e.aux>9999)return false;
    const int a=e.value,b=e.aux;const auto op=(XOp)e.target;
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
    case XOp::Draw:case XOp::DrawOne:return a>=0&&a<=MaxCards&&b==0;
    case XOp::EnchantReplay:return a>=0&&a<=255&&b==0;
    case XOp::Create:return a>=0&&a<=MaxCards&&b>=0&&b<512&&(b&255)<CardKinds;
    case XOp::CopyDamage:return actor&&b==0;
    case XOp::Restless:return a>=0&&a<=MaxHand&&b>=0&&b<=999;
    case XOp::Bomb:return a>=0&&b>=1&&b<=3;
    case XOp::EnemyAct:return a>=0&&a<=s.enemy_count&&b==0;
    case XOp::BoulderTick:return a>=0&&a<MaxPowerInstances&&b==0&&s.expansion.boulder[a]>0;
    case XOp::RandomHits:return a>=0&&b>=0&&b<=999;
    case XOp::ChannelDark:return a>=0&&a<=MaxOrbs&&b==0;
    case XOp::AllDamage:case XOp::EscapeBlock:case XOp::Strength:case XOp::Dexterity:
    case XOp::PlayerDamage:case XOp::Summon:case XOp::Forge:return a>=0&&b==0;
    default:return a==0&&b==0;
    }
}
