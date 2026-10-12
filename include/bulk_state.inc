// Included inside namespace spire, after Card/Pile declarations.
constexpr int MaxPotionSlots=4, MaxAutoDepth=12, MaxBombs=16;
enum class BulkPower : uint8_t {
    RetainHand, Plating, Buffer, Intangible, Ritual, Regen, Clarity, Radiance,
    Gigantic, Duplicator, Automation, Calamity, Mayhem, Nostalgia, Panache,
    PrepTime, Fasten, RollingBoulder, Stratagem, Entropy, Gambit, Count
};
constexpr int BulkPowerKinds=(int)BulkPower::Count;
enum class BulkChoice : uint8_t {
    None, Offer, DrawToHand, DiscardToHand, HandToDraw, DiscardRedraw,
    ExhaustHand, FreeThisCombat, TransformHand, Count
};
enum class BulkOp : uint8_t {
    GrantPower, UnpoweredAll, TemporaryEnemyStrength, MoveRare, Offer,
    RandomGenerated, ChooseDraw, ChooseDiscard, ChooseHandTop, DiscardRedraw,
    ExhaustAny, FreeCombat, TransformHand, AutoplayRandomDraw, AutoplayRandomDiscard,
    AutoplayTop, AutoplayInstance, Draw, EscapeResult, HiddenReplay, CreateShivs,
    CopyDamageOthers, DamageAsBlock, ReturnNextTurn, ProlongBlock, Restless,
    Bomb, ShuffleAll, ShuffleCosts, AllStrikesReplay, FinishPotion,
    TurnDraw, TurnReady, EndSide, NewTurn, GrantPotion, EnemyDemise,
    ExhaustWholeHand, DrawAfterDiscard, PostShuffle, AfterPlay, Strength,
    Dexterity, MoveChoice, FinishFrame, NextEnemy, BeginTurn, DrawGuard, ChooseTransform, RandomHits, Count
};
struct AutoFrame {
    int16_t playing=-1, spent=0, x_value=0, vigor=0, target=0;
    uint16_t remaining=0;
    uint8_t double_attack=0, triple_attack=0, debuff_double=0, original_source=0;
};
struct BulkState {
    int16_t powers[BulkPowerKinds]{};
    // 0 means no override; 1 means zero energy. This makes old zero-filled
    // state load without treating every old card as free.
    uint8_t turn_cost[MaxCards]{}, combat_cost[MaxCards]{}, replay[MaxCards]{};
    uint16_t return_turn[MaxCards]{};
    int16_t enemy_restore[MaxEnemies]{}, enemy_demise[MaxEnemies]{}, enemy_beetle[MaxEnemies]{};
    Pile choices{};
    Card offers[3]{};
    BulkChoice choice=BulkChoice::None;
    uint8_t choice_min=0,choice_max=0,choice_free=0,choice_upgraded=0;
    AutoFrame frames[MaxAutoDepth]{};
    uint8_t depth=0,triple_attack=0,debuff_double=0,enemy_turn=0,potion_active=0;
    int16_t spent=0,x_value=0,target=0,last_drawn=-1,last_unblocked=0,last_damage=0;
    uint16_t replay_remaining=0,cards_played=0,cards_drawn=0,turn_cards=0,previous_turn_cards=0;
    uint16_t panache_count=0,automation_count=0,hp_lost_turn=0,hp_lost_combat=0;
    uint8_t nostalgia_used=0,skip_enemy=0,dead_mask=0;
    int16_t boulder_increment=0;
    int16_t bomb_damage[MaxBombs]{};
    uint8_t bomb_turns[MaxBombs]{};
    uint8_t reward_upgraded[3]{};
    uint8_t tea_ready=0;
    uint16_t turn_flags[RelicKinds]{}, combat_flags[RelicKinds]{};
    uint16_t cards_finished=0;
    uint8_t sides_to_run=0;
};
