#pragma once
#include "profile_assets.hpp"
namespace spire {
constexpr unsigned HudUi7ContentVersion=27,HudUi7AnimationKinds=ProfileAnimationKinds;
constexpr unsigned HudUi7AuxiliaryCount=1945;
struct HudUi7Icon {const char*key;int icon,outline;};
// ImageHelper.GetRoomIconPath uses these run_history images for the top bar.
// Eight existing Ancient collection icons retain their original indices.
static constexpr HudUi7Icon hud_ui7_icons[]={
 {"AEONGLASS_BOSS",1889,1890},
 {"ANCIENT",1891,1892},
 {"CEREMONIAL_BEAST_BOSS",1893,1894},
 {"DARV",1876,1895},
 {"DOORMAKER_BOSS",1896,1897},
 {"ELITE",1898,1899},
 {"EVENT",1900,1901},
 {"KAISER_CRAB_BOSS",1902,1903},
 {"KNOWLEDGE_DEMON_BOSS",1904,1905},
 {"LAGAVULIN_MATRIARCH_BOSS",1906,1907},
 {"MONSTER",1908,1909},
 {"NEOW",1869,1910},
 {"NONUPEIPE",1873,1911},
 {"OROBAS",1870,1912},
 {"PAEL",1871,1913},
 {"QUEEN_BOSS",1914,1915},
 {"REST_SITE",1916,1917},
 {"SHOP",1918,1919},
 {"SOUL_FYSH_BOSS",1920,1921},
 {"TANX",1874,1922},
 {"TEST_SUBJECT_BOSS",1923,1924},
 {"TEZCATARA",1872,1925},
 {"THE_INSATIABLE_BOSS",1926,1927},
 {"THE_KIN_BOSS",1928,1929},
 {"TREASURE",1930,1931},
 {"UNKNOWN_ELITE",1932,1933},
 {"UNKNOWN_MONSTER",1934,1935},
 {"UNKNOWN_SHOP",1936,1937},
 {"UNKNOWN_TREASURE",1938,1939},
 {"VAKUU",1875,1940},
 {"VANTOM_BOSS",1941,1942},
 {"WATERFALL_GIANT_BOSS",1943,1944},
};
inline const HudUi7Icon* hud_ui7_icon(const char*key){
    for(const auto&row:hud_ui7_icons){const char*a=key,*b=row.key;
        while(*a&&*a==*b){++a;++b;}if(!*a&&!*b)return &row;}
    return nullptr;
}
static_assert(ProfileAuxiliaryCount==1889&&HudUi7AuxiliaryCount-ProfileAuxiliaryCount==56,"Top-bar resource order");
}
