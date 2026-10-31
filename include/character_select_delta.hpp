#pragma once
#include <stdint.h>
#include "framed_animation_ui22.hpp"

namespace spire::character_select_delta {
constexpr unsigned Width=320,Height=240,KeyInterval=8,Characters=5;
constexpr unsigned AuxiliaryFirst=FramedUi22AuxiliaryCount;
constexpr unsigned AuxiliaryCount=AuxiliaryFirst+Characters;
constexpr unsigned ResidentPages[Characters]={1600,2000,2000,1600,1600};
constexpr bool extended_cache(unsigned character){return character==1||character==2;}

template<class Content> bool available(const Content& content,unsigned character,unsigned first){
    return character<Characters&&content.ui_image(int(AuxiliaryFirst+character))==int(first);
}

// This occupies the existing mutually-exclusive scene raster slot. A selected
// background is opaque; copy patches change only source pixels which differ
// from the preceding frame. No interpolation, quantization or frame is lost.
struct Raster {
    const void* owner=nullptr;
    uint32_t generation=0;
    unsigned first=0,index=0;
    bool valid=false;
    uint16_t pixels[Width*Height];

    static unsigned word(const uint8_t* p){return unsigned(p[0])|(unsigned(p[1])<<8);}
    template<class Picture> bool apply(Picture p,bool key,void(*poll)()){
        if(!p.pixels||p.w!=Width||p.h!=Height)return false;
        if(p.format==0&&!p.alpha&&!p.palette){
            for(unsigned y=0;y<Height;y+=16){
                if(poll)poll();
                __builtin_memcpy(pixels+y*Width,p.pixels+y*Width*2,16*Width*2);
            }
            return true;
        }
        if(p.format!=4)return false;
        // Content publishes only bounded, validated sparse images. The extra
        // checks here enforce this scene's opaque key/copy-patch contract.
        const uint8_t* raw=p.pixels;
        for(unsigned y=0;y<Height;++y){
            if(!(y&15u)&&poll)poll();
            unsigned count=word(raw);raw+=2;
            if(key&&count!=1)return false;
            for(unsigned span=0;span<count;++span){
                const unsigned x=word(raw),length=word(raw+2),mode=raw[4];raw+=5;
                if(mode||!length||x+length>Width||(key&&(x||length!=Width)))return false;
                __builtin_memcpy(pixels+y*Width+x,raw,length*2);raw+=length*2;
            }
        }
        return true;
    }

    template<class Content> bool restore(Content& content,unsigned clip_first,unsigned target,
                                        void(*poll)()=nullptr){
        const unsigned key=target-target%KeyInterval;
        unsigned next=key;
        if(valid&&owner==&content&&generation==content.generation()&&first==clip_first&&
           index>=key&&index<=target){
            if(index==target)return true;
            next=index+1;
        }else{
            valid=false;
            // Every checkpoint is relative to the same opaque first frame.
            // Resetting from it avoids retaining a complete full-screen image
            // at every eighth sample, keeping more room for cold tile groups.
            if(!apply(content.image(int(clip_first)),true,poll))return false;
            if(key&&!apply(content.image(int(clip_first+key)),false,poll))return false;
            owner=&content;generation=content.generation();first=clip_first;next=key+1;
        }
        // A seek reads the base, at most one checkpoint and seven time patches.
        for(unsigned frame=next;frame<=target;++frame){
            if(!apply(content.image(int(clip_first+frame)),false,poll)){valid=false;return false;}
        }
        index=target;valid=true;return true;
    }
};
}
