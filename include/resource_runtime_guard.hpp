#pragma once
#include <stdint.h>

namespace spire {
// A process may try several locale candidates before its first successful
// initialization. Once live, a resource failure is terminal for that process:
// reopening Content or changing profiles cannot authorize a later state write.
class ResourceRuntimeGuard {
    enum class Phase:uint8_t { Starting,Active,Failed };
    Phase phase_=Phase::Starting;
public:
    bool start(bool healthy){
        if(phase_!=Phase::Starting||!healthy)return false;
        phase_=Phase::Active;return true;
    }
    bool allow(bool healthy){
        if(phase_!=Phase::Active)return false;
        if(!healthy){fail();return false;}
        return true;
    }
    void fail(){phase_=Phase::Failed;}
    bool failed()const{return phase_==Phase::Failed;}
    // Use the same boundary around update, each captured input sample, host
    // services and rendering. Mutation callbacks also call allow(), since one
    // App operation can encounter bad resources before trying to save history.
    template<class App,class Action>bool call(App&app,Action action){
        if(!allow(app.content.healthy()))return false;
        action();return allow(app.content.healthy());
    }
};
}
