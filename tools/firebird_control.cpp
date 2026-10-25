// Command frontend for the upstream Firebird core. It drives ordinary keypad
// and USB APIs and captures the emulated LCD; it does not patch the core or OS.
#include "core/emu.h"
#include "core/cpu.h"
#include "core/keypad.h"
#include "core/lcd.h"
#include "core/usblink_queue.h"
#include "keymap.h"
#include <array>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {
uint64_t simulated_ms=0,run_until=0,usb_deadline=0;
bool waiting_usb=false,failed=false;
FILE* serial_output=nullptr;
FILE* event_output=nullptr;
std::istream* commands=&std::cin;
std::ifstream command_file;
struct Release {int key;uint64_t at;};
std::vector<Release> releases;
struct Transfer {std::string operation,path;int progress=-2,last_report=-2;} transfer;
std::string json_quote(const std::string& value){
    std::string out="\"";
    for(unsigned char c:value){
        if(c=='"'||c=='\\'){out+='\\';out+=char(c);}
        else if(c=='\n')out+="\\n";
        else if(c=='\r')out+="\\r";
        else if(c=='\t')out+="\\t";
        else if(c<32){char code[7];std::snprintf(code,sizeof code,"\\u%04x",unsigned(c));out+=code;}
        else out+=char(c);
    }
    return out+'"';
}
void event(const char* operation,const std::string& detail=""){
    const std::string line="{\"operation\":"+json_quote(operation)+",\"simulated_ms\":"+
        std::to_string(simulated_ms)+(detail.empty()?"":","+detail)+"}\n";
    std::fwrite(line.data(),1,line.size(),stdout);std::fflush(stdout);
    if(event_output){std::fwrite(line.data(),1,line.size(),event_output);std::fflush(event_output);}
}
void failure(const std::string& message){failed=true;event("error","\"message\":"+json_quote(message));}
int named_key(const std::string& name){
    struct Named {const char* name;int id;};
    static const Named names[]={
        {"enter",keymap::enter},{"return",keymap::ret},{"esc",keymap::esc},
        {"doc",keymap::doc},{"menu",keymap::menu},{"tab",keymap::tab},
        {"ctrl",keymap::ctrl},{"shift",keymap::shift},{"space",keymap::space},
        {"del",keymap::del},{"on",keymap::on},{"pad",keymap::pad},
        {"var",keymap::var},{"flag",keymap::flag},{"catalog",keymap::cat},{"plus",keymap::plus},{"minus",keymap::minus},
        {"up",100},{"down",101},{"left",102},{"right",103}
    };
    for(const auto& k:names)if(name==k.name)return k.id;
    static const int letters[]={keymap::aa,keymap::ab,keymap::ac,keymap::ad,keymap::ae,keymap::af,
        keymap::ag,keymap::ah,keymap::ai,keymap::aj,keymap::ak,keymap::al,keymap::am,keymap::an,
        keymap::ao,keymap::ap,keymap::aq,keymap::ar,keymap::as,keymap::at,keymap::au,keymap::av,
        keymap::aw,keymap::ax,keymap::ay,keymap::az};
    static const int digits[]={keymap::n0,keymap::n1,keymap::n2,keymap::n3,keymap::n4,
        keymap::n5,keymap::n6,keymap::n7,keymap::n8,keymap::n9};
    if(name.size()==1&&name[0]>='a'&&name[0]<='z')return letters[name[0]-'a'];
    if(name.size()==1&&name[0]>='0'&&name[0]<='9')return digits[name[0]-'0'];
    return -1;
}
void set_key(int key,bool down){
    if(key<100){keypad_set_key(key/keymap::COLS,key%keymap::COLS,down);return;}
    const float x=key==102?0.0f:key==103?1.0f:0.5f;
    const float y=key==100?0.0f:key==101?1.0f:0.5f;
    touchpad_set_state(x,y,down,down);
}
bool screenshot(const std::string& path){
    static std::array<uint16_t,320*240> pixels;
    static std::array<uint8_t,320*240*3> rgb;
    lcd_cx_draw_frame(pixels.data());
    for(size_t i=0;i<pixels.size();++i){
        const unsigned p=pixels[i];
        rgb[i*3]=uint8_t(((p>>11)&31)*255/31);
        rgb[i*3+1]=uint8_t(((p>>5)&63)*255/63);
        rgb[i*3+2]=uint8_t((p&31)*255/31);
    }
    FILE* out=std::fopen(path.c_str(),"wb");if(!out)return false;
    const bool ok=std::fprintf(out,"P6\n320 240\n255\n")>0&&
        std::fwrite(rgb.data(),1,rgb.size(),out)==rgb.size();
    return std::fclose(out)==0&&ok;
}
void progress(int value,void*){
    transfer.progress=value;
    if(value<0||value==100||value/10!=transfer.last_report/10){
        event("usb_progress","\"action\":"+json_quote(transfer.operation)+",\"path\":"+
            json_quote(transfer.path)+",\"progress\":"+std::to_string(value));
        transfer.last_report=value;
    }
    if(value<0)failure("USB "+transfer.operation+" failed for "+transfer.path);
}
void listing(usblink_file* file,bool is_error,void*){
    if(file){event("usb_entry","\"name\":"+json_quote(file->filename)+",\"bytes\":"+
        std::to_string(file->size)+",\"directory\":"+(file->is_dir?"true":"false"));}
    else {transfer.progress=is_error?-1:100;event("usb_listing_end","\"success\":"+std::string(is_error?"false":"true"));
        if(is_error)failure("USB directory listing failed for "+transfer.path);}
}
void begin_transfer(const std::string& action,const std::string& path){
    transfer={action,path,-2,-2};waiting_usb=true;usb_deadline=simulated_ms+600000;
}
bool execute(const std::string& line){
    std::istringstream input(line);std::string operation;input>>operation;
    if(operation.empty()||operation[0]=='#')return false;
    event("command","\"text\":"+json_quote(line));
    if(operation=="run"){
        uint64_t ms=0;if(!(input>>ms)){failure("run MILLISECONDS");return false;}
        run_until=simulated_ms+ms;return ms!=0;
    }
    if(operation=="press"||operation=="key"){
        std::string name;uint64_t hold=0,settle=0;input>>name;const int id=named_key(name);
        if(id<0){failure("unknown key: "+name);return false;}
        if(operation=="key"){
            unsigned state=0;if(!(input>>state)||state>1){failure("key NAME 0|1");return false;}
            set_key(id,state!=0);return false;
        }
        if(!(input>>hold>>settle)||!hold){failure("press NAME HOLD_MS SETTLE_MS");return false;}
        set_key(id,true);releases.push_back({id,simulated_ms+hold});
        run_until=simulated_ms+hold+settle;return true;
    }
    if(operation=="touch"){
        float x=0,y=0;unsigned contact=0,down=0;
        if(!(input>>x>>y>>contact>>down)){failure("touch X Y CONTACT DOWN");return false;}
        touchpad_set_state(x,y,contact!=0,down!=0);return false;
    }
    if(operation=="screen"||operation=="snapshot"){
        std::string path;if(!(input>>std::quoted(path))){failure(operation+" PATH");return false;}
        const bool ok=operation=="screen"?screenshot(path):emu_suspend(path.c_str());
        event(operation.c_str(),"\"path\":"+json_quote(path)+",\"success\":"+(ok?"true":"false"));
        if(!ok)failure(operation+" failed: "+path);return false;
    }
    if(operation=="flush"){
        const bool ok=flash_save_changes();event("flash_saved","\"success\":"+std::string(ok?"true":"false"));
        if(!ok)failure("saving working flash failed");return false;
    }
    if(operation=="status"){
        event("status","\"pc\":"+std::to_string(arm.reg[15])+",\"product\":"+
            std::to_string(product)+",\"usb_connected\":"+(usblink_connected?"true":"false")+
            ",\"usb_queued\":"+std::to_string(usblink_queue_size()));return false;
    }
    if(operation=="dir"||operation=="mkdir"||operation=="rm"||operation=="rmdir"||operation=="put"||operation=="get"){
        std::string first,second;
        if(!(input>>std::quoted(first))||((operation=="put"||operation=="get")&&!(input>>std::quoted(second)))){
            failure(operation+" requires its path argument(s)");return false;
        }
        if(usblink_queue_size()){failure("previous USB action is still pending");return false;}
        begin_transfer(operation,operation=="put"?second:first);
        if(operation=="dir")usblink_queue_dirlist(first,listing,nullptr);
        else if(operation=="mkdir")usblink_queue_new_dir(first,progress,nullptr);
        else if(operation=="rm"||operation=="rmdir")usblink_queue_delete(first,operation=="rmdir",progress,nullptr);
        else if(operation=="put")usblink_queue_put_file(first,second,progress,nullptr);
        else usblink_queue_download(first,second,progress,nullptr);
        return true;
    }
    if(operation=="reset"){cpu_events|=EVENT_RESET;return false;}
    if(operation=="quit"){exiting=true;return true;}
    failure("unknown command: "+operation);return false;
}
}

