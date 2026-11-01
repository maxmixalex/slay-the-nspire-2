// Sample only smooth additive illumination at two-pixel spacing. The scene
// behind it is never resampled: interpolation produces a color contribution,
// which is added to each original full-resolution destination pixel.
class SoftAddGrid {
    static constexpr unsigned W=161,H=121;
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
    // centers .5,2.5,... when scaling the affine geometry by two.
    int position(int value)const{return active?(value+128)/2:value;}
    int size(int value)const{return active?value/2:value;}
    bool sampled()const{return active;}
    void finish(){
        if(!active)return;
        for(unsigned y=0;y<240;y+=2){
            if(!(y&15))destination.poll();
            const uint16_t*top=reduced.pixels+(y/2)*W,*bottom=top+W;
            uint16_t*out=destination.pixels+y*destination.pitch;
            for(unsigned x=0;x<320;x+=2){
                const unsigned k=x/2;const uint16_t a=top[k],b=top[k+1],c=bottom[k],d=bottom[k+1];
                if(!(a|b|c|d))continue;
                out[x]=presentation::add_preweighted(out[x],a);
                out[x+1]=presentation::add_preweighted(out[x+1],average(a,b));
                out[x+destination.pitch]=presentation::add_preweighted(out[x+destination.pitch],average(a,c));
                out[x+destination.pitch+1]=presentation::add_preweighted(out[x+destination.pitch+1],neow_post_ui23::average4(a,b,c,d));
            }
        }
    }
};
