#include "app.hpp"
#include "touchpad.hpp"
extern "C" {
void* nd_open(const char*,const char*);
unsigned nd_read(void*,unsigned,unsigned,void*);
unsigned nd_write(const void*,unsigned,unsigned,void*);
int nd_close(void*);
int nd_seek(void*,long,int);
long nd_tell(void*);
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
static spire::SaveData scratch;
static char pack_path[512],save_path[512],temp_path[512],backup_path[512];
static bool read_resource(void* file,uint32_t offset,void* output,uint32_t count){
    return nd_seek(file,(long)offset,0)==0&&nd_read(output,1,count,file)==count;
}
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
static bool clear_save(){
    // A deliberate abandon also removes the preceding atomic-save backup.
    // Otherwise a later recovery could bring back an abandoned expedition.
    const char*paths[]={backup_path,temp_path,save_path};for(unsigned i=0;i<3;i++){const char*path=paths[i];
        void*f=nd_open(path,"rb");if(!f)continue;if(nd_close(f)!=0||nd_unlink(path)!=0)return false;
    }return true;
}
static bool pressed(unsigned offset,unsigned mask){return (*(volatile uint16_t*)(0x900E0000u+offset)&mask)!=0;}
struct Clock {
    uint32_t load, control, last;
    // SP804 timer 2 is dedicated to this foreground runtime. Do not reset
    // the OS's 0x900C timer: restoring its registers cannot restore elapsed time.
    void init(){auto*t=(volatile uint32_t*)0x900D0000u;
        load=t[0];control=t[2];t[2]=0;t[0]=0xffffffffu;t[2]=0x82;last=t[1];}
    unsigned frame(){uint32_t now=*(volatile uint32_t*)0x900D0004u,dt=last-now;
        if(dt<1092)return 0;last=now;return dt>32768?1000:(dt*1000)>>15;}
    void restore(){auto*t=(volatile uint32_t*)0x900D0000u;t[2]=0;t[0]=load;t[2]=control;}
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
    spire::TouchSample read(){
        uint8_t b[12]={};
        if(!w||!h||!nd_touch_read(0,10,b))return {};
        return {(uint16_t)(((unsigned)b[2]<<8)|b[3]),
                (uint16_t)(((unsigned)b[4]<<8)|b[5]),(uint16_t)w,(uint16_t)h,
                b[0]!=0,b[10]!=0,true};
    }
};
extern "C" int main(int argc,char**argv){
    if(nd_revision()<2022||nd_hwtype()==0)return 2;
    const char*launcher=argc>0&&argv&&argv[0]?argv[0]:"";
    if(!sibling(pack_path,launcher,"nSpire2.nsp.tns")||!sibling(save_path,launcher,"nSpire2.sav.tns")||!sibling(temp_path,launcher,"nSpire2.tmp.tns")||!sibling(backup_path,launcher,"nSpire2.bak.tns"))return 3;
    void*f=nd_open(pack_path,"rb");if(!f)return 4;
    if(nd_seek(f,0,2)!=0){nd_close(f);return 5;}long length=nd_tell(f);
    if(length<=0||(unsigned long)length>spire::MaxContentFile||!app.init({f,(uint32_t)length,read_resource,nullptr},{save_game,load_game,clear_save})){nd_close(f);return 5;}
    if(!nd_lcd_init(0)){nd_close(f);return 6;}
    auto blit=nd_blitter(0);if(!blit){nd_lcd_init(-1);nd_close(f);return 7;}
    while(pressed(0x10,2)){} // Release ENTER used to open the document.
    Touchpad pad{};pad.init();spire::TouchpadCursor pad_cursor;
    Clock clock{};clock.init();uint32_t input_clock=0;
    const spire::Key keys[]={spire::Key::Left,spire::Key::Right,spire::Key::Up,spire::Key::Down,spire::Key::Confirm,spire::Key::Back,spire::Key::EndTurn,spire::Key::Inspect,spire::Key::Save,spire::Key::Load,spire::Key::Log,spire::Key::Map,spire::Key::Potions,spire::Key::Relics,spire::Key::Detail,spire::Key::ReferenceMap,spire::Key::Collection,spire::Key::Statuses};
    uint32_t previous=0xffff;unsigned repeat[4]={};bool input_ready=false;unsigned quiet_ms=0;app.render();blit(app.pixels);
    while(!app.quit){
        unsigned elapsed=clock.frame();if(!elapsed)continue;app.advance(elapsed);
        input_clock+=elapsed;
        auto sample=pad.read();auto pointer=pad_cursor.sample(sample,input_clock);
        if(input_ready&&pointer.cancelled)app.pointer_cancel();
        if(input_ready&&(pointer.moved||pointer.down||pointer.up||pointer.tap))app.pointer_move(pointer.x,pointer.y);
        if(input_ready&&(pointer.down||pointer.tap))app.pointer_button(true);
        if(input_ready&&(pointer.up||pointer.tap))app.pointer_button(false);
        uint32_t p=0;
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
        if(pressed(0x14,1))p|=1u<<13; // R: relic collection
        if(pressed(0x16,0x10))p|=1u<<14; // I: inspect selected card
        if(pressed(0x16,0x40))p|=1u<<15; // G: reference map preview
        if(pressed(0x18,0x10))p|=1u<<16; // C: compendium
        if(pressed(0x12,4))p|=1u<<17; // V: powers and statuses
        if(!input_ready){
            // The document launcher may leave ENTER or the pad held. Require a
            // stable release so opening the program cannot also start a run.
            quiet_ms=(p==0&&!sample.contact&&!sample.pressed)?quiet_ms+elapsed:0;
            if(quiet_ms>=100)input_ready=true;
            previous=p;pad_cursor.reset();app.render();blit(app.pixels);continue;
        }
        uint32_t edge=p&~previous;
        for(unsigned i=0;i<4;i++){if(p&(1u<<i)){repeat[i]++;if(repeat[i]>10&&repeat[i]%3==0)edge|=1u<<i;}else repeat[i]=0;}
        for(unsigned i=0;i<sizeof(keys)/sizeof(keys[0]);i++)if(edge&(1u<<i))app.input(keys[i]);previous=p;
        app.render();blit(app.pixels);
    }
    clock.restore();nd_lcd_init(-1);nd_close(f);return 0;
}
