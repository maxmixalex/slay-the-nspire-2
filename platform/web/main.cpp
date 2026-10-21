#include "app.hpp"
#include "runtime_scene.hpp"
extern "C" {
__attribute__((import_module("env"),import_name("save"))) int host_save(const uint8_t*,unsigned);
__attribute__((import_module("env"),import_name("load"))) int host_load(uint8_t*,unsigned);
__attribute__((import_module("env"),import_name("resource_read"))) int host_resource_read(unsigned,uint8_t*,unsigned);
}
static spire::App app;
static spire::SaveData buffer;
static bool read_resource(void*,uint32_t offset,void* output,uint32_t count){return host_resource_read(offset,(uint8_t*)output,count)!=0;}
static bool save_game(const spire::State&s){return spire::save(s,buffer)&&host_save(buffer.bytes,(unsigned)buffer.size)!=0;}
static bool load_game(spire::State&s){int n=host_load(buffer.bytes,sizeof buffer.bytes);return n>0&&spire::load(s,buffer.bytes,(unsigned)n);}
// The paired browser host treats an explicit empty save as deletion. Normal
// saves always contain the validated NSP save header and are nonempty.
static bool clear_save(){return host_save(nullptr,0)!=0;}
extern "C" {
int start(unsigned n){return app.init({nullptr,n,read_resource,nullptr},{save_game,load_game,clear_save})?1:0;}
unsigned framebuffer(){return (unsigned)(size_t)app.pixels;}
void input(int key){if(key>=0&&key<=(int)spire::Key::Statuses)app.input((spire::Key)key);}
void frame(){app.tick();app.render();}
void advance(unsigned elapsed_ms){app.advance(elapsed_ms);}
void draw_frame(){app.render();}
void pointer_move(int x,int y){app.pointer_move(x,y);}
void pointer_button(int down){app.pointer_button(down!=0);}
void pointer_cancel(){app.pointer_cancel();app.pointer.visible=false;}
void pointer_scroll(int rows){app.pointer_scroll(rows);}
int text_entry_active(){return app.map_preview.open&&app.map_preview.editing;}
void text_input(int ch){app.reference_text(ch);}
int closed(){return app.quit?1:0;}
// Read-only scene classification for opt-in host timing; no gameplay mutation.
int performance_scene(){
    return int(spire::runtime_scene(app));
}
}