void gui_do_stuff(bool periodic){
    if(!periodic||exiting)return;
    simulated_ms+=10;
    for(auto it=releases.begin();it!=releases.end();){
        if(it->at<=simulated_ms){set_key(it->key,false);it=releases.erase(it);}else ++it;
    }
    if(waiting_usb){
        if(usblink_queue_size()&&simulated_ms<usb_deadline)return;
        if(usblink_queue_size())failure("USB action exceeded 600 simulated seconds");
        waiting_usb=false;
    }
    if(simulated_ms<run_until)return;
    event("ready");std::string line;
    while(!exiting){
        if(!std::getline(*commands,line)){exiting=true;break;}
        if(execute(line))break;
        event("ready");
    }
}
void gui_debug_vprintf(const char* fmt,va_list args){std::vfprintf(stderr,fmt,args);}
void gui_debug_printf(const char* fmt,...){va_list args;va_start(args,fmt);gui_debug_vprintf(fmt,args);va_end(args);}
void gui_status_printf(const char* fmt,...){va_list args;va_start(args,fmt);gui_debug_vprintf(fmt,args);va_end(args);std::fputc('\n',stderr);}
void gui_perror(const char* message){std::fprintf(stderr,"%s: %s\n",message,std::strerror(errno));}
void gui_debugger_entered_or_left(bool entered){if(entered){failure("emulator entered debugger");exiting=true;}}
void gui_debugger_request_input(debug_input_cb callback){if(callback){failure("emulator requested debugger input");exiting=true;}}
void gui_putchar(char c){if(serial_output){std::fputc(c,serial_output);std::fflush(serial_output);}}
int gui_getchar(){return -1;}
void gui_set_busy(bool){}
void gui_show_speed(double){}
void gui_usblink_changed(bool state){event("usb_connection","\"connected\":"+std::string(state?"true":"false"));}
void throttle_timer_off(){}
void throttle_timer_on(){}
void throttle_timer_wait(unsigned int){}

