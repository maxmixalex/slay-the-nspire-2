#pragma once
#include "source_scene_oracle.hpp"

// Independent LCD text raster oracle. Read the original mask and UI40 font
// metrics, then place/composite scalar pixels without Canvas, fitted_text or
// the production profile label helpers. The older source-coordinate oracle
// remains unchanged for tests that deliberately exercise that legacy layout.
namespace spire_test {
struct LcdLetter {
    SceneLetter mask;
    int advance64=0,kerning64=0,bearing=0,extent=0;
};
static std::vector<LcdLetter> lcd_scene_letters(const Content&content,const std::vector<uint8_t>&pack,
                                              const std::string&text,unsigned size,unsigned face){
    auto masks=scene_letters(content,pack,text,size,face);
    std::vector<LcdLetter> letters;RichReader reader(text.c_str());RichChar ch{};unsigned i=0;
    while(reader.next(ch)){
        auto mask=std::move(masks[i++]);
        if(ch.codepoint=='\n'){letters.push_back({std::move(mask)});continue;}
        const unsigned selected=face|(ch.style.bold?5u:0u);
        const auto glyph=content.glyph(ch.codepoint,int(size),selected);
        const int kern=ch.previous_font==rich_font_style(ch)?content.glyph_kerning64(ch.previous,ch.codepoint,int(size),selected):0;
        letters.push_back({std::move(mask),glyph.advance64?int(glyph.advance64):int(glyph.advance)*64,
                           kern,glyph.bearing_x,std::max(int(glyph.w)+glyph.bearing_x,int(glyph.advance))});
    }
    return letters;
}
static int lcd_scene_width(const std::vector<LcdLetter>&letters){
    int pen=0;for(const auto&letter:letters){if(letter.mask.cp=='\n')break;pen+=(pen?letter.kerning64:0)+letter.advance64;}
    return (pen+32)/64;
}
static void lcd_scene_label(std::vector<uint16_t>&frame,const std::vector<LcdLetter>&letters,int x,int y,int width,int height,
                            unsigned size,uint16_t color,unsigned shadow,int outline=0,uint16_t outline_color=0,int shadow_outline=0,
                            bool center=true,bool wrap=false,unsigned outline_alpha=255,int pivot_x=0,int pivot_y=0,int scale=4096,bool clip=false,
                            int shadow_x=1,int shadow_y=0,unsigned overall_alpha=255,unsigned brightness=255,bool top_aligned=false){
    struct Placed {const LcdLetter*letter;int x;};
    struct Line {std::vector<Placed>glyphs;int extent=0;};
    std::vector<Line> lines(1);int pen=0;bool boundary=true;
    const auto pixel=[](int value){return (value+32)>>6;};
    const auto word=[](unsigned cp){return cp>32&&(cp<0x300||cp==0x202f||(cp>=0x370&&cp<0x530)||(cp>=0xac00&&cp<=0xd7a3));};
    for(unsigned i=0;i<letters.size();++i){
        const auto&letter=letters[i];const unsigned cp=letter.mask.cp;
        if(cp=='\n'){if(!wrap)break;lines.emplace_back();pen=0;boundary=true;continue;}
        int at=pixel(pen+(pen?letter.kerning64:0));const bool space=cp==' '||cp=='\t';
        if(wrap&&space&&(!pen||at+letter.extent>width)){boundary=true;continue;}
        int word_extent=0;
        if(wrap&&boundary&&pen&&word(cp)){
            int advance=0;
            for(unsigned n=i;n<letters.size()&&word(letters[n].mask.cp);++n){
                const auto&next=letters[n];if(n!=i)advance+=next.kerning64;
                word_extent=pixel(advance)+next.extent;advance+=next.advance64;
            }
        }
        if(wrap&&pen&&(at+letter.extent>width||(word_extent&&word_extent<=width&&at+word_extent>width))){
            lines.emplace_back();pen=at=0;
        }
        boundary=space;if(wrap&&!pen&&space)continue;
        lines.back().glyphs.push_back({&letter,at});lines.back().extent=at+letter.extent;
        pen+=(pen?letter.kerning64:0)+letter.advance64;
    }
    if(lines.size()>32)lines.resize(32);
    const int text_height=int(lines.size())*int(size+1)+1;
    const int top=top_aligned||(clip&&text_height>height)?y:y+(height-text_height)/2;
    const auto project=[&](int value,int pivot){const int delta=(value-pivot)*scale;return pivot+(delta+(delta<0?-2048:2048))/4096;};
    const auto pass=[&](int dx,int dy,unsigned opacity,uint16_t ink,bool decoration){
        for(unsigned row=0;row<lines.size();++row){const auto&line=lines[row];
            const int centered=center&&(!clip||line.extent<=width)?(width-line.extent)/2:0;
            for(const auto&placed:line.glyphs){const auto&letter=*placed.letter;const auto&mask=letter.mask;
                const uint16_t colors[]={color,scene_rgb(239,200,81),scene_rgb(255,85,85),scene_rgb(135,206,235),scene_rgb(127,255,0),scene_rgb(238,130,238),scene_rgb(255,165,24),scene_rgb(0,255,255),scene_rgb(255,255,255),scene_rgb(128,128,128),scene_rgb(255,120,160)};
                const uint16_t original=decoration?ink:colors[mask.color];
                const uint16_t modulated=uint16_t((((original>>11)*brightness+127)/255)<<11|
                    (((((original>>5)&63)*brightness+127)/255)<<5)|(((original&31)*brightness+127)/255));
                for(unsigned yy=0;yy<mask.height;++yy)for(unsigned xx=0;xx<mask.width;++xx){
                    const unsigned layer=(opacity*overall_alpha+127)/255,alpha=(mask.alpha[yy*mask.width+xx]*layer+127)/255;
                    const int px=x+centered+placed.x+letter.bearing+int(xx)+dx,py=top+int(row*(size+1))+int(yy)+dy;
                    for(int sy=project(py,pivot_y);sy<project(py+1,pivot_y);++sy)for(int sx=project(px,pivot_x);sx<project(px+1,pivot_x);++sx){
                        if(clip&&(sx<project(x,pivot_x)||sx>=project(x+width,pivot_x)||sy<project(y,pivot_y)||sy>=project(y+height,pivot_y)))continue;
                        scene_pixel(frame,sx,sy,modulated,alpha);
                    }
                }
            }
        }
    };
    if(shadow)for(int yy=-shadow_outline;yy<=shadow_outline;++yy)for(int xx=-shadow_outline;xx<=shadow_outline;++xx)
        if(xx*xx+yy*yy<=shadow_outline*shadow_outline)pass(xx+shadow_x,yy+shadow_y,shadow,0,true);
    for(int yy=-outline;yy<=outline;++yy)for(int xx=-outline;xx<=outline;++xx)
        if((xx||yy)&&xx*xx+yy*yy<=outline*outline)pass(xx,yy,outline_alpha,outline_color,true);
    pass(0,0,255,color,false);
}
}
