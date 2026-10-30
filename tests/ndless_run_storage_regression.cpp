#include "core.hpp"
#include "power_values.hpp"
#include "resource_transfer.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using namespace spire;
using Bytes=std::vector<uint8_t>;
static unsigned checks;
static const char*scenario="setup";
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Ndless run storage [%s]:%d %s\n",scenario,__LINE__,#x);std::exit(1);}}while(0)
static std::map<std::string,Bytes> files;
static std::string fail_open,fail_close,fail_unlink;
static std::vector<std::string> fail_renames;
static size_t write_limit=SIZE_MAX,read_limit=SIZE_MAX,written=0,read_total=0,peak=0;
static unsigned writes=0,reads=0,opens=0,max_write=0;
static bool reenter=false;
struct File {std::string path;size_t offset=0;};
static void count_peak(){size_t total=0;for(const auto&entry:files)total+=entry.second.size();peak=std::max(peak,total);}
static void*nd_open(const char*path,const char*mode){
 ++opens;if(fail_open==path){fail_open.clear();return nullptr;}
 if(*mode=='w')files[path]={};else if(!files.count(path))return nullptr;
 return new File{path};
}
static unsigned nd_read(void*output,unsigned size,unsigned count,void*handle){
 ++reads;auto&f=*static_cast<File*>(handle);const auto&bytes=files.at(f.path);
 const size_t n=std::min({size_t(size)*count,bytes.size()-f.offset,read_total<read_limit?read_limit-read_total:0});
 if(n)std::memcpy(output,bytes.data()+f.offset,n);f.offset+=n;read_total+=n;return unsigned(n/size);
}
static unsigned nd_write(const void*input,unsigned size,unsigned count,void*handle){
 ++writes;max_write=std::max(max_write,size*count);
 if(reenter){ResourceTransferLease nested(ResourceTransferUse::Bytes);CHECK(!nested);}
 const size_t n=std::min(size_t(size)*count,written<write_limit?write_limit-written:0);written+=n;
 auto&f=*static_cast<File*>(handle);const auto*p=static_cast<const uint8_t*>(input);auto&bytes=files.at(f.path);
 bytes.insert(bytes.end(),p,p+n);f.offset+=n;count_peak();return unsigned(n/size);
}
static int nd_close(void*handle){auto*f=static_cast<File*>(handle);const bool fail=fail_close==f->path;if(fail)fail_close.clear();delete f;return fail?-1:0;}
static int nd_rename(const char*from,const char*to){
 for(auto i=fail_renames.begin();i!=fail_renames.end();++i)if(*i==from){fail_renames.erase(i);return -1;}
 auto found=files.find(from);if(found==files.end()||files.count(to))return -1;
 files.emplace(to,std::move(found->second));files.erase(found);return 0;
}
static int nd_unlink(const char*path){if(fail_unlink==path){fail_unlink.clear();return -1;}return files.erase(path)?0:-1;}
// These are the existing profile helpers. The run callbacks below are the
// exact implementation compiled by the ARM host, not a parallel simulation.
static int profile_file_present(const char*path){void*f=nd_open(path,"rb");return !f?0:nd_close(f)==0?1:-1;}
static bool remove_profile_file(const char*path){const int present=profile_file_present(path);return present==0||(present>0&&nd_unlink(path)==0);}
static const char save_path[]="/game/nSpire2.sav.tns",temp_path[]="/game/nSpire2.tmp.tns",backup_path[]="/game/nSpire2.bak.tns";
// This isolated codec/transaction fixture has no App resource lifecycle.
// The real gate plus these callbacks are covered by resource_failure_regression.
static bool resource_mutation_allowed(){return true;}
#include "../platform/ndless/run_storage.inc"
static State source,target,guard;
static SaveData canonical,again,decoded;
static const char*export_directory=nullptr;
static void reset_io(){fail_open.clear();fail_close.clear();fail_unlink.clear();fail_renames.clear();write_limit=read_limit=SIZE_MAX;written=read_total=peak=0;writes=reads=opens=max_write=0;reenter=false;count_peak();}
static Bytes bytes(const SaveData&s){return Bytes(s.bytes,s.bytes+s.size);}
static uint32_t get_word(const Bytes&b,unsigned at){CHECK(at+4<=b.size());return uint32_t(b[at])|(uint32_t(b[at+1])<<8)|(uint32_t(b[at+2])<<16)|(uint32_t(b[at+3])<<24);}
static void set_word(Bytes&b,unsigned at,uint32_t value){CHECK(at+4<=b.size());for(unsigned i=0;i<4;++i)b[at+i]=uint8_t(value>>(8*i));}
static void export_bytes(const char*name,const Bytes&data){if(!export_directory)return;const std::string path=std::string(export_directory)+"/"+name;
 FILE*f=std::fopen(path.c_str(),"wb");CHECK(f);CHECK(std::fwrite(data.data(),1,data.size(),f)==data.size());CHECK(!std::fclose(f));}
