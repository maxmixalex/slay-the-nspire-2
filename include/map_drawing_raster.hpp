#pragma once
#include "map_drawing.hpp"

namespace spire {
struct MapDrawingAlpha {const uint8_t*data=nullptr;unsigned width=0,height=0;};
// A viewport-sized alpha cache, shared by the one visible map. Rebuilds are
// needed only after paper motion or editing. Warm animation redraws neither
// replay the strokes nor read their original textures again.
struct MapDrawingRaster {
    enum {Width=320,Top=22,Height=218};
    uint8_t alpha[Width*Height]{};
    uint16_t left[Height]{},right[Height]{};
    uint32_t distance[MapDrawingCapacity]{};
    uint8_t row[Width]{};
    static int minimum(int a,int b){return a<b?a:b;}
    static int maximum(int a,int b){return a>b?a:b;}
    static uint32_t root(uint64_t value){
        uint64_t bit=UINT64_C(1)<<62,result=0;while(bit>value)bit>>=2;
        while(bit){if(value>=result+bit){value-=result+bit;result=(result>>1)+bit;}else result>>=1;bit>>=2;}
        return uint32_t(result);
    }
    static unsigned texture(const MapDrawingAlpha&t,int u,int v){
        if(!t.data||!t.width||!t.height)return 255;
        const int period=int(t.width)*256;u%=period;if(u<0)u+=period;
        v=maximum(0,minimum(int(t.height-1)*256,v));
        const unsigned x=unsigned(u)/256,y=unsigned(v)/256,x1=(x+1)%t.width,y1=y+1<t.height?y+1:y;
        const unsigned fx=unsigned(u)&255,fy=unsigned(v)&255;
        const unsigned a=t.data[y*t.width+x]*(256-fx)+t.data[y*t.width+x1]*fx;
        const unsigned b=t.data[y1*t.width+x]*(256-fx)+t.data[y1*t.width+x1]*fx;
        return (a*(256-fy)+b*fy+32768)>>16;
    }
    void clear(){
        for(auto&value:alpha)value=0;
        for(unsigned y=0;y<Height;++y){left[y]=Width;right[y]=0;}
    }
    void prepare(const MapDrawingDocument&document){
        for(unsigned i=0;i<document.count;++i){const auto&p=document.points[i];
            if(!i||p.start()){distance[i]=0;continue;}
            const auto&previous=document.points[i-1];const int dx=int(p.x)-int(previous.x),dy=int(p.y())-int(previous.y());
            distance[i]=distance[i-1]+root(uint64_t(dx*dx+dy*dy)<<16);
        }
    }
    void segment_row(const MapDrawingTransform&transform,const MapDrawingPoint&a,const MapDrawingPoint&b,
                     uint32_t path,uint32_t length,unsigned screen_y,int radius,const MapDrawingAlpha&tex){
        const int ax=transform.screen_x_q8(a.x),ay=transform.screen_y_q8(a.y());
        const int bx=transform.screen_x_q8(b.x),by=transform.screen_y_q8(b.y());
        const int y=int(screen_y)*256+128,band=radius+128;
        if(y<minimum(ay,by)-band||y>maximum(ay,by)+band)return;
        const int dx=bx-ax,dy=by-ay;
        int first=minimum(ax,bx)-band,last=maximum(ax,bx)+band;
        if(dy){
            int64_t t0=int64_t(y-band-ay)*65536/dy,t1=int64_t(y+band-ay)*65536/dy;
            if(t0>t1){const int64_t t=t0;t0=t1;t1=t;}
            t0=maximum(0,minimum(65536,int(t0)));t1=maximum(0,minimum(65536,int(t1)));
            const int x0=ax+int(int64_t(dx)*t0/65536),x1=ax+int(int64_t(dx)*t1/65536);
            first=maximum(first,minimum(x0,x1)-band);last=minimum(last,maximum(x0,x1)+band);
        }
        first=maximum(0,first/256-1);last=minimum(Width-1,last/256+1);
        const int len=int((uint64_t(length)*transform.scale+transform.divisor/2)/transform.divisor);
        for(int x=first;x<=last;++x){const int px=x*256+128-ax,py=y-ay;
            int perpendicular=0,along=0;
            if(!len)perpendicular=int(root(uint64_t(int64_t(px)*px+int64_t(py)*py)));
            else{
                along=int((int64_t(px)*dx+int64_t(py)*dy)/len);
                if(along<0)perpendicular=int(root(uint64_t(int64_t(px)*px+int64_t(py)*py)));
                else if(along>len){const int ex=x*256+128-bx,ey=y-by;perpendicular=int(root(uint64_t(int64_t(ex)*ex+int64_t(ey)*ey)));}
                else perpendicular=int((int64_t(dx)*py-int64_t(dy)*px)/len);
            }
            if(perpendicular < -radius||perpendicular>radius)continue;
            along=maximum(0,minimum(len,along));
            const unsigned source_width=a.erase()?24:8;
            const int u=int((uint64_t(path)+uint64_t(along)*transform.divisor/transform.scale)*tex.width/source_width)-128;
            const int v=int(int64_t(perpendicular+radius)*tex.height*256/(2*radius))-128;
            const unsigned coverage=texture(tex,u,v);if(coverage>row[x])row[x]=uint8_t(coverage);
        }
    }
    void stroke(const MapDrawingDocument&document,const MapDrawingTransform&transform,unsigned begin,unsigned end,const MapDrawingAlpha&tex){
        const bool erase=document.points[begin].erase();
        const int radius=(erase?12:4)*transform.scale*256/transform.divisor;
        int top=Height+Top,bottom=Top;
        for(unsigned i=begin;i<end;++i){const int y=transform.screen_y_q8(document.points[i].y());
            top=minimum(top,(y-radius)/256-1);bottom=maximum(bottom,(y+radius)/256+1);}
        top=maximum(Top,top);bottom=minimum(Top+Height-1,bottom);
        for(int y=top;y<=bottom;++y){for(auto&value:row)value=0;
            if(end==begin+1)segment_row(transform,document.points[begin],document.points[begin],0,0,unsigned(y),radius,tex);
            else for(unsigned i=begin+1;i<end;++i)
                segment_row(transform,document.points[i-1],document.points[i],distance[i-1],distance[i]-distance[i-1],unsigned(y),radius,tex);
            for(unsigned x=0;x<Width;++x)if(row[x]){auto&old=alpha[(y-Top)*Width+x];
                // Original line_draw is source-over; line_erase subtracts
                // coverage in the same ordered viewport. Later ink can
                // therefore cover an erased portion without reviving it.
                old=erase?uint8_t(old>row[x]?old-row[x]:0):uint8_t(row[x]+(unsigned(old)*(255-row[x])+127)/255);
                if(old){left[y-Top]=uint16_t(minimum(left[y-Top],int(x)));right[y-Top]=uint16_t(maximum(right[y-Top],int(x+1)));}
            }
        }
    }
    void rebuild(const MapDrawingDocument&document,const MapDrawingTransform&transform,const MapDrawingAlpha&draw,const MapDrawingAlpha&erase){
        clear();prepare(document);
        for(unsigned begin=0;begin<document.count;){unsigned end=begin+1;
            while(end<document.count&&!document.points[end].start())++end;
            stroke(document,transform,begin,end,document.points[begin].erase()?erase:draw);begin=end;
        }
    }
};
static_assert(sizeof(MapDrawingRaster)<80*1024,"Bounded visible-paper alpha cache");
} // namespace spire
