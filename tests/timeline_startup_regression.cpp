#include "timeline.hpp"
#include "run_modes.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
using namespace spire;
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"Timeline startup:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(int argc,char**argv){
    CHECK(argc==2);std::ifstream input(argv[1]);CHECK(input.good());std::string line;unsigned vectors=0,regions=0,characters=0;
    while(std::getline(input,line)){
        if(line.empty()||line[0]=='#')continue;
        std::vector<std::string> f;std::stringstream fields(line);std::string field;
        while(std::getline(fields,field,'\t'))f.push_back(field);CHECK(f.size()==11);
        TimelineUnlocks unlocked;const bool under=f[0].rfind("underdocks",0)==0;
        if(f[0]=="fresh"||under){unlocked.words[0]=unlocked.words[1]=0;if(under)unlocked.set(Epoch::Underdocks);}
        const bool seen=f[0]=="legacy_seen"||f[0]=="underdocks_seen";
        char canonical[65]{};CHECK(reference::canonicalize_seed(f[1].c_str(),canonical,sizeof canonical));CHECK(f[2]==canonical);
        uint64_t root=0;CHECK(reference::seed_hash(canonical,root));CHECK(root==std::stoull(f[3],nullptr,16));
        reference::Random rng;rng.named(root,"act_selection");
        const unsigned region=timeline_first_act(unlocked,seen?15:0,rng);CHECK(region==unsigned(std::stoul(f[4])));regions|=1u<<region;
        bool random_allowed=true;for(unsigned i=0;i<CharacterKinds;++i)random_allowed&=(i==0||unlocked.has(i==1?Epoch::Silent1:i==2?Epoch::Defect1:i==3?Epoch::Regent1:Epoch::Necrobinder1));
        int character=-1;if(random_allowed){character=int(RunModeCharacters[rng.below(CharacterKinds)]);characters|=1u<<character;}
        CHECK(character==std::stoi(f[5]));CHECK(rng.calls==std::stoul(f[6]));
        for(unsigned i=0;i<4;++i)CHECK(rng.words[i]==std::stoull(f[7+i],nullptr,16));
        ++vectors;
    }
    CHECK(vectors==40&&regions==3&&characters==31);
    std::printf("Original Timeline startup: %u checks / %u original-DLL vectors, both first acts and all five Random outcomes.\n",checks,vectors);
}
