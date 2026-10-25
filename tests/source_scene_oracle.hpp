#pragma once
#include "app.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
namespace spire_test {
using namespace spire;
inline void source_scene_check(bool value,int line){if(!value){std::fprintf(stderr,"source scene oracle line %d\n",line);std::exit(1);}}
#define SOURCE_SCENE_CHECK(x) source_scene_check(bool(x),__LINE__)
// Independent scalar paint/word-wrap oracle. Exact source-font keys are
// required; no production text layout, fit, transform or paint helper is used.
struct SceneLetter {unsigned cp,width,height,advance,color;std::vector<uint8_t>alpha;};
static std::vector<SceneLetter> scene_letters(const Content&content,const std::vector<uint8_t>&pack,const std::string&text,unsigned size,unsigned face){
    const auto u32=[&](unsigned at){return unsigned(pack[at])|unsigned(pack[at+1])<<8|unsigned(pack[at+2])<<16|unsigned(pack[at+3])<<24;};
    const unsigned glyphs=u32(20),base=40+u32(16)*16;std::vector<SceneLetter>letters;
    RichReader reader(text.c_str());RichChar ch{};
    while(reader.next(ch)){
        if(ch.codepoint=='\n'){letters.push_back({ch.codepoint,0,0,0,ch.style.color,{}});continue;}
        const unsigned selected=face|(ch.style.bold?5u:0u),band=size|((selected&3)<<6)|((selected&4)<<3);
        const uint64_t key=uint64_t(band)<<24|ch.codepoint;unsigned low=0,high=glyphs;
        while(low<high){const unsigned mid=(low+high)/2,at=base+mid*16;
            const uint64_t value=uint64_t(pack[at+8]|unsigned(pack[at+9])<<8)<<24|u32(at);if(value<key)low=mid+1;else high=mid;}
        SOURCE_SCENE_CHECK(low<glyphs);const unsigned at=base+low*16;
        if(u32(at)!=ch.codepoint||(pack[at+8]|unsigned(pack[at+9])<<8)!=band)std::fprintf(stderr,"missing profile glyph cp=%u size=%u face=%u\n",ch.codepoint,size,selected);
        SOURCE_SCENE_CHECK(u32(at)==ch.codepoint&&(pack[at+8]|unsigned(pack[at+9])<<8)==band);
        const auto glyph=content.glyph(ch.codepoint,int(size),selected);SOURCE_SCENE_CHECK(glyph.pixels&&glyph.advance);
        SceneLetter letter{ch.codepoint,glyph.w,glyph.h,glyph.advance,ch.style.color,std::vector<uint8_t>(unsigned(glyph.w)*glyph.h)};
        if(glyph.encoded_size){unsigned pos=0;for(unsigned i=0;i<glyph.encoded_size;i+=2){SOURCE_SCENE_CHECK(i+1<glyph.encoded_size&&pos+glyph.pixels[i]<=letter.alpha.size());
            for(unsigned n=0;n<glyph.pixels[i];++n)letter.alpha[pos++]=glyph.pixels[i+1];}SOURCE_SCENE_CHECK(pos==letter.alpha.size());}
        else std::memcpy(letter.alpha.data(),glyph.pixels,letter.alpha.size());letters.push_back(letter);
    }return letters;
}
static unsigned scene_width(const std::vector<SceneLetter>&letters){unsigned n=0;for(const auto&letter:letters)n+=letter.advance;return n;}
static uint16_t scene_rgb(unsigned r,unsigned g,unsigned b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
static void scene_pixel(std::vector<uint16_t>&frame,int x,int y,uint16_t color,unsigned alpha){
    if(x<0||x>=320||y<0||y>=240)return;const uint16_t old=frame[unsigned(y*320+x)];
    const auto mix=[&](unsigned a,unsigned b){return (a*(255-alpha)+b*alpha+127)/255;};
    frame[unsigned(y*320+x)]=uint16_t(mix(old>>11,color>>11)<<11|mix((old>>5)&63,(color>>5)&63)<<5|mix(old&31,color&31));
}
static void scene_label(std::vector<uint16_t>&frame,const std::vector<SceneLetter>&letters,int x,int y,int width,int height,
                        unsigned size,uint16_t color,unsigned shadow,int outline=0,uint16_t outline_color=0,int shadow_outline=0,
                        bool center=true,bool wrap=false,unsigned outline_alpha=255,int pivot_x=0,int pivot_y=0,int scale=4096,bool clip=false,int shadow_x=1,int shadow_y=0,unsigned overall_alpha=255){
    std::vector<std::vector<SceneLetter>>lines(1);unsigned line_width=0;
    for(unsigned i=0;i<letters.size();++i){const auto&letter=letters[i];
        if(letter.cp=='\n'){lines.emplace_back();line_width=0;continue;}
        const bool space=letter.cp==' '||letter.cp=='\t';if(wrap&&space&&(!line_width||line_width+letter.width>unsigned(width)))continue;
        if(wrap&&line_width){unsigned word_width=0;if((i==0||letters[i-1].cp==' '||letters[i-1].cp=='\n')&&letter.cp>32&&letter.cp<0x300){
                unsigned cursor=0;for(unsigned n=i;n<letters.size()&&letters[n].cp>32&&letters[n].cp<0x300;++n){word_width=cursor+std::max(letters[n].width,letters[n].advance);cursor+=letters[n].advance;}}
            if(line_width+std::max(letter.width,letter.advance)>unsigned(width)||(word_width&&word_width<=unsigned(width)&&line_width+word_width>unsigned(width))){lines.emplace_back();line_width=0;}}
        if(wrap&&space&&!line_width)continue;lines.back().push_back(letter);line_width+=letter.advance;
    }
    const int top=y+(height-(int(lines.size())*int(size+1)+1))/2;
    const auto project=[&](int value,int pivot){const int delta=(value-pivot)*scale;return pivot+(delta+(delta<0?-2048:2048))/4096;};
    const auto pass=[&](int dx,int dy,unsigned opacity,uint16_t ink,bool decoration){
        for(unsigned row=0;row<lines.size();++row){int at=x+(center?(width-int(scene_width(lines[row])))/2:0);
            for(const auto&letter:lines[row]){const uint16_t colors[]={color,scene_rgb(239,200,81),scene_rgb(255,85,85),scene_rgb(135,206,235),scene_rgb(127,255,0),scene_rgb(238,130,238),scene_rgb(255,165,24),scene_rgb(0,255,255),scene_rgb(255,255,255),scene_rgb(128,128,128)};const uint16_t rich=colors[letter.color];
                for(unsigned yy=0;yy<letter.height;++yy)for(unsigned xx=0;xx<letter.width;++xx){const unsigned layer=(opacity*overall_alpha+127)/255,alpha=(letter.alpha[yy*letter.width+xx]*layer+127)/255;
                    const int px=at+int(xx)+dx,py=top+int(row*(size+1))+int(yy)+dy;
                    for(int sy=project(py,pivot_y);sy<project(py+1,pivot_y);++sy)for(int sx=project(px,pivot_x);sx<project(px+1,pivot_x);++sx){
                        if(clip&&(sx<project(x,pivot_x)||sx>=project(x+width,pivot_x)||sy<project(y,pivot_y)||sy>=project(y+height,pivot_y)))continue;
                        scene_pixel(frame,sx,sy,decoration?ink:rich,alpha);}}
                at+=letter.advance;}}
    };
    if(shadow)for(int yy=-shadow_outline;yy<=shadow_outline;++yy)for(int xx=-shadow_outline;xx<=shadow_outline;++xx)
        if(xx*xx+yy*yy<=shadow_outline*shadow_outline)pass(xx+shadow_x,yy+shadow_y,shadow,0,true);
    for(int yy=-outline;yy<=outline;++yy)for(int xx=-outline;xx<=outline;++xx)
        if((xx||yy)&&xx*xx+yy*yy<=outline*outline)pass(xx,yy,outline_alpha,outline_color,true);
    pass(0,0,255,color,false);
}
#undef SOURCE_SCENE_CHECK
}
