// The legacy five adapted EventId values remain a separate save prefix.
enum class OriginalEventId:uint8_t {
 None,AbyssalBaths,Amalgamator,AromaOfChaos,BattlewornDummy,BrainLeech,Bugslayer,
 ByrdonisNest,ColorfulPhilosophers,ColossalFlower,CrystalSphere,Darv,
 DenseVegetation,DollRoom,DoorsOfLightAndDark,DrowningBeacon,EndlessConveyor,
 FakeMerchant,FieldOfManSizedHoles,GraveOfTheForgotten,HungryForMushrooms,
 InfestedAutomaton,JungleMazeAdventure,LostWisp,LuminousChoir,MorphicGrove,
 Neow,Nonupeipe,Orobas,Pael,PotionCourier,PunchOff,RanwidTheElder,Reflections,
 RelicTrader,RoomFullOfCheese,RoundTeaParty,SapphireSeed,SelfHelpBook,
 SlipperyBridge,SpiralingWhirlpool,SpiritGrafter,StoneOfAllTime,SunkenStatue,
 SunkenTreasury,Symbiote,TabletOfTruth,Tanx,TeaMaster,Tezcatara,TheArchitect,
 TheFutureOfPotions,TheLanternKey,TheLegendsWereTrue,ThisOrThat,TinkerTime,
 TrashHeap,Trial,UnrestSite,Vakuu,WarHistorianRepy,WaterloggedScriptorium,
 WelcomeToWongos,Wellspring,WhisperingHollow,WoodCarvings,ZenWeaver,Count
};
constexpr unsigned OriginalEventKinds=(unsigned)OriginalEventId::Count-1;
constexpr unsigned MaxOriginalEventOptions=16,MaxOriginalEventSteps=48;
constexpr unsigned MaxOriginalEventOffers=32,MaxCrystalItems=150,CrystalCells=121;
// The source constructor can append fifteen items on each of ten attempts.
// Each attempt subscribes every item again: 15 * (1+...+10) is the strict
// revealed-entry bound; fourteen reward items and three 3-card rewards occur
// per attempt. Preserve every callback, including failed-placement retries.
constexpr unsigned MaxCrystalReveals=825,MaxOriginalEventRewards=770;
constexpr unsigned MaxOriginalEventRewardCards=495;
constexpr uint16_t NoOriginalEventReward=UINT16_MAX;
enum class OriginalEventSelection:uint8_t {
 None,Remove,Upgrade,Transform,Enchant,Duplicate,OfferCard,Potion,Relic,
 Crystal,Merchant,TransferEnchantment,Rewards,Count
};
enum class OriginalEventFilter:uint8_t {
 Any,Removable,Upgradeable,Transformable,BasicStrike,BasicDefend,Basic,
 Attack,Skill,Power,Curse,Enchantment,NotBasic,Count
};
enum class OriginalEventOp:uint8_t {
 None,Damage,Heal,MaxHp,Gold,AddCard,AddCurse,GainRelic,LoseRelic,Potion,
 Select,RandomUpgrade,RandomTransform,CardReward,RandomRelic,RandomPotion,
 Combat,Rest,Finish,Page,Continue,EnchantAll,RemoveKind,Special,ProcurePotion,KillPlayer,Count
};
struct OriginalEventStep {
 OriginalEventOp op=OriginalEventOp::None;
 int16_t a=0,b=0,c=0,d=0,e=0;
};
struct CrystalItemState {
 uint8_t kind=0,x=0,y=0,w=0,h=0,subscriptions=0,revealed=0;
 uint16_t model=0,amount=0;
};
struct OriginalEventReward {
 uint8_t kind=0,claimed=0,count=0; // gold, potion, relic, card reward
 uint16_t first=0,model=0,amount=0;
 // CardReward keeps the creation request for Driftwood, not its old options.
 // flags: subscribed, upgrade after generation, can reroll, rerolled, event RNG.
 uint8_t flags=0,original_count=0,type_mask=0,zero_energy=0;
 uint16_t rarity_mask=0,creation_flags=0;
 uint8_t pools[11]{},pool_count=0,odds=0,source=0;
};
struct OriginalEventState {
 OriginalEventId id=OriginalEventId::None;
 uint8_t active=0,finished=0,act_index=0,page=0;
 uint8_t option_count=0,options[MaxOriginalEventOptions]{};
 char page_key[64]{};
 char option_keys[MaxOriginalEventOptions][64]{};
 int16_t values[16]{};
 uint64_t rng_words[4]{};
 uint32_t rng_calls=0;
 uint8_t step_count=0,step_cursor=0;
 OriginalEventStep steps[MaxOriginalEventSteps]{};
 OriginalEventSelection selection=OriginalEventSelection::None;
 OriginalEventFilter filter=OriginalEventFilter::Any;
 uint8_t select_min=0,select_max=0,select_cancelable=0;
 uint16_t select_enchantment=0;
 int16_t select_amount=0;
 int16_t select_extra=0;
 uint16_t offer_count=0,offers[MaxOriginalEventOffers]{};
 EventCardFacts offer_facts[MaxOriginalEventOffers]{};
 int16_t choice_prices[MaxOriginalEventOptions]{};
 uint16_t selected_count=0,selected[MaxDeck]{},selected_cursor=0;
 uint8_t remembered[MaxDeck]{};
 uint8_t selection_committed=0;
 EventDeckSnapshot selected_cards[MaxDeck]{};
 uint8_t reward_active=0;
 uint16_t reward_count=0,reward_current=NoOriginalEventReward,reward_card_count=0;
 OriginalEventReward rewards[MaxOriginalEventRewards]{};
 uint16_t reward_cards[MaxOriginalEventRewardCards]{};
 EventCardFacts reward_facts[MaxOriginalEventRewardCards]{};
 uint8_t combat_pending=0,resume_after_combat=0;
 uint16_t encounter=0;
 uint8_t crystal_tool=1,crystal_charges=0,crystal_item_count=0;
 uint8_t crystal_clear[CrystalCells]{},crystal_owner[CrystalCells]{};
 CrystalItemState crystal_items[MaxCrystalItems]{};
 uint16_t crystal_revealed_count=0;
 uint8_t crystal_revealed[MaxCrystalReveals]{};
};
struct EventRunFacts {
 uint32_t wongo_total=0,wongo_earned=0;
 uint32_t wins=0,character_wins[5]{};
 uint8_t freed_repy=0,completion_recorded=0;
};
