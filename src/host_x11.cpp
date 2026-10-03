#include "app.hpp"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>

static spire::App app;
static uint8_t pack[spire::MaxPack];
static std::string save_path;
static bool save_game(const spire::State&s){
    static spire::SaveData b{};if(!spire::save(s,b))return false;
    std::string temp=save_path+".tmp";FILE*f=fopen(temp.c_str(),"wb");if(!f)return false;
    bool ok=fwrite(b.bytes,1,b.size,f)==b.size;ok=(fclose(f)==0)&&ok;
    if(ok)ok=rename(temp.c_str(),save_path.c_str())==0;if(!ok)remove(temp.c_str());return ok;
}
static bool load_game(spire::State&s){
    FILE*f=fopen(save_path.c_str(),"rb");if(!f)return false;
    static uint8_t b[16385];size_t n=fread(b,1,sizeof b,f);bool ok=!ferror(f);fclose(f);return ok&&spire::load(s,b,n);
}
static void capture(const char*path){
    app.render();FILE*f=fopen(path,"wb");if(!f){perror(path);exit(3);}fprintf(f,"P6\n320 240\n255\n");
    for(auto p:app.pixels){uint8_t c[3]={(uint8_t)(((p>>11)*255+15)/31),(uint8_t)((((p>>5)&63)*255+31)/63),(uint8_t)(((p&31)*255+15)/31)};fwrite(c,1,3,f);}fclose(f);
}
static bool key(KeySym sym,spire::Key&k){
    using spire::Key;switch(sym){case XK_Left:case XK_4:k=Key::Left;break;case XK_Right:case XK_6:k=Key::Right;break;
    case XK_Up:case XK_8:k=Key::Up;break;case XK_Down:case XK_2:k=Key::Down;break;
    case XK_Return:case XK_KP_Enter:case XK_5:k=Key::Confirm;break;case XK_space:k=Key::EndTurn;break;
    case XK_Escape:k=Key::Back;break;case XK_Tab:k=Key::Inspect;break;case XK_l:case XK_L:k=Key::Log;break;
    case XK_m:case XK_M:k=Key::Map;break;case XK_p:case XK_P:k=Key::Potions;break;
    case XK_s:case XK_S:k=Key::Save;break;case XK_o:case XK_O:k=Key::Load;break;default:return false;}return true;
}
int main(int argc,char**argv){
    std::string dir=argv[0],actions,packpath;auto slash=dir.find_last_of('/');dir=slash==std::string::npos?".":dir.substr(0,slash);
    packpath=dir+"/nSpire2.nsp.tns";save_path=dir+"/nSpire2.sav";bool headless=false;const char*shot=nullptr;
    for(int i=1;i<argc;i++){
        std::string a=argv[i];if(a=="--headless")headless=true;
        else if((a=="--pack"||a=="--save"||a=="--capture"||a=="--actions")&&i+1<argc){const char*v=argv[++i];if(a=="--pack")packpath=v;else if(a=="--save")save_path=v;else if(a=="--capture")shot=v;else actions=v;}
        else {fprintf(stderr,"Usage: %s [--pack FILE] [--save FILE] [--headless] [--actions enter,right,…] [--capture FILE.ppm]\n",argv[0]);return 2;}}
    FILE*f=fopen(packpath.c_str(),"rb");if(!f){perror(packpath.c_str());return 2;}size_t n=fread(pack,1,sizeof pack,f);bool eof=fgetc(f)==EOF;fclose(f);
    if(!eof||!app.init(pack,n,{save_game,load_game})){fprintf(stderr,"Invalid or incompatible content pack\n");return 2;}
    if(!actions.empty()){
        size_t start=0;while(start<actions.size()){
            size_t end=actions.find(',',start);if(end==std::string::npos)end=actions.size();std::string a=actions.substr(start,end-start);
            using spire::Key;Key k=Key::Confirm;
            if(a=="left")k=Key::Left;else if(a=="right")k=Key::Right;else if(a=="up")k=Key::Up;else if(a=="down")k=Key::Down;else if(a=="enter")k=Key::Confirm;
            else if(a=="space")k=Key::EndTurn;else if(a=="esc")k=Key::Back;else if(a=="tab")k=Key::Inspect;else if(a=="save")k=Key::Save;else if(a=="load")k=Key::Load;else if(a=="log")k=Key::Log;else if(a=="map")k=Key::Map;else if(a=="potions")k=Key::Potions;
            else{fprintf(stderr,"Unknown action: %s\n",a.c_str());return 2;}
            app.input(k);for(int tick=0;tick<100;tick++)app.tick();start=end+1;
        }
    }
    if(shot)capture(shot);
    if(headless){auto&s=app.game;printf("{\"title\":%s,\"phase\":%d,\"hp\":%d,\"energy\":%d,\"block\":%d,\"pain\":%d,\"embrace\":%d,\"hand\":%u,\"discard\":%u,\"exhaust\":%u,\"pending\":%u,\"valid\":%s}\n",app.title?"true":"false",(int)s.phase,s.hp,s.energy,s.block,s.pain,s.embrace,s.hand.count,s.discard.count,s.exhaust.count,s.effect_count,app.title||spire::validate(s)?"true":"false");return 0;}
    Display*d=XOpenDisplay(nullptr);if(!d){fprintf(stderr,"No X display. Use --headless for batch execution.\n");return 2;}
    Bool detectable;XkbSetDetectableAutoRepeat(d,True,&detectable);
    int screen=DefaultScreen(d);Window win=XCreateSimpleWindow(d,RootWindow(d,screen),0,0,960,720,0,0,0);
    XStoreName(d,win,"nSpire II | Native expedition prototype 0.2");XSelectInput(d,win,ExposureMask|KeyPressMask|KeyReleaseMask|StructureNotifyMask|FocusChangeMask);
    Atom close=XInternAtom(d,"WM_DELETE_WINDOW",False);XSetWMProtocols(d,win,&close,1);XMapWindow(d,win);
    GC gc=XCreateGC(d,win,0,nullptr);int width=960,height=720;bool held[256]={};XImage*image=nullptr;
    while(!app.quit){
        while(XPending(d)){
            XEvent e;XNextEvent(d,&e);
            if(e.type==ClientMessage&&(Atom)e.xclient.data.l[0]==close)app.quit=true;
            else if(e.type==ConfigureNotify){width=std::max(320,e.xconfigure.width);height=std::max(240,e.xconfigure.height);if(image){XDestroyImage(image);image=nullptr;}}
            else if(e.type==FocusOut)memset(held,0,sizeof held);
            else if(e.type==KeyRelease)held[e.xkey.keycode&255]=false;
            else if(e.type==KeyPress&&!held[e.xkey.keycode&255]){held[e.xkey.keycode&255]=true;spire::Key k;if(key(XLookupKeysym(&e.xkey,0),k))app.input(k);}
        }
        app.tick();app.render();int scale=std::max(1,std::min(width/320,height/240)),w=320*scale,h=240*scale;
        if(!image){image=XCreateImage(d,DefaultVisual(d,screen),DefaultDepth(d,screen),ZPixmap,0,nullptr,w,h,32,0);if(!image)break;image->data=(char*)calloc(image->bytes_per_line,h);if(!image->data){XDestroyImage(image);image=nullptr;break;}}
        for(int y=0;y<h;y++){auto*row=(uint32_t*)(image->data+y*image->bytes_per_line);for(int x=0;x<w;x++){uint16_t p=app.pixels[(y/scale)*320+x/scale];row[x]=((p>>11)*255/31)<<16|(((p>>5)&63)*255/63)<<8|(p&31)*255/31;}}
        XPutImage(d,win,gc,image,0,0,(width-w)/2,(height-h)/2,w,h);XFlush(d);std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    if(image)XDestroyImage(image);XFreeGC(d,gc);XDestroyWindow(d,win);XCloseDisplay(d);return 0;
}
