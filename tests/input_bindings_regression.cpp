#include "input_bindings.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
using namespace spire;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"input_bindings:%d %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main(){
    for(unsigned calculator=0;calculator<2;++calculator){auto bindings=default_input_bindings(calculator!=0);CHECK(valid_input_bindings(bindings));
        const auto original=bindings;
        CHECK(rebind_input(bindings,0,unsigned(InputCommand::Map),'a'));CHECK(bindings.keys[0][unsigned(InputCommand::Map)]=='A'&&bindings.keys[0][unsigned(InputCommand::Draw)]=='M');CHECK(valid_input_bindings(bindings));
        CHECK(rebind_input(bindings,0,unsigned(InputCommand::EndTurn),'M'));CHECK(bindings.keys[0][unsigned(InputCommand::Confirm)]=='M'&&bindings.keys[0][unsigned(InputCommand::Draw)]=='E');CHECK(valid_input_bindings(bindings));
        CHECK(rebind_input(bindings,0,unsigned(InputCommand::Cancel),'M'));CHECK(bindings.keys[0][unsigned(InputCommand::Confirm)]==27&&bindings.keys[0][unsigned(InputCommand::EndTurn)]==27);CHECK(valid_input_bindings(bindings));
        CHECK(!rebind_input(bindings,0,unsigned(InputCommand::Select),'Y')&&!rebind_input(bindings,0,unsigned(InputCommand::TopPanel),'Y'));
        CHECK(rebind_input(bindings,1,unsigned(InputCommand::Confirm),'E'));CHECK(bindings.keys[1][unsigned(InputCommand::Confirm)]=='E'&&bindings.keys[1][unsigned(InputCommand::EndTurn)]==13);CHECK(valid_input_bindings(bindings));
        CHECK(!rebind_input(bindings,1,unsigned(InputCommand::Map),0xffff));
        bindings=original;CHECK(valid_input_bindings(bindings));bindings.keys[1][0]=bindings.keys[1][1];CHECK(!valid_input_bindings(bindings));
    }
    char label[16];input_key_label(InputF12,label,sizeof label);CHECK(!std::strcmp(label,"F12"));input_key_label('Q',label,sizeof label);CHECK(!std::strcmp(label,"Q"));
    std::puts("Original two-bank binding defaults, conflict swaps, linked M/K confirm and calculator adaptation passed");
}
