#include "presentation_blend.hpp"
#include <cstdint>
#include <cstdio>
#include <cstdlib>

// The oracle uses independent 8-bit-alpha channel arithmetic, not packed
// lanes or the production divide-by-255 shortcut.
static uint16_t reference(uint16_t a,uint16_t b,unsigned alpha){
    const auto channel=[&](unsigned shift,unsigned mask){
        return ((((a>>shift)&mask)*(255-alpha)+((b>>shift)&mask)*alpha+127)/255)<<shift;
    };
    return uint16_t(channel(11,31)|channel(5,63)|channel(0,31));
}
static uint64_t checks;
static void check(uint16_t a,uint16_t b,unsigned alpha){
    ++checks;
    const uint16_t got=spire::presentation::source_over_rgb565(a,b,alpha);
    const uint16_t expected=reference(a,b,alpha);
    if(got!=expected){
        std::fprintf(stderr,"blend a=%04x b=%04x alpha=%u got=%04x expected=%04x\n",a,b,alpha,got,expected);
        std::exit(1);
    }
}
int main(){
    // Exhaust all four independently varying packed red/blue components at
    // every alpha. Mixed increasing/decreasing lanes exercise unsigned borrow.
    for(unsigned alpha=0;alpha<256;++alpha)
    for(unsigned a=0;a<1024;++a)for(unsigned b=0;b<1024;++b){
        const unsigned ag=((a>>2)^b)&63,bg=((b>>3)^(a*13u))&63;
        check(uint16_t(((a>>5)<<11)|(ag<<5)|(a&31)),
              uint16_t(((b>>5)<<11)|(bg<<5)|(b&31)),alpha);
    }
    // Green's complete 6-bit domain independently crosses every alpha.
    for(unsigned alpha=0;alpha<256;++alpha)
    for(unsigned a=0;a<64;++a)for(unsigned b=0;b<64;++b)
        check(uint16_t((a<<5)|0xf81f),uint16_t(b<<5),alpha);
    std::printf("rgb565_blend_regression: %llu exact checks passed\n",
                static_cast<unsigned long long>(checks));
}
