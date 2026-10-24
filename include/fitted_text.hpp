#pragma once
#include "rich_text.hpp"

namespace spire { namespace fitted_text {
struct Metric {int advance=0,width=0;};
struct Plan {int size=8,rows=0,total=0,step=9;bool truncated=false;};
inline int next_size(int size){return size>16?16:size>12?12:size>10?10:size>8?8:size>6?6:5;}
inline int row_capacity(int height,int size){return height>2?(height-2)/(size+1):0;}
inline bool word_character(uint32_t cp){return cp>32&&cp<0x300;}
template<class Metrics> int word_width(const RichReader&tail,const RichChar&first,int size,Metrics metrics){
    RichReader reader=tail;RichChar ch{};auto g=metrics(first,size);int advance=g.advance,width=g.width;
    while(reader.next(ch)&&word_character(ch.codepoint)){g=metrics(ch,size);width=advance+g.width;advance+=g.advance;}
    return width;
}
template<class Metrics> int line_count(const char*text,int width,int size,Metrics metrics){
    if(!*text||width<=0)return 0;
    RichReader reader(text);RichChar ch{};int at=0,row=1;bool boundary=true;
    while(reader.next(ch)){
        if(ch.codepoint=='\n'){at=0;++row;boundary=true;continue;}
        const auto g=metrics(ch,size);
        if((ch.codepoint==' '||ch.codepoint=='\t')&&(!at||at+g.width>width)){boundary=true;continue;}
        int word=boundary&&at&&word_character(ch.codepoint)?word_width(reader,ch,size,metrics):0;
        if(at&&(at+g.width>width||(word&&word<=width&&at+word>width))){at=0;++row;}
        boundary=ch.codepoint==' '||ch.codepoint=='\t';if(!at&&boundary)continue;
        at+=g.advance;
    }return row;
}
template<class Metrics> Plan plan(const char*text,int width,int height,int preferred,int minimum,Metrics metrics){
    Plan result;result.size=preferred<5?5:preferred;minimum=minimum<5?5:minimum;
    if(width<=0||height<=2||!*text)return result;
    for(;;){
        result.rows=row_capacity(height,result.size);result.step=result.size+1;
        result.total=line_count(text,width,result.size,metrics);
        if((result.rows&&result.total<=result.rows)||result.size<=minimum)break;
        int next=next_size(result.size);result.size=next<minimum?minimum:next;
    }
    result.truncated=result.total>result.rows;
    if(result.rows>result.total)result.rows=result.total;
    return result;
}
// Reserve the actual ellipsis advance before the final visible line is
// painted. No already-drawn characters are covered by a late ellipsis.
// RichReader retains keyword colors/emphasis while fitting UTF-8 glyphs.
template<class Metrics,class Emit> void visit(const char*text,int width,const Plan&plan,Metrics metrics,Emit emit){
    if(!plan.rows||width<=0)return;
    RichReader reader(text);RichChar ch{};int at=0,row=0;bool boundary=true;
    const RichChar dots{0x2026,{},0};const auto ellipsis=metrics(dots,plan.size);
    while(reader.next(ch)){
        const auto g=metrics(ch,plan.size);
        if((ch.codepoint==' '||ch.codepoint=='\t')&&(!at||at+g.width>width)){boundary=true;continue;}
        int word=boundary&&at&&word_character(ch.codepoint)?word_width(reader,ch,plan.size,metrics):0;
        const bool wrap=at&&(at+g.width>width||(word&&word<=width&&at+word>width));
        boundary=ch.codepoint==' '||ch.codepoint=='\t'||ch.codepoint=='\n';
        if(row==plan.rows-1&&plan.truncated){
            if(ch.codepoint=='\n'||wrap||at+g.width>width-ellipsis.width){emit(dots,at,row,plan.size);return;}
        }else if(ch.codepoint=='\n'||wrap){
            at=0;if(++row>=plan.rows)return;if(ch.codepoint=='\n')continue;
            if(row==plan.rows-1&&plan.truncated&&g.width>width-ellipsis.width){emit(dots,0,row,plan.size);return;}
        }
        if(!at&&boundary)continue;
        emit(ch,at,row,plan.size);at+=g.advance;
    }
    if(plan.truncated)emit(dots,at,row,plan.size);
}
}}
