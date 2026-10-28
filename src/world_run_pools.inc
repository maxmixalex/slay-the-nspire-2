// Declared pool order from config/original-act-pools-0.17.json.
// This table is content routing data, not a claim of implemented behavior.
static constexpr OriginalEventId WorldEvents0[]={OriginalEventId::AromaOfChaos,OriginalEventId::ByrdonisNest,OriginalEventId::DenseVegetation,OriginalEventId::JungleMazeAdventure,OriginalEventId::LuminousChoir,OriginalEventId::MorphicGrove,OriginalEventId::SapphireSeed,OriginalEventId::SunkenStatue,OriginalEventId::TabletOfTruth,OriginalEventId::UnrestSite,OriginalEventId::Wellspring,OriginalEventId::WhisperingHollow,OriginalEventId::WoodCarvings};
static constexpr OriginalEventId WorldAncients0[]={OriginalEventId::Neow};
static constexpr OriginalEventId WorldEvents1[]={OriginalEventId::AbyssalBaths,OriginalEventId::DrowningBeacon,OriginalEventId::EndlessConveyor,OriginalEventId::PunchOff,OriginalEventId::SpiralingWhirlpool,OriginalEventId::SunkenStatue,OriginalEventId::SunkenTreasury,OriginalEventId::DoorsOfLightAndDark,OriginalEventId::TrashHeap,OriginalEventId::WaterloggedScriptorium};
static constexpr OriginalEventId WorldAncients1[]={OriginalEventId::Neow};
static constexpr OriginalEventId WorldEvents2[]={OriginalEventId::Amalgamator,OriginalEventId::Bugslayer,OriginalEventId::ColorfulPhilosophers,OriginalEventId::ColossalFlower,OriginalEventId::FieldOfManSizedHoles,OriginalEventId::InfestedAutomaton,OriginalEventId::LostWisp,OriginalEventId::SpiritGrafter,OriginalEventId::TheLanternKey,OriginalEventId::ZenWeaver};
static constexpr OriginalEventId WorldAncients2[]={OriginalEventId::Orobas,OriginalEventId::Pael,OriginalEventId::Tezcatara};
static constexpr OriginalEventId WorldEvents3[]={OriginalEventId::BattlewornDummy,OriginalEventId::GraveOfTheForgotten,OriginalEventId::HungryForMushrooms,OriginalEventId::Reflections,OriginalEventId::RoundTeaParty,OriginalEventId::Trial,OriginalEventId::TinkerTime};
static constexpr OriginalEventId WorldAncients3[]={OriginalEventId::Nonupeipe,OriginalEventId::Tanx,OriginalEventId::Vakuu};
static constexpr OriginalEventId WorldSharedEvents[]={OriginalEventId::BrainLeech,OriginalEventId::CrystalSphere,OriginalEventId::DollRoom,OriginalEventId::FakeMerchant,OriginalEventId::PotionCourier,OriginalEventId::RanwidTheElder,OriginalEventId::RelicTrader,OriginalEventId::RoomFullOfCheese,OriginalEventId::SelfHelpBook,OriginalEventId::SlipperyBridge,OriginalEventId::StoneOfAllTime,OriginalEventId::Symbiote,OriginalEventId::TeaMaster,OriginalEventId::TheFutureOfPotions,OriginalEventId::TheLegendsWereTrue,OriginalEventId::ThisOrThat,OriginalEventId::WarHistorianRepy,OriginalEventId::WelcomeToWongos};
struct WorldRegionPool { const char*key; uint8_t rooms,weak,event_count,ancient_count; const OriginalEventId*events; const OriginalEventId*ancients; };
static const WorldRegionPool WorldRegionPools[]={
 {"Overgrowth",15,3,13,1,WorldEvents0,WorldAncients0},
 {"Underdocks",15,3,10,1,WorldEvents1,WorldAncients1},
 {"Hive",14,2,10,3,WorldEvents2,WorldAncients2},
 {"Glory",13,2,7,3,WorldEvents3,WorldAncients3},
};
