#include "app.hpp"
extern "C" {
void* nd_open(const char*,const char*);
unsigned nd_read(void*,unsigned,unsigned,void*);
unsigned nd_write(const void*,unsigned,unsigned,void*);
int nd_close(void*);
int nd_rename(const char*,const char*);
int nd_unlink(const char*);
unsigned nd_hwtype();
unsigned nd_revision();
int nd_lcd_init(int);
void (*nd_blitter(int))(void*);
int nd_touch_read(unsigned,unsigned,void*);
int nd_touch_write(unsigned,unsigned,const void*);
}
static spire::App app;
static uint8_t content[spire::MaxPack];
static spire::SaveData scratch;
static char pack_path[512],save_path[512],temp_path[512],backup_path[512];
static bool sibling(char*out,const char*launcher,const char*filename){
    int slash=-1,n=0;while(launcher&&launcher[n]){if(n>=420)return false;if(launcher[n]=='/')slash=n;n++;}
    int at=0;while(at<=slash){out[at]=launcher[at];at++;}while(*filename&&at<510)out[at++]=*filename++;out[at]=0;return !*filename;
}
static bool save_game(const spire::State&s){
    if(!spire::save(s,scratch))return false;
    void*f=nd_open(temp_path,"wb");if(!f)return false;
    bool good=nd_write(scratch.bytes,1,(unsigned)scratch.size,f)==scratch.size;
    good=(nd_close(f)==0)&&good;if(!good){nd_unlink(temp_path);return false;}
    bool old=false;f=nd_open(save_path,"rb");if(f){old=true;nd_close(f);}
    if(old){nd_unlink(backup_path);if(nd_rename(save_path,backup_path)!=0){nd_unlink(temp_path);return false;}}
    if(nd_rename(temp_path,save_path)!=0){if(old)nd_rename(backup_path,save_path);return false;}
    return true;
}
static bool load_game(spire::State&s){
    void*f=nd_open(save_path,"rb");if(!f)return false;
    unsigned n=nd_read(scratch.bytes,1,sizeof scratch.bytes,f);uint8_t extra=0;bool end=nd_read(&extra,1,1,f)==0;nd_close(f);
    return end&&spire::load(s,scratch.bytes,n);
}
static bool pressed(unsigned offset,unsigned mask){return (*(volatile uint16_t*)(0x900E0000u+offset)&mask)!=0;}
struct Clock {
    uint32_t source,control,value,last;
    // Clock register setup follows nZATO's modern Ndless backend (GPL-3.0).
    void init(){auto*v=(volatile uint32_t*)0x900C0004u;auto*c=(volatile uint32_t*)0x900C0008u;auto*s=(volatile uint32_t*)0x900C0080u;
        value=*v;control=*c;source=*s;*c=0;*s=10;*c=0x82;last=*v;}
    bool frame(){uint32_t now=*(volatile uint32_t*)0x900C0004u;if(last-now<1092)return false;last=now;return true;}
    void restore(){auto*v=(volatile uint32_t*)0x900C0004u;auto*c=(volatile uint32_t*)0x900C0008u;auto*s=(volatile uint32_t*)0x900C0080u;*c=0;*s=source;*v=value;*c=control;}
};
struct Touchpad {
    unsigned w,h;
    void init(){
        uint32_t cpsr;asm volatile("mrs %0,cpsr":"=r"(cpsr));asm volatile("msr cpsr_c,%0"::"r"(cpsr|0xC0):"memory");
        uint8_t page=16,b[4]={};bool ok=nd_touch_write(255,255,&page)!=0;
        if(ok)ok=nd_touch_read(4,7,b)!=0;page=4;bool restored=nd_touch_write(255,255,&page)!=0;
        asm volatile("msr cpsr_c,%0"::"r"(cpsr):"memory");
        w=ok&&restored?((unsigned)b[0]<<8)|b[1]:0;h=ok&&restored?((unsigned)b[2]<<8)|b[3]:0;
    }
    unsigned keys(){
        if(!w||!h)return 0;uint8_t b[12]={};if(!nd_touch_read(0,10,b)||!b[10])return 0;
        unsigned x=((unsigned)b[2]<<8)|b[3],y=((unsigned)b[4]<<8)|b[5];
        if(y*3>=h*2)return 1u<<2;if(y*3<=h)return 1u<<3;
        if(x*3<=w)return 1u<<0;if(x*3>=w*2)return 1u<<1;return 1u<<4;
    }
};
extern "C" int main(int argc,char**argv){
    if(nd_revision()<2022||nd_hwtype()==0)return 2;
    const char*launcher=argc>0&&argv&&argv[0]?argv[0]:"";
    if(!sibling(pack_path,launcher,"nSpire2.nsp.tns")||!sibling(save_path,launcher,"nSpire2.sav.tns")||!sibling(temp_path,launcher,"nSpire2.tmp.tns")||!sibling(backup_path,launcher,"nSpire2.bak.tns"))return 3;
    void*f=nd_open(pack_path,"rb");if(!f)return 4;
    unsigned n=nd_read(content,1,sizeof content,f);uint8_t extra=0;bool end=nd_read(&extra,1,1,f)==0;nd_close(f);
    if(!end||!app.init(content,n,{save_game,load_game}))return 5;
    if(!nd_lcd_init(0))return 6;
    auto blit=nd_blitter(0);if(!blit){nd_lcd_init(-1);return 7;}
    while(pressed(0x10,2)){} // Release ENTER used to open the document.
    Touchpad pad{};pad.init();Clock clock{};clock.init();
    const spire::Key keys[]={spire::Key::Left,spire::Key::Right,spire::Key::Up,spire::Key::Down,spire::Key::Confirm,spire::Key::Back,spire::Key::EndTurn,spire::Key::Inspect,spire::Key::Save,spire::Key::Load,spire::Key::Log,spire::Key::Map,spire::Key::Potions};
    uint32_t previous=0;unsigned repeat[4]={};app.render();blit(app.pixels);
    while(!app.quit){
        if(!clock.frame())continue;
        uint32_t p=pad.keys();
        if(pressed(0x14,0x80))p|=1u<<0; // 4
        if(pressed(0x14,8))p|=1u<<1; // 6
        if(pressed(0x1C,0x40))p|=1u<<2; // 8
        if(pressed(0x1C,0x10))p|=1u<<3; // 2
        if(pressed(0x10,2)||pressed(0x1A,0x40))p|=1u<<4; // ENTER / 5
        if(pressed(0x1C,0x80))p|=1u<<5; // ESC
        if(pressed(0x10,0x10))p|=1u<<6; // SPACE
        if(pressed(0x1C,0x200))p|=1u<<7; // TAB
        if(pressed(0x12,0x40))p|=1u<<8; // S
        if(pressed(0x14,0x10))p|=1u<<9; // O
        if(pressed(0x16,1))p|=1u<<10; // L
        if(pressed(0x14,0x40))p|=1u<<11; // M
        if(pressed(0x14,4))p|=1u<<12; // P
        uint32_t edge=p&~previous;
        for(unsigned i=0;i<4;i++){if(p&(1u<<i)){repeat[i]++;if(repeat[i]>10&&repeat[i]%3==0)edge|=1u<<i;}else repeat[i]=0;}
        for(unsigned i=0;i<13;i++)if(edge&(1u<<i))app.input(keys[i]);previous=p;
        bool dirty=edge||app.notice_ticks||app.feedback||(app.animation%2==0);app.tick();
        if(dirty){app.render();blit(app.pixels);}
    }
    clock.restore();nd_lcd_init(-1);return 0;
}
