#pragma once
#include <stddef.h>
#include <stdint.h>
namespace spire {
// Platform-independent physical key codes. Printable keys retain ASCII;
// letters are uppercase, and no OS keycode leaks into saved preferences.
enum InputKey:uint16_t {
    InputLeft=256,InputRight,InputUp,InputDown,InputInsert,InputDelete,
    InputHome,InputEnd,InputPageUp,InputPageDown,
    InputF1=272,InputF12=283,InputShift=288,InputControl,InputAlt,InputMeta
};
enum class InputCommand:uint8_t {
    Select,Cancel,Map,TopPanel,Deck,Draw,Discard,Exhaust,Confirm,EndTurn,Peek,
    Up,Down,Left,Right,Card1,Card2,Card3,Card4,Card5,Card6,Card7,Card8,Card9,Card10,Count
};
constexpr unsigned InputCommandCount=unsigned(InputCommand::Count);
struct InputBindings {uint16_t keys[2][InputCommandCount]{};};
constexpr bool input_remappable(unsigned bank,unsigned command)noexcept{return bank<2&&command<InputCommandCount&&(bank|| (command!=unsigned(InputCommand::Select)&&command!=unsigned(InputCommand::TopPanel)));}
constexpr uint16_t normalize_input_key(uint16_t key)noexcept{return key>='a'&&key<='z'?uint16_t(key-'a'+'A'):key;}
constexpr bool valid_input_key(uint16_t key)noexcept{return key==8||key==9||key==13||key==27||
    (key>=32&&key<=126&&!(key>='a'&&key<='z'))||(key>=InputLeft&&key<=InputPageDown)||
    (key>=InputF1&&key<=InputF12)||(key>=InputShift&&key<=InputMeta);}
constexpr InputBindings default_input_bindings(bool calculator=false)noexcept{
    InputBindings result{};
    const uint16_t mouse[]={0,27,'M',0,'D','A','S','X','E','E',' ',InputUp,InputDown,InputLeft,InputRight};
    const uint16_t keyboard[]={' ',27,9,'W','Q','A','D','F',13,'E','S',InputUp,InputDown,InputLeft,InputRight};
    for(unsigned i=0;i<15;++i){result.keys[0][i]=mouse[i];result.keys[1][i]=keyboard[i];}
    for(unsigned bank=0;bank<2;++bank)for(unsigned i=0;i<10;++i)result.keys[bank][15+i]=uint16_t(i==9?'0':'1'+i);
    if(calculator){
        // CX has no separate arrow keys: retain its established number-pad
        // navigation. Every adapted default is visible and remappable.
        const uint16_t cards[]={'1','3','7','9','0','Z','X','C','V','B'};
        for(unsigned bank=0;bank<2;++bank){auto&keys=result.keys[bank];
            keys[unsigned(InputCommand::Map)]='M';keys[unsigned(InputCommand::Deck)]='Q';
            keys[unsigned(InputCommand::Discard)]='D';keys[unsigned(InputCommand::Exhaust)]='F';
            keys[unsigned(InputCommand::Peek)]=' ';keys[unsigned(InputCommand::Up)]='8';keys[unsigned(InputCommand::Down)]='2';
            keys[unsigned(InputCommand::Left)]='4';keys[unsigned(InputCommand::Right)]='6';
            for(unsigned i=0;i<10;++i)keys[15+i]=cards[i];
        }
        result.keys[1][unsigned(InputCommand::Select)]='5';
    }
    return result;
}
inline bool valid_input_bindings(const InputBindings&bindings)noexcept{
    for(unsigned bank=0;bank<2;++bank)for(unsigned i=0;i<InputCommandCount;++i){const uint16_t key=bindings.keys[bank][i];
        if(!input_remappable(bank,i)){if(key)return false;continue;}
        if(!valid_input_key(key))return false;
        for(unsigned j=0;j<i;++j)if(key==bindings.keys[bank][j]&&!(bank==0&&i==unsigned(InputCommand::EndTurn)&&j==unsigned(InputCommand::Confirm)))return false;
    }
    return bindings.keys[0][unsigned(InputCommand::Confirm)]==bindings.keys[0][unsigned(InputCommand::EndTurn)];
}
inline bool rebind_input(InputBindings&bindings,unsigned bank,unsigned command,uint16_t key)noexcept{
    key=normalize_input_key(key);if(!input_remappable(bank,command)||!valid_input_key(key))return false;
    auto&keys=bindings.keys[bank];const uint16_t old=keys[command];
    const auto assign=[&](unsigned index,uint16_t value)noexcept{keys[index]=value;
        if(!bank&&(index==unsigned(InputCommand::Confirm)||index==unsigned(InputCommand::EndTurn)))keys[unsigned(InputCommand::Confirm)]=keys[unsigned(InputCommand::EndTurn)]=value;};
    // NInputManager swaps a conflicting command into the old slot. The
    // mouse/keyboard Confirm/EndTurn pair stays linked through either swap.
    for(unsigned i=0;i<InputCommandCount;++i)if(input_remappable(bank,i)&&keys[i]==key){assign(i,old);break;}
    assign(command,key);return true;
}
inline const char* input_command_key(unsigned command)noexcept{
    static const char*const names[]={"select","cancel","viewMap","topPanel","viewDeck","viewDraw","viewDiscard","viewExhaust","confirm","endTurn","peek",
        "up","down","left","right","selectCard1","selectCard2","selectCard3","selectCard4","selectCard5","selectCard6","selectCard7","selectCard8","selectCard9","selectCard10"};
    return command<InputCommandCount?names[command]:"";
}
inline void input_key_label(uint16_t key,char*out,size_t capacity)noexcept{
    if(!capacity)return;out[0]=0;const char*name=nullptr;char text[5]{};
    switch(key){case 0:name="-";break;case 8:name="Backspace";break;case 9:name="Tab";break;case 13:name="Enter";break;case 27:name="Esc";break;case 32:name="Space";break;
        case InputLeft:name="Left";break;case InputRight:name="Right";break;case InputUp:name="Up";break;case InputDown:name="Down";break;
        case InputInsert:name="Insert";break;case InputDelete:name="Delete";break;case InputHome:name="Home";break;case InputEnd:name="End";break;
        case InputPageUp:name="Page Up";break;case InputPageDown:name="Page Down";break;case InputShift:name="Shift";break;case InputControl:name="Ctrl";break;case InputAlt:name="Alt";break;case InputMeta:name="Meta";break;
        default:if(key>=InputF1&&key<=InputF12){const unsigned number=key-InputF1+1;text[0]='F';if(number>=10){text[1]='1';text[2]=char('0'+number%10);}else text[1]=char('0'+number);}
            else if(key>=33&&key<=126)text[0]=char(key);name=text;break;
    }
    unsigned n=0;while(name[n]&&n+1<capacity){out[n]=name[n];++n;}out[n]=0;
}
} // namespace spire
