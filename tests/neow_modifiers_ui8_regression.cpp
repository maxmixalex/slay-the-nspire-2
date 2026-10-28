#include "app.hpp"
#include "original_events.hpp"
#include "world_card_factory.hpp"
#include "world_run.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
static App app;
static State game,before,loaded;
static SaveData bytes,again;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"neow_modifiers_ui8:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
static void same_save(const State&a,const State&b){
    CHECK(save(a,bytes)&&save(b,again));CHECK(bytes.size==again.size&&!std::memcmp(bytes.bytes,again.bytes,bytes.size));
}
static void persisted(){
    CHECK(validate(game)&&save(game,bytes));CHECK(load(loaded,bytes.bytes,bytes.size));same_save(game,loaded);game=loaded;
}
static unsigned begin(uint32_t modifiers){
    new_run(game,0x8d7342,false,Character::Ironclad);const unsigned starting=game.deck_count;
    CHECK(configure_run_options(game,RunMode::Custom,modifiers));CHECK(world_set_seed(game,"UI8NEOW"));
    CHECK(world_begin_run(game));CHECK(game.original_events.id==OriginalEventId::Neow&&game.original_events.active);
    CHECK(original_event_option_count(game)==1&&original_event_option_enabled(game,0));persisted();return starting;
}
static void no_pandora(){
    for(unsigned owner=0;owner<2;++owner)for(unsigned rarity=0;rarity<4;++rarity)
        for(unsigned i=0;i<game.world_relics.bag_sizes[owner][rarity];++i)CHECK(game.world_relics.bags[owner][rarity][i]!=WorldRelicPandorasBox);
}
static void finished(RunModifier modifier,unsigned cards){
    CHECK(game.deck_count==cards&&game.original_events.finished&&game.phase==Phase::Event);
    CHECK(!game.run_options.neow_current&&!game.run_options.draft_remaining&&!(game.run_options.setup_flags&1));
    CHECK(game.run_options.neow_completed&run_modifier_bit(modifier));
    CHECK(original_event_option_count(game)==1&&world_can_travel(game));persisted();
    CHECK(original_event_choose(game,0)&&game.phase==Phase::Event&&game.original_events.active&&game.world.room_open==2&&world_can_travel(game));persisted();
}
static void draft(bool hoarder){
    begin(run_modifier_bit(RunModifier::Draft)|(hoarder?run_modifier_bit(RunModifier::Hoarder):0));
    CHECK(!game.deck_count&&current_neow_modifier(game)==RunModifier::Draft);CHECK(original_event_choose(game,0));
    const unsigned copies=hoarder?3:1;
    for(unsigned round=0;round<10;++round){
        const auto&e=game.original_events;CHECK(game.phase==Phase::OriginalEventChoice&&e.selection==OriginalEventSelection::OfferCard);
        CHECK(e.offer_count==3&&e.select_min==1&&e.select_max==1&&!e.select_cancelable);
        CHECK(game.run_options.draft_remaining==10-round&&game.deck_count==round*copies);
        for(unsigned i=0;i<e.offer_count;++i)CHECK(!card_from_token(e.offers[i]).upgraded);
        before=game;CHECK(!original_event_select(game,nullptr,0));same_save(game,before);
        persisted();app.game=game;CHECK(app.world_choice_is_card_reward()&&app.world_choice_page_size()>=e.offer_count);
        uint16_t choice=uint16_t(round%3);const Card picked=card_from_token(game.original_events.offers[choice]);
        before=game;CHECK(original_event_select(game,&choice,1));
        CHECK(game.deck_count==(round+1)*copies);
        for(unsigned i=round*copies;i<game.deck_count;++i)CHECK(game.deck[i].kind==picked.kind&&game.deck[i].upgraded==picked.upgraded);
        if(round==3){CHECK(save(before,bytes)&&load(loaded,bytes.bytes,bytes.size));CHECK(original_event_select(loaded,&choice,1));same_save(game,loaded);}
    }
    no_pandora();finished(RunModifier::Draft,10*copies);
}
static void sealed(){
    begin(run_modifier_bit(RunModifier::SealedDeck));CHECK(!game.deck_count&&current_neow_modifier(game)==RunModifier::SealedDeck);
    CHECK(original_event_choose(game,0));CHECK(game.original_events.offer_count==30&&game.original_events.select_min==10&&game.original_events.select_max==10&&!game.original_events.select_cancelable);
    persisted();app.game=game;CHECK(!app.world_choice_is_card_reward());uint16_t sorted[MaxWorldUiChoices]{};CHECK(app.world_choice_options(sorted,MaxWorldUiChoices)==30);
    for(unsigned i=1;i<30;++i){
        const Card a=card_from_token(game.original_events.offers[sorted[i-1]]),b=card_from_token(game.original_events.offers[sorted[i]]);
        CHECK(card_rarity(a.kind)<card_rarity(b.kind)||(card_rarity(a.kind)==card_rarity(b.kind)&&app.localized_card_rank(a.kind)<=app.localized_card_rank(b.kind)));
    }
    for(unsigned i=0;i<30;++i){CHECK(!card_from_token(game.original_events.offers[i]).upgraded);for(unsigned j=0;j<i;++j)CHECK(game.original_events.offers[i]!=game.original_events.offers[j]);}
    before=game;CHECK(!original_event_select(game,sorted,9));same_save(game,before);
    uint16_t repeated[10]{};for(unsigned i=0;i<10;++i)repeated[i]=sorted[i];repeated[9]=repeated[0];
    CHECK(!original_event_select(game,repeated,10));same_save(game,before);
    Card expected[10]{};for(unsigned i=0;i<10;++i)expected[i]=card_from_token(game.original_events.offers[sorted[i]]);
    CHECK(original_event_select(game,sorted,10));CHECK(game.deck_count==10);
    for(unsigned i=0;i<10;++i)CHECK(game.deck[i].kind==expected[i].kind&&game.deck[i].upgraded==expected[i].upgraded);
    CHECK(save(before,bytes)&&load(loaded,bytes.bytes,bytes.size));CHECK(original_event_select(loaded,sorted,10));same_save(game,loaded);
    no_pandora();finished(RunModifier::SealedDeck,10);
}
static void automatic(RunModifier modifier,bool hoarder){
    const unsigned starting=begin(run_modifier_bit(modifier)|(hoarder?run_modifier_bit(RunModifier::Hoarder):0));
    const unsigned base=modifier==RunModifier::Insanity?0:starting,copies=hoarder?3:1,amount=(modifier==RunModifier::Insanity?30:5)*copies;
    CHECK(game.deck_count==base&&current_neow_modifier(game)==modifier);before=game;
    CHECK(original_event_choose(game,0));CHECK(game.deck_count==base+amount&&game.original_events.finished);
    for(unsigned i=base;i<game.deck_count;++i){CHECK(!game.deck[i].upgraded);
        if(modifier==RunModifier::AllStar)CHECK(world_card_pool(game.deck[i].kind)==WorldCardPool::Colorless);
        if(modifier==RunModifier::Specialized)CHECK(game.deck[i].kind==game.deck[base].kind&&!std::memcmp(&game.event_cards.deck[i],&game.event_cards.deck[base],sizeof(EventCardFacts)));
    }
    CHECK(save(before,bytes)&&load(loaded,bytes.bytes,bytes.size));CHECK(original_event_choose(loaded,0));same_save(game,loaded);
    if(modifier==RunModifier::Insanity)no_pandora();finished(modifier,base+amount);
}
static void sequence(){
    const uint32_t modifiers=run_modifier_bit(RunModifier::Specialized)|run_modifier_bit(RunModifier::AllStar)|run_modifier_bit(RunModifier::Flight);
    const unsigned starting=begin(modifiers);CHECK(current_neow_modifier(game)==RunModifier::Specialized);CHECK(original_event_choose(game,0));
    CHECK(game.deck_count==starting+5&&!game.original_events.finished&&current_neow_modifier(game)==RunModifier::AllStar);persisted();
    CHECK(original_event_choose(game,0));finished(RunModifier::AllStar,starting+10);
    // Daily records the two good modifiers in their draw order, even if their
    // enum indices are reversed. The bad modifier has no Neow option.
    new_run(game,0x8d7342,false,Character::Ironclad);CHECK(configure_run_options(game,RunMode::Daily,
        run_modifier_bit(RunModifier::Specialized)|run_modifier_bit(RunModifier::AllStar)|run_modifier_bit(RunModifier::Murderous),20750));
    game.run_options.setup_flags=2;CHECK(world_set_seed(game,"24_10_2026_1P")&&world_begin_run(game));
    CHECK(current_neow_modifier(game)==RunModifier::AllStar);persisted();CHECK(original_event_choose(game,0));CHECK(current_neow_modifier(game)==RunModifier::Specialized);
    CHECK(game.run_options.setup_flags==2);persisted();CHECK(original_event_choose(game,0));CHECK(game.original_events.finished&&game.run_options.setup_flags==2);persisted();
}
int main(){
    draft(false);sealed();automatic(RunModifier::Insanity,false);automatic(RunModifier::Specialized,false);automatic(RunModifier::AllStar,false);sequence();
    draft(true);automatic(RunModifier::Insanity,true);automatic(RunModifier::Specialized,true);automatic(RunModifier::AllStar,true);
    std::printf("Neow ten-round Draft, thirty-card Sealed, automatic decks, modifier order, Hoarder and exact saved continuation: %u checks\n",checks);
}
