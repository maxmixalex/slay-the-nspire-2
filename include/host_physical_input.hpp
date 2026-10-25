#pragma once
#include "host_text_input.hpp"
#include "input_bindings.hpp"
namespace spire {
constexpr unsigned CalculatorPhysicalKeyCount=sizeof(CalculatorTextKeys)/sizeof(CalculatorTextKeys[0])+9;
template<class Visit>void calculator_physical_keys(Visit visit){
    unsigned index=0;for(const auto&key:CalculatorTextKeys)visit(index++,key.row,key.mask,normalize_input_key(key.character));
    visit(index++,6,0x200,9); // TAB
    visit(index++,7,0x100,InputShift);
    visit(index++,7,0x200,InputControl);
    visit(index++,6,0x08,InputDoc);
    visit(index++,6,0x20,InputMenu);
    visit(index++,5,0x02,InputVar);
    visit(index++,6,0x01,InputFlag);
    visit(index++,0,0x200,InputHome);
    visit(index++,5,0x80,InputCatalog);
}
// A host samples a matrix; saved bindings use the same normalized key codes
// as desktop/browser. Repeats are emitted for held keys, then App only repeats
// navigation. Releases always retain their original down-event meaning.
template<class Emit>void calculator_physical_edges(const uint16_t*current,const uint16_t*previous,uint8_t*repeat,bool text_mode,Emit emit,bool allow_repeat=true){
    calculator_physical_keys([&](unsigned index,unsigned row,unsigned mask,uint16_t code){
        const bool down=(current[row]&mask)!=0,was=(previous[row]&mask)!=0;
        if(!down){repeat[index]=0;if(was)emit(code,false);}
    });
    calculator_physical_keys([&](unsigned index,unsigned row,unsigned mask,uint16_t code){
        const bool down=(current[row]&mask)!=0,was=(previous[row]&mask)!=0;
        if(!down)return;if(!was){repeat[index]=0;if(!text_mode)emit(code,true);return;}
        if(text_mode||!allow_repeat)return;if(repeat[index]<12)++repeat[index];else repeat[index]=10;
        if(repeat[index]==12)emit(code,true);
    });
}
} // namespace spire
