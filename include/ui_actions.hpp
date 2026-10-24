#pragma once
namespace spire {
enum class UiAction {
    StartCharacter, LibraryCards, LibraryRelics, LibraryPotions,
    PreviousPage, NextPage, ToggleUpgrades, DiscardPotion, TargetFirst,
    TargetSecond, ToggleCardUpgrade, RewardCards, RewardGold, RewardPotion,
    LeaveRewards, Back, SelectIronclad, SelectSilent, FilterAll, FilterIronclad, FilterSilent, FilterColorless, CycleSort, ConfirmChoice, ClearChoice, ConfirmDeckChoice, CancelDeckChoice, PreviousDeckPage, NextDeckPage, PreviousChoicePage, NextChoicePage, OpenShop, CloseShop, LeaveShop, ReadEvent, CloseEventReader, LabCard, LabPotion,
    SelectDefect, SelectRegent, SelectNecrobinder, FilterDefect, FilterRegent, FilterNecrobinder,
    RewardGrimoire, SkipGrimoire, ShowStars, ShowOrbs, ShowOsty, MoreStatus,
    RewardRelic, ConfirmWorldChoice, ClearWorldChoice, PreviousWorldPage, NextWorldPage,
    CrystalSmall, CrystalLarge, SacrificeReward, RerollReward,
    RunSettings, RunAscensionDown, RunAscensionUp, RunRegion, RunSeed, RunSeedClear,
    ToggleMapLegend, MapLegendUnknown, MapLegendShop, MapLegendTreasure,
    MapLegendRest, MapLegendMonster, MapLegendElite,
    LeaveCamp, LeaveWorldRewards,
    GameOverSummary, GameOverMainMenu,
    FrontendAccept, FrontendCancel, SettingsFastMode, SettingsControls,
    SettingsPractice, SettingsReference, SettingsBack,
    TreasureOpen, TreasureTake, TreasureLeave, PreviousCampPage, NextCampPage,
    SettingsGeneralTab, SettingsInputTab,
    SettingsLanguage, SettingsLanguageEnglish, SettingsLanguageChinese,
    SettingsLanguageClose, SettingsCardIndices, SettingsTextEffects,
    SettingsKeyboardOnly, SettingsPhobia, SettingsShakeDown, SettingsShakeUp,
    SettingsReset, SettingsScrollUp, SettingsScrollDown,
    InspectPreviousCard, InspectNextCard,
    OpenCompendium, OpenBestiary, OpenStatistics, OpenHistory, HistoryPrevious, HistoryNext,
    OpenProfiles, SelectProfile1, SelectProfile2, SelectProfile3,
    DeleteProfile1, DeleteProfile2, DeleteProfile3,
    OpenStandardRun, OpenCustomRun, OpenDailyRun, RandomizeRun, StartConfiguredRun
};
}
