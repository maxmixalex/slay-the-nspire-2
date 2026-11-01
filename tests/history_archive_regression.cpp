#include "history_archive_store.hpp"
#include <vector>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"history archive:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
struct Disk {
 std::vector<uint8_t> bytes;unsigned allowance=~0u;bool fail_flush=false;
 static bool length(void*p,uint32_t&n){n=static_cast<Disk*>(p)->bytes.size();return true;}
 static bool read(void*p,uint32_t at,void*out,unsigned n){auto&b=static_cast<Disk*>(p)->bytes;if(at>b.size()||n>b.size()-at)return false;std::memcpy(out,b.data()+at,n);return true;}
 static bool write(void*p,uint32_t at,const void*in,unsigned n){auto&d=*static_cast<Disk*>(p);const unsigned take=n<d.allowance?n:d.allowance;d.allowance-=take;if(at+take>d.bytes.size())d.bytes.resize(at+take);if(take)std::memcpy(d.bytes.data()+at,in,take);return take==n;}
 static bool flush(void*p){auto&d=*static_cast<Disk*>(p);if(d.fail_flush){d.fail_flush=false;return false;}return true;}
 history_archive::IO io(){return {this,length,read,write,flush};}
};
static UiRunHistory run;static UiHistoryJournal journal;static UiHistoryPage page;
static uint8_t bytes[UiProgressCapacity];
static void fixture(unsigned serial,unsigned first,unsigned floors,unsigned events){
 run={};journal={};run.serial=serial;run.outcome=2;run.floor_count=floors;run.facts.version=1;run.facts.complete=1;
 std::memcpy(run.build_id,UiCurrentBuildId,sizeof UiCurrentBuildId);
 auto&o=journal.owners[7];o.serial=serial;
 for(unsigned i=0;i<floors;++i){run.floors[i].floor=first+i;run.floors[i].point=i;run.floors[i].act=i%3;o.observe(i,true);}
 for(unsigned i=0;i<events;++i)CHECK(journal.append(7,i%floors,RunHistoryEvent::CardObtained,card_token({Strike,0})));
}
static unsigned encode(){const auto n=encode_ui_history_page(run,journal,bytes,sizeof bytes);CHECK(n);return n;}
static void append(Disk&d){auto io=d.io();UiHistoryArchiveQuery q{UiHistoryArchiveAction::Append};q.completed=run.outcome!=0;unsigned n=encode();CHECK(history_archive::access(io,q,bytes,n)==UiHistoryArchiveResult::Complete);}
static bool read(Disk&d,UiHistoryArchiveQuery&q){auto io=d.io();unsigned n=sizeof bytes;auto result=history_archive::access(io,q,bytes,n);if(result==UiHistoryArchiveResult::Missing)return false;CHECK(result==UiHistoryArchiveResult::Complete);CHECK(decode_ui_history_page(bytes,n,page));page.cursor=q.cursor;return true;}
int main(){
 Disk d;
 for(unsigned i=1;i<=70;++i){fixture(i,1,96,4096);append(d);}
 UiHistoryArchiveQuery q{UiHistoryArchiveAction::Latest,70};CHECK(read(d,q));
 for(unsigned i=70;i;--i){CHECK(page.run.serial==i&&page.run.floor_count==96&&page.journal.count==4096);q={UiHistoryArchiveAction::OlderRun,i};if(i>1)CHECK(read(d,q));else CHECK(!read(d,q));}
 fixture(71,1,96,4096);run.outcome=0;append(d);fixture(71,97,96,4096);run.outcome=0;append(d);fixture(71,193,7,32);append(d);
 q={UiHistoryArchiveAction::Latest,71};CHECK(read(d,q)&&page.run.floors[0].floor==193);
 q.action=UiHistoryArchiveAction::OlderPage;CHECK(read(d,q)&&page.run.floors[0].floor==97);
 CHECK(read(d,q)&&page.run.floors[0].floor==1);CHECK(!read(d,q));
 // Reopen is stateless; an exact retry is idempotent even after other records.
 const size_t original=d.bytes.size();fixture(1,1,96,4096);append(d);CHECK(d.bytes.size()==original);
 // Later RAM journal eviction must not replace the complete first archive.
 fixture(1,1,96,0);append(d);CHECK(d.bytes.size()==original);
 q={UiHistoryArchiveAction::Latest,1};CHECK(read(d,q)&&page.journal.count==4096);
 // Fail every write byte through payload and commit publication. Every old
 // record stays reachable; retry either commits once or recognizes a commit
 // whose close/acknowledgement failed. No process-local index is required.
 fixture(72,1,2,3);const unsigned size=encode();const Disk base=d;
 for(unsigned cut=0;cut<24+size+32;++cut){Disk failure=base;failure.allowance=cut;auto io=failure.io();UiHistoryArchiveQuery put{UiHistoryArchiveAction::Append};put.completed=run.outcome!=0;unsigned n=encode();CHECK(history_archive::access(io,put,bytes,n)==UiHistoryArchiveResult::Error);
  q={UiHistoryArchiveAction::Latest,70};CHECK(read(failure,q)&&page.run.serial==70);failure.allowance=~0u;append(failure);
  q={UiHistoryArchiveAction::Latest,72};CHECK(read(failure,q)&&page.journal.count==3);const auto after=failure.bytes.size();append(failure);CHECK(failure.bytes.size()==after);
 }
 Disk failed=base;failed.fail_flush=true;auto io=failed.io();UiHistoryArchiveQuery put{UiHistoryArchiveAction::Append};put.completed=run.outcome!=0;unsigned n=encode();CHECK(history_archive::access(io,put,bytes,n)==UiHistoryArchiveResult::Error);append(failed);
 // The first file creation must also recover after any partially written
 // reservation, initial empty head, record, or final commit head.
 for(unsigned cut=0;cut<64+32+24+size+32;++cut){Disk fresh;fresh.allowance=cut;auto fresh_io=fresh.io();UiHistoryArchiveQuery initial{UiHistoryArchiveAction::Append};initial.completed=1;unsigned length=encode();
  CHECK(history_archive::access(fresh_io,initial,bytes,length)==UiHistoryArchiveResult::Error);
  fresh.allowance=~0u;append(fresh);q={UiHistoryArchiveAction::Latest,72};CHECK(read(fresh,q)&&page.run.serial==72&&page.journal.count==3);
 }
 Disk corrupt;corrupt.bytes.resize(64);corrupt.bytes[0]=0xff;auto corrupt_io=corrupt.io();UiHistoryArchiveQuery broken{UiHistoryArchiveAction::Append};n=encode();
 CHECK(history_archive::access(corrupt_io,broken,bytes,n)==UiHistoryArchiveResult::Error);
 // Every truncation of an individual page rejects, without touching live UI.
 unsigned npage=encode();for(unsigned i=0;i<npage;++i)CHECK(!decode_ui_history_page(bytes,i,page));CHECK(decode_ui_history_page(bytes,npage,page));
 std::printf("%u checks; 70 runs, 199 floors, 4096 events/page, torn writes/reopen/idempotent retry; run=%zu journal=%zu view=%zu\n",checks,sizeof(UiRunHistory),sizeof(UiHistoryJournal),sizeof(UiHistoryPage));
}