int main(int argc,char** argv){
    std::string snapshot,script,events,serial;
    for(int i=1;i<argc;++i){
        const std::string option=argv[i];
        if(option=="--help"){
            std::puts("firebird-control --boot1 IMAGE --flash WORKING_COPY [--snapshot SNAPSHOT] [--commands FILE] [--events JSONL] [--serial LOG]\n"
                "Commands: run MS | press KEY HOLD_MS SETTLE_MS | key KEY 0|1 | touch X Y CONTACT DOWN\n"
                "          screen PPM | snapshot FILE | flush | status | dir PATH | mkdir PATH\n"
                "          put LOCAL REMOTE | get REMOTE LOCAL | rm PATH | rmdir PATH | reset | quit\n"
                "Key names: enter, return, esc, doc, menu, tab, ctrl, shift, space, del, on, pad, var, plus, minus, arrows, a-z, 0-9.\n"
                "Simulation pauses at ready until a command is supplied. Paths can be quoted.");return 0;
        }
        if(i+1>=argc){std::fprintf(stderr,"Missing argument for %s\n",option.c_str());return 2;}
        const std::string value=argv[++i];
        if(option=="--boot1")path_boot1=value;
        else if(option=="--flash")path_flash=value;
        else if(option=="--snapshot")snapshot=value;
        else if(option=="--commands")script=value;
        else if(option=="--events")events=value;
        else if(option=="--serial")serial=value;
        else {std::fprintf(stderr,"Unknown option %s\n",option.c_str());return 2;}
    }
    if(path_boot1.empty()||path_flash.empty()){std::fprintf(stderr,"Boot1 and a working flash copy are required\n");return 2;}
    if(!script.empty()){command_file.open(script);if(!command_file)return 2;commands=&command_file;}
    if(!events.empty()){event_output=std::fopen(events.c_str(),"wb");if(!event_output)return 2;}
    if(!serial.empty()){serial_output=std::fopen(serial.c_str(),"wb");if(!serial_output)return 2;}
    print_on_warn=true;debug_on_warn=false;turbo_mode=true;
    if(!emu_start(0,0,snapshot.empty()?nullptr:snapshot.c_str())){failure("emulator start failed");return 1;}
    event("started","\"flash\":"+json_quote(path_flash)+",\"resumed\":"+(snapshot.empty()?"false":"true"));
    emu_loop(false);emu_cleanup();event("stopped","\"success\":"+std::string(failed?"false":"true"));
    if(serial_output)std::fclose(serial_output);if(event_output)std::fclose(event_output);
    return failed?1:0;
}
