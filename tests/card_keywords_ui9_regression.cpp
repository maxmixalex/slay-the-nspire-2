#include "../src/app.cpp"
#include "content_fixture.hpp"
#include "resource_transfer.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static App a;
static State before;
static unsigned checks;
static bool actual;
static const char*captures;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"card keywords:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
#include "ftue_fixture.hpp"
static unsigned word(const uint8_t*p){return unsigned(p[0])|unsigned(p[1])<<8|unsigned(p[2])<<16|unsigned(p[3])<<24;}
static Bytes keyword_fixture(){
 auto b=fixture();const unsigned directory=40+word(b.data()+16)*16+word(b.data()+20)*16;
 const auto text=[&](unsigned id,const char*value){put(b,directory+id*8,unsigned(b.size()));put(b,directory+id*8+4,unsigned(std::strlen(value)));b.insert(b.end(),value,value+std::strlen(value)+1);};
 const unsigned indexes[]={2239,2232,2235,2237,2228,2230,2226};const char*titles[]={"Unplayable","Innate","Retain","Sly","Ethereal","Exhaust","Eternal"};
 for(unsigned i=0;i<7;++i)text(indexes[i],titles[i]);text(2233,".");text(6034,"Replay {Times}.");
 text(1,"Body mentions [gold]Exhaust[/gold] without being a keyword paragraph.");put(b,12,unsigned(b.size()));return b;
}
static unsigned paragraphs(const char*text,const card_keyword_text::Labels&labels,unsigned keyword){
 unsigned count=0;for(const char*p=text;*p;){const char*end=p;while(*end&&*end!='\n')++end;
  if(card_keyword_text::paragraph(p,end,labels.title[keyword],labels.period,false))++count;p=*end?end+1:end;}return count;
}
static void check_flags(const char*text,unsigned flags,const card_keyword_text::Labels&labels){
 for(unsigned i=0;i<7;++i)CHECK(paragraphs(text,labels,i)==unsigned(bool(flags&card_keyword_text::Flags[i])));
 // Source beforeDescription is prepended; afterDescription is appended.
 std::string prefix,suffix;for(unsigned i=0;i<7;++i)if(flags&card_keyword_text::Flags[i]){
  const std::string line=std::string("[gold]")+labels.title[i]+"[/gold]"+labels.period;
  if(i<5){if(!prefix.empty())prefix+='\n';prefix+=line;}else{if(!suffix.empty())suffix+='\n';suffix+=line;}}
 CHECK(prefix.empty()||!std::strncmp(text,prefix.c_str(),prefix.size()));const size_t length=std::strlen(text);CHECK(suffix.empty()||(length>=suffix.size()&&!std::strcmp(text+length-suffix.size(),suffix.c_str())));
}
static void snapshot(){std::memcpy(&before,&a.game,sizeof before);}
static void unchanged(){CHECK(!std::memcmp(&before,&a.game,sizeof before));}
static void capture(const char*name){if(!captures)return;const std::string path=std::string(captures)+"/"+name+".ppm";FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);std::fprintf(f,"P6\n320 240\n255\n");
 for(auto color:a.pixels){const uint8_t p[]={uint8_t((color>>11)*255/31),uint8_t(((color>>5)&63)*255/63),uint8_t((color&31)*255/31)};CHECK(std::fwrite(p,1,3,f)==3);}CHECK(!std::fclose(f));}
