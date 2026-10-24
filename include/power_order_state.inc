// Stable model identities shared by the core and the original power UI.
// Keep the historical status directory aligned; saves store these IDs, never
// localized strings or pointers. Instanced models use their existing slots.
constexpr int IroncladStatusStart=47,SilentStatusStart=IroncladStatusStart+IPowerCount;
constexpr int CharacterExtraStatusStart=SilentStatusStart+SPowerCount-2;
constexpr int DefectStatusStart=CharacterExtraStatusStart+6,RegentStatusStart=DefectStatusStart+DPowerCount;
constexpr int NecroStatusStart=RegentStatusStart+RPowerCount,FiveCharacterExtraStatusStart=NecroStatusStart+NPowerCount;
constexpr int LegacyStatusKinds=FiveCharacterExtraStatusStart+5;
constexpr int EnemyStatusStart=LegacyStatusKinds,EventCardStatusStart=EnemyStatusStart+EnemyPowerCount;
constexpr int StatusKinds=EventCardStatusStart+EventCardPowerKinds;
constexpr unsigned InstancedPowerFamilies=7;
constexpr unsigned MaxPlayerPowerEntries=StatusKinds+InstancedPowerFamilies*(MaxPowerInstances-1);
constexpr unsigned MaxPowerOrderEntries=MaxPlayerPowerEntries+MaxEnemies*(EnemyPowerCount+NDebuffCount);
struct PowerOrderState {
 uint16_t entries[MaxPowerOrderEntries]{};
 uint16_t count=0;
};
static_assert(StatusKinds<256&&MaxEnemies<16&&MaxPowerInstances==16&&MaxBombs==16&&MaxNightmares==16,
              "Power order tokens use 4 actor + 4 instance + 8 model bits");
static_assert(sizeof(PowerOrderState)<3u*1024u,"Power order adds a bounded sparse model list");
