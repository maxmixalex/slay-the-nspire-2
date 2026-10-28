// Original own potion pool followed by Shared; special-event pools are separate.
static const uint8_t world_potions_ironclad[]={3,47,48};
static const uint8_t world_potions_silent[]={49,50,51};
static const uint8_t world_potions_defect[]={59,60,61};
static const uint8_t world_potions_regent[]={62,63,64};
static const uint8_t world_potions_necrobinder[]={58,57,56};
static const uint8_t world_potions_shared[]={17,18,12,2,19,20,21,22,6,23,24,25,4,26,8,27,1,13,16,11,28,29,30,31,15,32,33,34,35,36,37,38,39,40,41,42,43,44,14,45,5,7,46,9,10};
struct WorldPotionSpan {const uint8_t* ids;uint8_t count;};
static const WorldPotionSpan world_potion_pools[]={{world_potions_ironclad,3},{world_potions_silent,3},{world_potions_defect,3},{world_potions_regent,3},{world_potions_necrobinder,3},{world_potions_shared,45}};
