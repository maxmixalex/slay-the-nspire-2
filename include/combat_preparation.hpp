#pragma once
#include <stdint.h>
namespace spire {
// Bounded preparation of the current battle's decoded working set. The
// presentation clock is paused while a bounded batch is decoded per update.
// Counters measure actual source work; the original black transition hides
// preparation and its fade-in clock starts only after these images are ready.
struct CombatPreparation {
    static constexpr unsigned Capacity=288,PageBudget=640,FirstCapacity=28,ImagesPerUpdate=4;
    static constexpr unsigned UiCapacity=256,UpdateTicks=819,MaximumSteps=1024,OfflineSteps=256;
    uint32_t images[Capacity]{},first[FirstCapacity]{};
    uint16_t ui_images[UiCapacity]{};
    uint32_t seed=0,read_calls=0;
    uint64_t read_bytes=0;
    int floor=-1,combat=-1,encounter=-1;
    unsigned count=0,cursor=0,pages=0,first_count=0,cold_images=0,updates=0;
    uint16_t ui_count=0,ui_cursor=0;
    uint8_t ui_stage=0,card=0;
    bool text_ready=false;
    bool initialized=false,active=false,shown=false,bounded=false;
    void reset(){*this=CombatPreparation{};}
};
}
