#pragma once
#include "resource_source.hpp"

namespace spire {
// The resource pack is untrusted until opening completes. This tiny original
// boot display therefore uses no pack image, glyph, allocation or game state.
// The bar measures work within the displayed stage; its animation is only an
// activity indicator and never advances the reported completion count.
inline void loading_screen(uint16_t* pixels,ResourceStage stage,uint32_t completed,
                           uint32_t total,uint32_t animation,const char* custom_label=nullptr,
                           bool hide_phase=false){
    if(!pixels)return;
    struct Paint {
        uint16_t* p;
        void box(int x,int y,int w,int h,uint16_t color){
            for(int yy=y;yy<y+h;++yy)if(yy>=0&&yy<240)
                for(int xx=x;xx<x+w;++xx)if(xx>=0&&xx<320)p[yy*320+xx]=color;
        }
        void text(const char* s,int x,int y,int scale,uint16_t color){
            static constexpr uint8_t glyphs[][5]={
                {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
                {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
                {54,73,73,73,54},{6,73,73,41,30},
                {126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},
                {127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},
                {0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},
                {127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},
                {62,65,81,33,94},{127,9,25,41,70},{70,73,73,73,49},{1,1,127,1,1},
                {63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},
                {7,8,112,8,7},{97,81,73,69,67},
                {99,19,8,100,99},{32,16,8,4,2}
            };
            for(;*s;++s,x+=6*scale){
                int id=*s>='0'&&*s<='9'?*s-'0':*s>='A'&&*s<='Z'?10+*s-'A':*s=='%'?36:*s=='/'?37:-1;
                if(id<0)continue;
                for(int col=0;col<5;++col)for(int row=0;row<7;++row)
                    if(glyphs[id][col]&(1u<<row))box(x+col*scale,y+row*scale,scale,scale,color);
            }
        }
    } c{pixels};
    constexpr uint16_t background=0x10c3,dim=0x2945,gold=0xc568,white=0xef5b;
    c.box(0,0,320,240,background);
    // A small tower silhouette and travelling light are independent of the
    // content model. No original image or unvalidated byte is consulted here.
    c.box(151,44,18,50,dim);c.box(155,36,10,9,dim);c.box(157,29,6,8,gold);
    c.box(143,93,34,4,dim);c.box(138,98,44,3,dim);
    for(int floor=0;floor<4;++floor)c.box(158,49+floor*11,4,4,((animation>>3)&3)==unsigned(floor)?gold:background);
    c.text("SLAY THE NSPIRE",76,115,2,white);
    static const char*const labels[]={"OPENING","VERIFYING DATA","DIRECTORY","IMAGES","ANIMATION INDEX","FONTS","TEXT","READY"};
    unsigned index=unsigned(stage);if(index>=unsigned(ResourceStage::Count))index=0;
    const char*label=custom_label?custom_label:labels[index];unsigned length=0;while(label[length])++length;
    c.text(label,160-int(length*6)/2,149,1,white);
    if(completed>total)completed=total;
    // Binary-search ceil(total * percentage / 100), after dividing total.
    // Every intermediate fits uint32_t, including UINT32_MAX work counters;
    // Ndless therefore needs neither a 64-bit divide nor an extra runtime.
    unsigned percent=0;
    if(total){
        const uint32_t quotient=total/100u,remainder=total%100u;unsigned high=100;
        while(percent<high){const unsigned middle=(percent+high+1)/2;
            const uint32_t threshold=quotient*middle+(remainder*middle+99u)/100u;
            if(threshold<=completed)percent=middle;else high=middle-1;
        }
    }
    c.box(38,167,244,10,dim);c.box(40,169,int(percent*240u/100u),6,gold);
    char number[6]{};unsigned at=0;
    if(percent==100)number[at++]='1';
    if(percent>=10)number[at++]=char('0'+(percent/10)%10);
    number[at++]=char('0'+percent%10);number[at++]='%';
    c.text(number,160-int(at*6)/2,186,1,white);
    if(!hide_phase){
        char phase[4]={char('1'+index),'/',char('0'+unsigned(ResourceStage::Count)),0};
        c.text(phase,151,210,1,gold);
    }
}
} // namespace spire