static Bytes stored(const Bytes&raw){
 const char*path="/codec";void*f=nd_open(path,"wb");CHECK(f&&run_storage::write(f,raw.data(),unsigned(raw.size())));CHECK(!nd_close(f));
 Bytes result=files.at(path);files.erase(path);return result;
}
static bool unpack(const Bytes&input){files["/codec"]=input;void*f=nd_open("/codec","rb");CHECK(f);const bool ok=run_storage::read(f,decoded);CHECK(!nd_close(f));files.erase("/codec");return ok;}
static void expect_load_failure(const Bytes&input){files[save_path]=input;guard=target;reset_io();CHECK(!load_game(target));CHECK(!std::memcmp(&guard,&target,sizeof target)&&!resource_transfer_uses);}
static void source_roundtrip(){guard=source;CHECK(save(source,canonical));CHECK(save_game(source));CHECK(!std::memcmp(&source,&guard,sizeof source));CHECK(load_game(target)&&save(target,again));CHECK(bytes(canonical)==bytes(again)&&!resource_transfer_uses);}
static void packet_edges(){
 scenario="packet boundaries, incompressible fallback and exact payload";files.clear();reset_io();uint32_t rng=123;
 const unsigned sizes[]={16,20,124,128,132,252,256,260,508,512,516,1024,4096,SaveCapacity};
 for(unsigned size:sizes)for(unsigned pattern=0;pattern<5;++pattern){Bytes raw(size);
  for(unsigned i=0;i<size;++i){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;
   raw[i]=pattern==0?0:pattern==1?uint8_t(1+(rng%255)):pattern==2?uint8_t(i%3==0?1:0):pattern==3?uint8_t(i%129==0?1:0):uint8_t(rng);}
  const unsigned counted=run_storage::encoded_size(raw.data(),size);CHECK(counted<=size+(size+127)/128+8);
  const auto encoded=stored(raw);CHECK(encoded.size()<=raw.size());CHECK(unpack(encoded)&&decoded.size==size&&!std::memcmp(decoded.bytes,raw.data(),size));
  if(counted<size){CHECK(encoded.size()==counted&&get_word(encoded,0)==0x315a534e&&get_word(encoded,4)==size);}
  else CHECK(encoded==raw);
 }
 scenario="explicit zero/literal lengths across I/O refill";
 for(unsigned length=1;length<=128;++length){Bytes encoded={'N','S','Z','1',0,0,0,0};const unsigned size=512;
  set_word(encoded,4,size);Bytes raw;while(raw.size()<size){const unsigned n=std::min<unsigned>(length,size-unsigned(raw.size()));
   encoded.push_back(uint8_t(n-1));for(unsigned i=0;i<n;++i){encoded.push_back(uint8_t(i+1));raw.push_back(uint8_t(i+1));}}
  CHECK(unpack(encoded)&&decoded.size==raw.size()&&!std::memcmp(decoded.bytes,raw.data(),raw.size()));
 }
}
static void basic_and_failures(){
 scenario="canonical save equality, retained raw backup, exact three-host bytes";files.clear();reset_io();new_run(source,17,false,Character::Defect);
 CHECK(save(source,canonical));files[save_path]=bytes(canonical);const Bytes old_raw=files.at(save_path);source.gold=517;reenter=true;
 source_roundtrip();reenter=false;const Bytes primary=files.at(save_path);CHECK(files.at(backup_path)==old_raw&&primary.size()<canonical.size&&get_word(primary,0)==0x315a534e&&max_write<=512);
 CHECK(!files.count(temp_path));source.gold=618;source_roundtrip();CHECK(files.at(backup_path)==primary&&get_word(files.at(backup_path),0)==0x315a534e);
 const auto good_files=files;source.gold=719;
 scenario="write, close and open failures preserve the committed files";
 for(unsigned fault=0;fault<7;++fault){files=good_files;reset_io();
  if(fault==0)fail_open=temp_path;
  if(fault==1)write_limit=0;
  if(fault==2)write_limit=513;
  if(fault==3)fail_close=temp_path;
  if(fault==4)fail_close=save_path;
  if(fault==5)fail_close=backup_path;
  if(fault==6)fail_unlink=backup_path;
  CHECK(!save_game(source));CHECK(files==good_files&&!resource_transfer_uses);
 }
 scenario="rename failures retain the old primary or its recovery identity";
 files=good_files;reset_io();fail_renames={save_path};CHECK(!save_game(source));CHECK(files.at(save_path)==good_files.at(save_path)&&!files.count(temp_path));
 files=good_files;reset_io();fail_renames={temp_path};CHECK(!save_game(source));CHECK(files.at(save_path)==good_files.at(save_path)&&!files.count(temp_path));
 files=good_files;reset_io();fail_renames={temp_path,backup_path};CHECK(!save_game(source));CHECK(!files.count(save_path)&&files.at(backup_path)==good_files.at(save_path)&&!files.count(temp_path));
 // Keep the historical primary-only restore policy, even when a good backup
 // is present. The wrapper must not silently revive a different run.
 guard=target;CHECK(!load_game(target)&&!std::memcmp(&guard,&target,sizeof target));
 files.clear();reset_io();fail_renames={temp_path};CHECK(!save_game(source)&&files.empty());
 scenario="read close and shared-workspace conflicts do not publish state";
 files=good_files;reset_io();fail_close=save_path;guard=target;CHECK(!load_game(target)&&!std::memcmp(&guard,&target,sizeof target));
 for(unsigned stop:{0u,7u,8u,511u}){files=good_files;reset_io();read_limit=stop;guard=target;CHECK(!load_game(target)&&!std::memcmp(&guard,&target,sizeof target));}
 files=good_files;reset_io();{ResourceTransferLease busy(ResourceTransferUse::Bytes);CHECK(busy);guard=target;CHECK(!load_game(target)&&!save_game(source)&&!opens&&!std::memcmp(&guard,&target,sizeof target));}
 CHECK(!resource_transfer_uses);
 scenario="bad envelopes, truncation, overrun and trailing input";
 reset_io();const Bytes good=good_files.at(save_path);CHECK(get_word(good,0)==0x315a534e);
 for(unsigned n=0;n<8;++n)expect_load_failure(Bytes(good.begin(),good.begin()+n));
 expect_load_failure(Bytes(good.begin(),good.end()-1));
 Bytes bad=good;bad.push_back(0);expect_load_failure(bad);bad=good;bad[3]='2';expect_load_failure(bad);
 for(uint32_t n:{0u,12u,17u,SaveCapacity+4u,0xffffffffu}){bad=good;set_word(bad,4,n);expect_load_failure(bad);}
 bad={'N','S','Z','1',16,0,0,0,255};expect_load_failure(bad);
 bad={'N','S','Z','1',16,0,0,0,15,1,2,3};expect_load_failure(bad);
 CHECK(save(source,canonical));bad=bytes(canonical);set_word(bad,12,255);expect_load_failure(stored(bad));
 bad=bytes(canonical);set_word(bad,4,SaveVersion+1);expect_load_failure(stored(bad));
 bad=bytes(canonical);bad.push_back(0);expect_load_failure(bad);
 scenario="invalid simulation never touches storage";files=good_files;reset_io();source.phase=Phase::Error;CHECK(!save_game(source)&&files==good_files&&!opens&&!resource_transfer_uses);
}
static void old_raw(const char*directory){
 scenario="unchanged genuine raw v3-v13 migrations and modern v17 raw";
 const char*names[]={"v3-suspended.hex","v4-shop.hex","v5-practice.hex","v6-silent-choice.hex","v7-silent-choice.hex","v8-relic-choice.hex","v9-grove-choice.hex","v11-entropy-boulder.hex","v12-entropy-choice.hex","v13-nightmare-havoc-choice.hex"};
 for(const char*name:names){const std::string path=std::string(directory)+"/"+name;FILE*f=std::fopen(path.c_str(),"r");CHECK(f);Bytes input;unsigned v;
  while(std::fscanf(f,"%2x",&v)==1)input.push_back(uint8_t(v));CHECK(!std::ferror(f)&&!std::fclose(f));
  CHECK(load(source,input.data(),input.size())&&save(source,canonical));files.clear();reset_io();files[save_path]=input;
  CHECK(load_game(target)&&save(target,again)&&bytes(canonical)==bytes(again));CHECK(save_game(target)&&files.at(backup_path)==input);
  CHECK(load_game(source)&&save(source,again)&&bytes(canonical)==bytes(again));
 }
 for(unsigned c=0;c<CharacterKinds;++c){new_run(source,100+c,false,Character(c));CHECK(save(source,canonical));files.clear();reset_io();files[save_path]=bytes(canonical);
  CHECK(load_game(target)&&save(target,again)&&bytes(canonical)==bytes(again));}
}
static uint32_t stress_rng=0x9747b28c;
static uint32_t noise(){stress_rng^=stress_rng<<13;stress_rng^=stress_rng>>17;stress_rng^=stress_rng<<5;return stress_rng;}
static int amount(){return 257+int(noise()%9743);}
static void boundary_state(){
 new_run(source,0x41cef1ea,false,Character::Necrobinder);source.phase=Phase::ExtendedChoice;source.floor=source.turn=1;source.enemy_count=MaxEnemies;source.current_node=0;
 source.relics=source.relic_count=source.relic_flash=0;for(const auto&r:relic_definitions)CHECK(gain_relic(source,r.id));
 source.deck_count=MaxDeck;source.card_count=MaxCards;source.playing=-1;
 for(unsigned i=0;i<MaxDeck;++i)source.deck[i]={(CardKind)(i%CardKinds),0};
 source.draw.count=source.expansion.choices.count=MaxCards;source.expansion.depth=MaxAutoFrames;source.expansion.choice=XChoice::Draw;source.expansion.choice_min=source.expansion.choice_max=1;
 for(unsigned i=0;i<MaxCards;++i){CardKind k=CardKind(i%CardKinds);source.cards[i]={k,uint8_t(i%2&&card_upgradeable({k,0}))};source.zones[i]=Zone::Draw;source.draw.ids[i]=source.expansion.choices.ids[i]=uint16_t(i);}
 for(auto&f:source.expansion.frames)f.playing=-1;
 source.effect_count=MaxEffects;for(auto&e:source.effects)e={EffectKind::Extended,0,int16_t(XOp::Noop),0};source.log_count=24;for(auto&l:source.logs)l={LogKind::Turn,1,0};
 for(unsigned i=0;i<MaxNightmares;++i){source.silent.nightmare_active[i]=1;source.silent.power_order[source.silent.power_count++]=uint8_t(SPowerCount+i);source.silent.nightmares[i].card={Strike,0};}
 for(unsigned t=0;t<MaxEnemies;++t){auto&e=source.enemies[t];e={9999,9999,0,1,1,2};e.weak=e.poison=1;
  source.silent.strangle[t]=source.silent.enemy_artifact[t]=1;source.expansion.restore_strength[t]=source.expansion.demise[t]=source.expansion.beetle[t]=source.expansion.doom[t]=1;
  source.regent.conqueror[t]=source.necro.debilitate[t]=source.necro.hang[t]=source.necro.oblivion[t]=source.necro.sic_em[t]=1;}
 CHECK(validate(source));
}
static EventCardFacts stress_facts(){EventCardFacts f{};f.enchantment=EventEnchantment::TezcatarasEmber;f.enchantment_amount=int16_t(amount());f.energy_offset=int16_t(-amount());return f;}
static void dense_state(){
 boundary_state();
 for(unsigned i=0;i<MaxDeck;++i){source.deck[i]={DefectModded,uint8_t(i&1)};source.event_cards.deck[i]=stress_facts();}
 for(unsigned i=0;i<MaxCards;++i){source.cards[i]={DefectModded,uint8_t(i&1)};auto&x=source.expansion;x.turn_cost[i]=uint8_t(1+noise()%100);x.combat_cost[i]=uint8_t(1+noise()%100);x.replay[i]=uint8_t(1+noise()%255);x.return_turn[i]=uint16_t(1+noise()%20000);x.star_turn_free[i]=x.star_combat_free[i]=1;
  source.ironclad.forced_exhaust[i]=1;source.silent.sly[i]=source.silent.retain[i]=3;source.silent.bullet_free[i]=1;
  source.defect.cost_delta[i]=int16_t(-amount());source.defect.turn_origin[i]=int16_t(-amount());source.defect.combat_origin[i]=int16_t(-amount());
  source.necro.ethereal[i]=1;source.necro.cost_delta[i]=int16_t(amount());source.necro.turn_origin[i]=int16_t(-amount());source.necro.combat_origin[i]=int16_t(-amount());
  source.event_cards.cards[i]=stress_facts();source.event_cards.momentum[i]=int16_t(amount());source.event_cards.maul_damage[i]=int16_t(amount());source.event_cards.enchantment_disabled[i]=1;
  source.enemy_world.affliction[i]=1;source.enemy_world.affliction_amount[i]=int16_t(amount());source.enemy_world.card_value[i]=int16_t(amount());source.enemy_world.dampen_restore[i]=1;
  source.world_relics.card_is_dupe[i]=1;
 }
 for(auto&f:source.expansion.frames){f.target=int16_t(noise()%MaxEnemies);f.x=int16_t(amount());f.last_damage=int16_t(amount());f.vigor=int16_t(amount());f.remaining=uint16_t(noise());f.double_attack=f.triple=f.returning=1;}
 for(auto&e:source.effects)e={EffectKind::Damage,int16_t(amount()),int16_t(noise()%MaxEnemies),2};
 auto&w=source.world_relics;w.active=1;w.count=WorldRelicCapacity;w.last_serial=WorldRelicCapacity;
 for(unsigned i=0;i<w.count;++i){auto&r=w.entries[i];r.id=uint16_t(i%WorldRelicKinds);if(r.id==WorldRelicFurCoat)r.id=0;r.serial=uint16_t(i+1);r.floor=uint16_t(noise());r.flags=7;r.skin=3;
  for(auto&v:r.values)v=int32_t(noise()|0x01010101u);for(auto&v:r.marked_points)v=uint8_t(1+noise()%255);
  for(unsigned j=0;j<2;++j){r.card_tokens[j]=uint16_t(card_token({DefectModded,uint8_t(j)}));r.card_facts[j]=stress_facts();}}
 for(auto&f:w.hook_frames){f.active=1;f.count=WorldRelicCapacity;f.cursor=uint16_t(noise()%WorldRelicCapacity);for(unsigned i=0;i<WorldRelicCapacity;++i)f.serials[i]=uint16_t(i+1);for(auto&v:f.args)v=int32_t(noise()|0x01010101u);}
 w.step_count=MaxWorldRelicSteps;w.steps[0].op=WorldRelicOp::TransformAppend;
 for(auto&step:w.steps){step.owner=uint16_t(1+noise()%WorldRelicCapacity);step.a=int32_t(noise()|0x01010101u);step.b=int32_t(noise()|0x01010101u);step.c=int32_t(noise()|0x01010101u);}
 w.transform_count=MaxDeck;for(unsigned i=0;i<MaxDeck;++i){auto&t=w.transforms[i];t.original=DefectModded;t.index=uint16_t(i);t.card.token=uint16_t(card_token({DefectModded,uint8_t(i&1)}));t.card.facts=stress_facts();t.card.genetic=uint16_t(1+noise()%9998);t.card.scythe=uint16_t(1+noise()%9986);}
 w.byrdpip_count=MaxCards;for(unsigned i=0;i<MaxCards;++i)w.byrdpip_cards[i]=uint16_t(i);
 w.group_count=MaxWorldRelicRewardGroups;for(unsigned i=0;i<w.group_count;++i){w.group_active[i]=1;w.group_owner[i]=uint16_t(i+1);}
 w.reward_count=MaxWorldRelicRewards;for(unsigned i=0;i<w.reward_count;++i){auto&r=w.rewards[i];r.group=uint8_t(1+i%w.group_count);r.kind=1;r.count=8;r.rarity_mask=r.creation_flags=65535;
  for(unsigned j=0;j<8;++j){r.tokens[j]=uint16_t(card_token({DefectModded,uint8_t(j&1)}));r.facts[j]=stress_facts();}}
 CHECK(validate(source));
}
static size_t bounded_storage(const char*label,const char*file){
 scenario=label;files.clear();reset_io();source_roundtrip();const auto data=files.at(save_path);CHECK(data.size()<canonical.size&&data.size()==run_storage::encoded_size(canonical.bytes,unsigned(canonical.size)));
 CHECK(max_write<=run_storage::BlockBytes);std::printf("%s: NS17=%zu, stored=%zu, writes=%u, max_write=%u; canonical exact.\n",label,canonical.size,data.size(),writes,max_write);
 export_bytes(file,data);std::string raw_name=std::string(file)+".raw";export_bytes(raw_name.c_str(),bytes(canonical));return data.size();
}
static void capacity(){
 boundary_state();const size_t sparse=bounded_storage("all variable counts and 8 enemy states","bounded-run.sav.tns");CHECK(canonical.size==250964);
 dense_state();const size_t dense=bounded_storage("nonzero signed card metadata, 320 relics, 32 full hooks and 128 rewards","dense-run.sav.tns");
 constexpr size_t history=295347,raw_bound=252928,small=340;
 std::printf("Observed dense 3-profile history atomic peak=%zu; mixed legacy backups=%zu; unconditional raw-bound peak=%zu bytes.\n",4*history+6*dense+small,4*history+3*dense+3*raw_bound+small,4*history+6*raw_bound+small);
 CHECK(sparse<dense&&4*history+6*dense+small<2400000&&4*history+3*dense+3*raw_bound+small<2400000);
}
int main(int argc,char**argv){
 CHECK(argc==2||argc==4);if(argc==4){CHECK(std::string(argv[2])=="--export");export_directory=argv[3];}
 if(std::string(argv[1])=="--measure"){capacity();return 0;}
 packet_edges();basic_and_failures();old_raw(argv[1]);capacity();
 std::printf("Ndless run storage: %u checks passed; raw compatibility, bounded streaming and atomic failures.\n",checks);
}
