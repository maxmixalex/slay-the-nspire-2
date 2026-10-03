#include "app.hpp"
extern "C" {
__attribute__((import_module("env"),import_name("save"))) int host_save(const uint8_t*,unsigned);
__attribute__((import_module("env"),import_name("load"))) int host_load(uint8_t*,unsigned);
}
static spire::App app;
static uint8_t content[spire::MaxPack];
static spire::SaveData buffer;
static bool save_game(const spire::State&s){return spire::save(s,buffer)&&host_save(buffer.bytes,(unsigned)buffer.size)!=0;}
static bool load_game(spire::State&s){int n=host_load(buffer.bytes,sizeof buffer.bytes);return n>0&&spire::load(s,buffer.bytes,(unsigned)n);}
extern "C" {
unsigned content_buffer(){return (unsigned)(size_t)content;}
int start(unsigned n){return app.init(content,n,{save_game,load_game})?1:0;}
unsigned framebuffer(){return (unsigned)(size_t)app.pixels;}
void input(int key){if(key>=0&&key<=(int)spire::Key::Potions)app.input((spire::Key)key);}
void frame(){app.tick();app.render();}
int closed(){return app.quit?1:0;}
}
