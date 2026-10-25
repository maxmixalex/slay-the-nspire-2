#pragma once
#include <stdint.h>
namespace spire { namespace card_highlight {
constexpr unsigned Full=65536,MaximumWidth=4915; // source .075
struct Fraction {
    unsigned divisor,reciprocal;
    explicit Fraction(unsigned denominator):divisor(denominator),reciprocal(denominator?unsigned((uint64_t(1)<<32)/denominator):0){}
    unsigned smooth(unsigned value)const{
        if(!divisor||value>=divisor)return Full;
        // For value<divisor, value*floor(2^32/divisor) fits uint32.
        // The estimate is at most two low for our denominator<=75000.
        // Unsigned wrap preserves the small positive remainder exactly.
        unsigned q=(value*reciprocal)>>16;
        unsigned remainder=(value<<16)-q*divisor;
        if(remainder>=divisor){++q;remainder-=divisor;}
        if(remainder>=divisor)++q;
        const unsigned q2=(q*q)>>16;
        return (q2*(3*Full-2*q))>>16;
    }
};
inline unsigned smooth(unsigned value,unsigned maximum){return Fraction(maximum).smooth(value);}
inline unsigned source_alpha(uint16_t alpha){
    // 980000 = 14*65535 + 62510. The remaining product and the
    // Mersenne-divisor correction both fit uint32 over all uint16 values.
    const unsigned n=unsigned(alpha)*62510u;
    return unsigned(alpha)*14u+((n+1u+(n>>16))>>16);
}
// card_ripple.gdshader, evaluated on the projected source RGBAH distance
// field. Fixed point and RGB565 projection are explicit target adaptations;
// this is not a claim of bit equality with the desktop GPU rasterizer.
struct Context {
    Fraction edge;
    unsigned time;
    explicit Context(unsigned width,uint32_t milliseconds):
        edge(unsigned(uint64_t(width>MaximumWidth?MaximumWidth:width)*1000000/Full)),
        time((milliseconds%2000)*30){}
    unsigned transparent_through()const{
        // The existing early return is floor(alpha*980000/65535) <=
        // 980000-edge.divisor. Solve for the greatest inclusive raw alpha.
        // Reducing by five keeps the numerator below 2^30 over every allowed
        // width, so this preparation needs only constant 32-bit arithmetic.
        return 65535u-(edge.divisor*13107u+182893u)/196000u;
    }
    unsigned alpha(uint16_t original_alpha)const{
        if(!edge.divisor)return 0;
        const unsigned source=source_alpha(original_alpha);
        // The shader cannot reach the edge at these distances. This also
        // avoids evaluating the ripple over its large transparent exterior.
        if(source+20000u<=1000000u-edge.divisor)return 0;
        const unsigned n=time+source;
        const unsigned quotient=unsigned((uint64_t(n)*214748u)>>32);
        unsigned ripple=n-quotient*20000u;if(ripple>=20000u)ripple-=20000u;
        const Fraction front_edge(5000);
        const unsigned front=front_edge.smooth(ripple);
        const int brightness=int(source)-int(ripple)+int((front*20000u)>>16);
        const int distance=brightness-int(1000000u-edge.divisor);
        return distance<=0?0:(edge.smooth(unsigned(distance))*255u)>>16;
    }
};
inline unsigned alpha(uint16_t original_alpha,unsigned width,uint32_t milliseconds){return Context(width,milliseconds).alpha(original_alpha);}
} }
