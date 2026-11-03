#include "app.hpp"
#include "world_run.hpp"
#include "world_card_factory.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App a;static UiProgress old,decoded;static SaveData save_data;static uint8_t bytes[UiProgressCapacity];static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Character progress:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void fresh(){a=App{};a.progress_loaded=true;legacy_progress_initialize(a.progress.timeline);a.progress.character_unlocks=1;}
static void migration(){
 clear_ui_progress(old);legacy_progress_initialize(old.timeline);old.timeline.score=137;old.timeline.ancients_seen=7;
 old.timeline.states[unsigned(Epoch::Neow)]=EpochState::ObtainedNoSlot;old.timeline.obtained_at[unsigned(Epoch::Neow)]=1793552400;
 old.timeline.states[unsigned(Epoch::Silent1)]=EpochState::Obtained;old.timeline.obtained_at[unsigned(Epoch::Silent1)]=1793552500;
 old.characters[unsigned(Character::Silent)].losses=1;old.characters_started=3;
 const size_t n=encode_ui_progress(old,bytes,sizeof bytes);CHECK(n&&bytes[3]=='B');CHECK(decode_ui_progress(bytes,n,decoded));
 CHECK(profile_character_unlocks(decoded)==11);CHECK(!std::memcmp(&old.timeline,&decoded.timeline,sizeof old.timeline));
 decoded.character_unlocks=profile_character_unlocks(decoded);const size_t m=encode_ui_progress(decoded,bytes,sizeof bytes);
 CHECK(m==n+1&&bytes[3]=='C');CHECK(decode_ui_progress(bytes,m,old));CHECK(old.character_unlocks==11&&old.characters[1].losses==1);
 CHECK(old.timeline.score==137&&old.timeline.obtained_at[unsigned(Epoch::Neow)]==1793552400);
 UiProgressSummary summary;CHECK(decode_ui_progress_summary(bytes,m,summary));
 bytes[m-1]=0;CHECK(!decode_ui_progress(bytes,m,old));bytes[m-1]=32;CHECK(!decode_ui_progress(bytes,m,old));
 clear_ui_progress(old);const size_t legacy=encode_ui_progress(old,bytes,sizeof bytes);CHECK(decode_ui_progress(bytes,legacy,decoded));CHECK(profile_character_unlocks(decoded)==31);
 clear_ui_progress(old);legacy_progress_initialize(old.timeline);old.characters_started=1u<<unsigned(Character::Defect);CHECK(profile_character_unlocks(old)&(1u<<unsigned(Character::Defect)));
}
static void independent_unlocks(){
 fresh();CHECK(a.title_menu_count()==4);CHECK(a.character_available(0));for(unsigned i=1;i<=5;++i)CHECK(!a.character_available(i));
 CHECK(a.run_mode_available(RunMode::Custom)&&a.run_mode_available(RunMode::Daily));
 a.character_select=true;a.ui_action(UiAction::SelectSilent);CHECK(a.selected_character==Character::Silent);const auto serial=a.run_number;
 a.ui_action(UiAction::StartCharacter);CHECK(a.run_number==serial&&!a.character_departure.active());
 const Character order[]={Character::Ironclad,Character::Silent,Character::Regent,Character::Necrobinder,Character::Defect};
 for(unsigned i=0;i<5;++i){
  a.choose_character(order[i]);CHECK(a.start_selected_run({},false));CHECK(a.game.original_events.id==OriginalEventId::Neow&&validate(a.game));
  CHECK(save(a.game,save_data));const auto mask=profile_character_unlocks(a.progress);CHECK(!a.progress.current.outcome);
  a.save_current_run();a.return_to_title();CHECK(profile_character_unlocks(a.progress)==mask);
  a.finish_progress(true);CHECK(a.progress.current.outcome==3);CHECK(a.progress.characters[unsigned(order[i])].losses==1);
  if(i+1<5)CHECK(a.character_available(unsigned(order[i+1])));
  a.finish_progress(true);CHECK(a.progress.characters[unsigned(order[i])].losses==1);
  for(unsigned e=0;e<EpochCount;++e)CHECK(a.progress.timeline.states[e]==EpochState::None&&a.progress.timeline.obtained_at[e]==0);
  CHECK(!a.progress.timeline.score);
 }
 CHECK(a.character_available(CharacterKinds)&&a.progress.character_unlocks==31);
}
static void special_modes_do_not_unlock(){
 fresh();RunOptions custom;custom.mode=RunMode::Custom;
 a.choose_character(Character::Silent);CHECK(!a.start_selected_run(custom,false));
 a.choose_character(Character::Ironclad);CHECK(a.start_selected_run(custom,false));
 a.finish_progress(true);CHECK(a.progress.character_unlocks==1&&a.first_standard_run());
 fresh();RunOptions daily;daily.mode=RunMode::Daily;daily.daily_day=20759;
 a.choose_character(Character::Defect);CHECK(a.start_selected_run(daily,false));
 CHECK(a.game.character==Character::Defect&&a.progress.character_unlocks==1);
 CHECK(save(a.game,save_data));a.begin_progress(true);CHECK(a.progress.character_unlocks==1);
 a.finish_progress(true);CHECK(a.progress.character_unlocks==1&&a.first_standard_run());
}
static void pools_ignore_retired_nodes(){
 fresh();a.selected_character=Character::Ironclad;CHECK(a.start_selected_run({},false));
 CardKind full[CardKinds],former[CardKinds];WorldCardOptions opts;const unsigned n=world_card_candidates(a.game,opts,full,CardKinds);
 a.game.world.unlocks.words[0]=a.game.world.unlocks.words[1]=0;
 const unsigned m=world_card_candidates(a.game,opts,former,CardKinds);CHECK(n==m&&!std::memcmp(full,former,n*sizeof(CardKind)));
 bool molten=false;for(unsigned i=0;i<m;++i)molten|=former[i]==MoltenFist;CHECK(molten);
 CHECK(a.game.world.rooms[0].ancient==unsigned(OriginalEventId::Neow));CHECK(a.game.world.maps[0].count>1);
}
int main(){migration();independent_unlocks();special_modes_do_not_unlock();pools_ignore_retired_nodes();std::printf("Independent character progress: %u checks; lossless NSHB/NSHC migration, playable masks, settlement and unrestricted model pools.\n",checks);}
