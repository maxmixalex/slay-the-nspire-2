#pragma once
namespace spire {
enum class UiAction {
    StartCharacter, LibraryCards, LibraryRelics, LibraryPotions,
    PreviousPage, NextPage, ToggleUpgrades, DiscardPotion, TargetFirst,
    TargetSecond, ToggleCardUpgrade, RewardCards, RewardGold, RewardPotion,
    LeaveRewards, Back, SelectIronclad, SelectSilent, FilterAll, FilterIronclad, FilterSilent, FilterColorless, CycleSort, ConfirmChoice, ClearChoice, ConfirmDeckChoice, CancelDeckChoice, PreviousDeckPage, NextDeckPage, PreviousChoicePage, NextChoicePage, OpenShop, CloseShop, LeaveShop, ReadEvent, CloseEventReader, LabCard, LabPotion,
    SelectDefect, SelectRegent, SelectNecrobinder, FilterDefect, FilterRegent, FilterNecrobinder,
    RewardGrimoire, SkipGrimoire, ShowStars, ShowOrbs, ShowOsty, MoreStatus
};
}
