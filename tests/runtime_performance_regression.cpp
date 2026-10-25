#include "runtime_performance.hpp"
#include "resource_file_cursor.hpp"
#include "host_input_latch.hpp"
#include "host_physical_input.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>

static unsigned checks=0;
static void check(bool good){++checks;if(!good){std::fprintf(stderr,"Runtime performance check %u failed\n",checks);std::exit(1);}}
struct Sink {std::string text;unsigned calls=0;bool reject=false;};
struct File {unsigned position=0,seeks=0,reads=0;bool seek_fail=false,short_read=false;};
static int seek_file(void*context,long offset,int origin){auto&f=*static_cast<File*>(context);++f.seeks;check(origin==0);if(f.seek_fail)return -1;f.position=(unsigned)offset;return 0;}
static unsigned read_file(void*output,unsigned size,unsigned count,void*context){auto&f=*static_cast<File*>(context);++f.reads;check(size==1);unsigned n=f.short_read&&count?count-1:count;auto*p=static_cast<unsigned char*>(output);for(unsigned i=0;i<n;++i)p[i]=(unsigned char)(f.position+i);f.position+=n;return n;}
static bool sink(void*context,const char*bytes,size_t count){
    auto&s=*static_cast<Sink*>(context);++s.calls;check(count<=256);
    if(s.reject)return false;s.text.append(bytes,count);return true;
}
int main(){
    using namespace spire;
    RuntimePerformance p;RuntimeResourceTiming before{};
    p.read(10,4096,true);p.frame(RuntimeScene::Combat,100,1,2,3,before);
    check(p.resources.calls==0&&p.scenes[(unsigned)RuntimeScene::Combat].interval.count==0);
    p.enabled=true;p.clock_hz=1000;
    for(unsigned delay:{50u,51u,100u,101u,250u,251u}){
        before=p.resources;p.read(7,4096,true,true);p.read(3,128,false);
        p.frame(RuntimeScene::Combat,delay,2,delay-5,3,before);
    }
    const auto&s=p.scenes[(unsigned)RuntimeScene::Combat];
    check(s.interval.count==6&&s.interval.total==803&&s.interval.minimum==50&&s.interval.maximum==251);
    check(s.over_50ms==5&&s.over_100ms==3&&s.over_250ms==1);
    check(s.resource.total==60&&s.read_calls==12&&s.read_bytes==25344);
    check(p.resources.seeks==6&&s.read_seeks==6);
    check(p.resources.failures==6&&p.resources.ticks==60&&p.resources.calls==12);
    // A wrapping down-counter must report the complete interval, not a
    // negative duration or the game loop's deliberate one-second dt clamp.
    const uint32_t started=5,finished=UINT32_MAX-4;
    p.frame(RuntimeScene::Map,started-finished,0,0,0,p.resources);
    check(p.scenes[(unsigned)RuntimeScene::Map].interval.total==10);
    p.frame(static_cast<RuntimeScene>(255),2000,0,0,0,p.resources);
    check(p.scenes[(unsigned)RuntimeScene::Other].interval.maximum==2000);
    static_assert((unsigned)RuntimeScene::Other==7&&unsigned(RuntimeScene::CombatPreparation)==8,
                  "Existing diagnostic scene indexes remain compatible");
    p.frame(RuntimeScene::CombatPreparation,3000,2800,100,100,p.resources);
    check(p.scenes[(unsigned)RuntimeScene::CombatPreparation].interval.total==3000);
    check(p.scenes[(unsigned)RuntimeScene::Combat].interval.total==803);
    RuntimeTiming t;t.add(0);t.add(UINT32_MAX);
    check(t.count==2&&t.minimum==0&&t.maximum==UINT32_MAX&&t.total==UINT32_MAX);
    t.count=UINT32_MAX;t.total=123;t.add(99);check(t.total==123&&t.count==UINT32_MAX);
    p.startup_phases[(unsigned)ResourceStage::Integrity].add(1234);
    Sink output;check(p.write(sink,&output));
    check(output.text.find("\"format\":\"nspire-runtime-perf-1\"")!=std::string::npos);
    check(output.text.find("\"over_50ms\":5,\"over_100ms\":3,\"over_250ms\":1")!=std::string::npos);
    check(output.text.find("\"startup_phases\":{")!=std::string::npos);
    check(output.text.find("\"integrity\":{\"count\":1,\"total\":1234,\"min\":1234,\"max\":1234}")!=std::string::npos);
    check(output.text.find("\"scene\":\"combat_preparation\"")!=std::string::npos);
    check(output.text.front()=='{'&&output.text.substr(output.text.size()-3)=="]}\n");
    check(output.text.size()<8192&&output.calls<33);
    Sink failed;failed.reject=true;check(!p.write(sink,&failed));check(failed.calls==1&&failed.text.empty());
    check(!p.write(nullptr,nullptr));
    p.resources.bytes=UINT64_MAX;Sink large;check(p.write(sink,&large));
    check(large.text.find("18446744073709551615")!=std::string::npos);
    // Exercise observable seek/read ordering against an independent stream,
    // including partial reads, failed seeks, and opening another file.
    ResourceFileCursor cursor;File file;bool sought=false;unsigned char bytes[16]{};
    check(cursor.read(&file,0,bytes,8,seek_file,read_file,sought)&&sought&&file.seeks==1);
    check(bytes[0]==0&&bytes[7]==7);
    check(cursor.read(&file,8,bytes,8,seek_file,read_file,sought)&&!sought&&file.seeks==1);
    check(bytes[0]==8&&bytes[7]==15);
    check(cursor.read(&file,3,bytes,4,seek_file,read_file,sought)&&sought&&file.seeks==2);
    file.short_read=true;check(!cursor.read(&file,7,bytes,4,seek_file,read_file,sought)&&!sought&&!cursor.known);
    file.short_read=false;check(cursor.read(&file,7,bytes,4,seek_file,read_file,sought)&&sought&&file.seeks==3);
    file.seek_fail=true;unsigned reads=file.reads;check(!cursor.read(&file,0,bytes,4,seek_file,read_file,sought)&&sought&&!cursor.known&&file.reads==reads);
    file.seek_fail=false;check(cursor.read(&file,0,bytes,4,seek_file,read_file,sought)&&sought);
    check(!cursor.read(&file,UINT32_MAX,bytes,4,seek_file,read_file,sought)&&!cursor.known&&!sought);
    cursor={};File other;check(cursor.read(&other,4,bytes,4,seek_file,read_file,sought)&&sought&&other.seeks==1&&bytes[0]==4);
    // A press released wholly inside resource work must reach the following
    // update in order. No App/input callbacks run inside the read callback.
    CalculatorInputLatch latch;uint16_t matrix[8]{},queued[8]{},prior[8]{};
    uint8_t repeats[CalculatorPhysicalKeyCount]{};unsigned emitted=0;
    latch.sample(matrix);check(latch.count==0);latch.reset(matrix,true);
    matrix[0]=2;latch.sample(matrix);matrix[0]=0;latch.sample(matrix);
    check(latch.count==2);
    while(latch.pop(queued)){
        calculator_physical_edges(queued,prior,repeats,false,[&](uint16_t code,bool down){
            check(code==13&&down==(emitted==0));++emitted;
        },false);
        for(unsigned i=0;i<8;++i)prior[i]=queued[i];
    }
    check(emitted==2&&prior[0]==0);
    // Overflow preserves the final released matrix, and a launch-key reset
    // never manufactures a press. Identical samples consume no queue slots.
    for(unsigned i=0;i<100;++i){matrix[0]=(i%2)?0:2;latch.sample(matrix);}
    check(latch.count==CalculatorInputLatch::Capacity);
    while(latch.pop(queued)){}check(queued[0]==0);
    matrix[0]=2;latch.reset(matrix,true);latch.sample(matrix);check(latch.count==0);
    matrix[0]=0;latch.sample(matrix);check(latch.pop(queued)&&queued[0]==0);
    latch.reset(matrix,false);matrix[0]=2;latch.sample(matrix);check(latch.count==0);
    // Chunking preserves exact data/cursor order and stops on short reads;
    // its maximum gap is one 8-KiB filesystem operation, not a whole image.
    unsigned char large_bytes[20000]{};File chunked;unsigned polls=0;
    auto poll=[&](){++polls;};
    check(sampled_resource_read(large_bytes,sizeof large_bytes,&chunked,read_file,poll)==sizeof large_bytes);
    check(chunked.reads==3&&polls==4&&chunked.position==sizeof large_bytes);
    for(unsigned i=0;i<sizeof large_bytes;++i)check(large_bytes[i]==uint8_t(i));
    chunked={};chunked.short_read=true;polls=0;
    check(sampled_resource_read(large_bytes,sizeof large_bytes,&chunked,read_file,poll)==8191);
    check(chunked.reads==1&&polls==2&&chunked.position==8191);
    polls=0;check(sampled_resource_read(large_bytes,0,&chunked,read_file,poll)==0&&polls==1);
    std::printf("%u runtime performance checks passed; %zu-byte state; %zu-byte report\n",checks,sizeof p,output.text.size());
}