static void all_canonical(){
 const auto labels=wt_card_keyword_labels(a.content);for(const auto&title:labels.title)CHECK(*title);CHECK(*labels.period);
 a.library=true;snapshot();uint32_t fingerprint=2166136261u;
 for(unsigned kind=0;kind<CardKinds;++kind)for(unsigned upgraded=0;upgraded<2;++upgraded){
  const Card card{CardKind(kind),uint8_t(upgraded)};const unsigned flags=card_keywords(card);fingerprint=(fingerprint^flags)*16777619u;
  CHECK(flags==card_keywords(a.game,card,-1,MaxDeck));char text[8192]{};a.card_description(card,-1,text,sizeof text);CHECK(*text);check_flags(text,flags,labels);
  // Exercise the actual common face path for all558 base/upgrade models,
  // not only the text formatter. No canonical gameplay is adopted here.
  Canvas c{a};c.rect(0,0,320,240,rgb(18,21,26));draw_card_face(c,card,132,68,57,80,cost(card));
  CHECK(a.content.healthy());
  if(kind==Purity||kind==Scrawl){char name[64]{};std::snprintf(name,sizeof name,"keywords-%s-%u",kind==Purity?"purity":"scrawl",upgraded);capture(name);}
 }
 CHECK(fingerprint==0x4f58d566u);unchanged();
}
static void mutable_facts(){
 const auto labels=wt_card_keyword_labels(a.content);a.library=false;auto&s=a.game;s.phase=Phase::Combat;s.card_count=1;s.deck_count=1;s.playing=0;s.cards[0]={Purity,0};s.deck[0]={Purity,0};s.zones[0]=Zone::Hand;
 s.silent.retain[0]=1;s.silent.sly[0]=1;s.necro.ethereal[0]=1;s.expansion.replay[0]=2;s.event_cards.cards[0]={};s.event_cards.deck[0]={};
 char text[8192]{};snapshot();a.card_description(s.cards[0],-1,text,sizeof text,0);check_flags(text,1|4|8|16,labels);unchanged();
 const char*replay=std::strstr(text,"2");CHECK(replay);const std::string exhaust=std::string("[gold]")+labels.title[5]+"[/gold]"+labels.period;CHECK(replay<std::strstr(text,exhaust.c_str()));
 Canvas c{a};c.rect(0,0,320,240,rgb(18,21,26));draw_card_face(c,s.cards[0],132,68,57,80,cost(s,0),-2,0);capture("keywords-retain-sly-ethereal-replay");unchanged();
 // HexPower.TryModifyKeywordsInCombat adds Ethereal only to its own
 // Hexed model. It composes with temporary Ethereal without duplication.
 s.enemy_world.active=1;s.enemy_world.affliction[0]=unsigned(EnemyAffliction::Hexed);s.necro.ethereal[0]=0;
 snapshot();a.card_description(s.cards[0],-1,text,sizeof text,0);check_flags(text,1|4|8|16,labels);unchanged();
 s.necro.ethereal[0]=1;snapshot();a.card_description(s.cards[0],-1,text,sizeof text,0);check_flags(text,1|4|8|16,labels);unchanged();
 // Canonical and permanent-deck previews must not borrow the matching
 // playing model's temporary powers, replay or affliction.
 a.card_description(s.cards[0],-1,text,sizeof text);check_flags(text,1|4,labels);unchanged();
 a.card_description(s.deck[0],-1,text,sizeof text,-1,0);check_flags(text,1|4,labels);unchanged();
 a.library=true;snapshot();a.card_description(s.cards[0],-1,text,sizeof text,0);check_flags(text,1|4,labels);unchanged();a.library=false;
 for(unsigned e=0;e<unsigned(EventEnchantment::Count);++e){const auto enchantment=EventEnchantment(e);EventCardFacts facts{};facts.enchantment=enchantment;facts.enchantment_amount=1;
  unsigned expected=5;if(enchantment==EventEnchantment::SoulsPower)expected&=~1u;if(enchantment==EventEnchantment::RoyallyApproved)expected|=2;if(enchantment==EventEnchantment::TezcatarasEmber)expected|=64;
  CHECK(event_card_keywords(facts,5)==expected);snapshot();a.card_description({Purity,0},-1,text,sizeof text,-1,-1,&facts);check_flags(text,expected,labels);unchanged();
  if(expected&1){const char*modifier=std::strstr(text,"[purple]");if(modifier)CHECK(modifier<std::strstr(text,exhaust.c_str()));}
 }
 // The State overload and pure facts overload remain identical for every
 // supported model and enchantment; the observed pre-refactor mask hash
 // above independently fixes the complete canonical rule set.
 for(unsigned e=0;e<unsigned(EventEnchantment::Count);++e){s.event_cards.cards[0].enchantment=EventEnchantment(e);snapshot();
  for(unsigned k=0;k<CardKinds;++k){const Card card{CardKind(k),0};if(k==Purity)CHECK(card_keywords(s,card,0)==event_card_keywords(s.event_cards.cards[0],card_keywords(card)));else CHECK(card_keywords(s,card,0)==card_keywords(card));}unchanged();}
}
int main(int argc,char**argv){
 Bytes data;if(argc>1){FILE*f=std::fopen(argv[1],"rb");CHECK(f);data.resize(spire_test::pack_size(f));CHECK(std::fread(data.data(),1,data.size(),f)==data.size());std::fclose(f);actual=true;}else data=keyword_fixture();
 CHECK(a.init(data.data(),data.size(),{}));if(argc>2)captures=argv[2];all_canonical();mutable_facts();CHECK(a.content.healthy());
 std::printf("card keywords: %u checks passed (%s)\n",checks,actual?"actual":"synthetic");
}
