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
enum class OriginalEventSelection:uint8_t {
 None,Remove,Upgrade,Transform,Enchant,Duplicate,OfferCard,Potion,Relic,
 Crystal,Merchant,TransferEnchantment,Count
};
enum class OriginalEventFilter:uint8_t {
 Any,Removable,Upgradeable,Transformable,BasicStrike,BasicDefend,Basic,
 Attack,Skill,Power,Curse,Enchantment,NotBasic,Count
};
enum class OriginalEventOp:uint8_t {
 None,Damage,Heal,MaxHp,Gold,AddCard,AddCurse,GainRelic,LoseRelic,Potion,
 Select,RandomUpgrade,RandomTransform,CardReward,RandomRelic,RandomPotion,
 Combat,Rest,Finish,Page,Continue,EnchantAll,RemoveKind,Special,Count
};
struct OriginalEventStep {
 OriginalEventOp op=OriginalEventOp::None;
 int16_t a=0,b=0,c=0,d=0,e=0;
};
struct CrystalItemState {
 uint8_t kind=0,x=0,y=0,w=0,h=0,subscriptions=0,revealed=0;
 uint16_t model=0,amount=0;
};
struct OriginalEventState {
 OriginalEventId id=OriginalEventId::None;
 uint8_t active=0,finished=0,act_index=0,page=0;
 uint8_t option_count=0,options[MaxOriginalEventOptions]{};
 char page_key[40]{};
 char option_keys[MaxOriginalEventOptions][40]{};
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
 uint16_t selected_count=0,selected[MaxDeck]{};
 uint8_t combat_pending=0,resume_after_combat=0;
 uint16_t encounter=0;
 uint8_t crystal_tool=1,crystal_charges=0,crystal_item_count=0;
 uint8_t crystal_clear[CrystalCells]{},crystal_owner[CrystalCells]{};
 CrystalItemState crystal_items[MaxCrystalItems]{};
 uint8_t crystal_revealed_count=0,crystal_revealed[MaxCrystalItems]{};
};
struct EventRunFacts {
 uint32_t wongo_total=0,wongo_earned=0;
 uint8_t freed_repy=0,completion_recorded=0;
};
