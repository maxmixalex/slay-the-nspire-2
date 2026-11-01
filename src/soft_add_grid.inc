// Sample only smooth additive illumination at four-pixel spacing. The scene
// behind it is never resampled: interpolation produces a color contribution,
// which is added to each original full-resolution destination pixel.
class SoftAddGrid {
    static constexpr unsigned W=81,H=61;
    ForegroundScratch<uint16_t,W*H> storage;
    Canvas&destination;
    Canvas reduced;
    bool active;
    static uint16_t average(uint16_t a,uint16_t b){
        // Independent round-to-nearest channels without cross-channel carry.
        return uint16_t((a&b)+(((a^b)&0xf7deu)>>1)+((a^b)&0x0821u));
    }
public:
    explicit SoftAddGrid(Canvas&c,bool enabled=true):destination(c),reduced(c),
        active(enabled&&scene_opaque_canvas(c)&&bool(storage)){
        if(!active)return;
        reduced.pixels=storage.array();reduced.pitch=reduced.clip_w=W;reduced.clip_h=H;
        __builtin_memset(reduced.pixels,0,W*H*sizeof(uint16_t));
    }
    Canvas&canvas(){return active?reduced:destination;}
    // Canvas raster centers are x+.5; this offset preserves the original
    // centers .5,4.5,... when scaling the affine geometry by four.
    int position(int value)const{return active?(value+384)/4:value;}
    int size(int value)const{return active?value/4:value;}
    bool sampled()const{return active;}
    void finish(){
        if(!active)return;
        for(unsigned y=0;y<240;y+=4){
            if(!(y&15))destination.poll();
            const uint16_t*top=reduced.pixels+(y/4)*W,*bottom=top+W;
            uint16_t*out=destination.pixels+y*destination.pitch;
            for(unsigned x=0;x<320;x+=4){
                const unsigned k=x/4;const uint16_t a=top[k],b=top[k+1],c=bottom[k],d=bottom[k+1];
                if(!(a|b|c|d))continue;
                const uint16_t middle_left=average(a,c),middle_right=average(b,d);
                const uint16_t left[4]={a,average(a,middle_left),middle_left,average(middle_left,c)};
                const uint16_t right[4]={b,average(b,middle_right),middle_right,average(middle_right,d)};
                for(unsigned row=0;row<4;++row){
                    const uint16_t middle=average(left[row],right[row]);
                    const uint16_t value[4]={left[row],average(left[row],middle),middle,average(middle,right[row])};
                    uint16_t*target=out+x+row*destination.pitch;
                    for(unsigned column=0;column<4;++column)target[column]=presentation::add_preweighted(target[column],value[column]);
                }
            }
        }
    }
};
