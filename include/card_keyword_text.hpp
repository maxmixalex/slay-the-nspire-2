#pragma once
#include <stddef.h>
#include <stdint.h>
namespace spire { namespace card_keyword_text {
// CardKeywordOrder.beforeDescription is prepended in reverse iteration
// order. Exhaust and Eternal follow the complete model/modifier prose.
constexpr unsigned Count=7,BeforeCount=5;
constexpr uint8_t Flags[Count]={128,2,4,16,8,1,64};
constexpr const char*Keys[Count]={"UNPLAYABLE","INNATE","RETAIN","SLY","ETHEREAL","EXHAUST","ETERNAL"};
struct Labels {char title[Count][48]{};char period[8]{};};
inline bool part(const char*&at,const char*end,const char*value){
 while(*value){if(at==end||*at!=*value)return false;++at;++value;}return true;
}
inline bool paragraph(const char*line,const char*end,const char*title,const char*period,bool old_period){
 if(!*title||!*period)return false;
 while(line<end&&(*line==' '||*line=='\t'||*line=='\r'))++line;
 while(end>line&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'))--end;
 return part(line,end,"[gold]")&&part(line,end,title)&&
  part(line,end,old_period?period:"[/gold]")&&part(line,end,old_period?"[/gold]":period)&&line==end;
}
// Only a complete standalone keyword paragraph is removed. A sentence
// mentioning a gold keyword, a quoted phrase or an affliction stays intact.
inline void strip(char*body,const Labels&labels){
 char*read=body,*write=body;
 while(*read){char*end=read;while(*end&&*end!='\n')++end;char*next=*end?end+1:end;bool keyword=false;
  for(unsigned i=0;i<Count&&!keyword;++i)keyword=paragraph(read,end,labels.title[i],labels.period,false)||paragraph(read,end,labels.title[i],labels.period,true);
  if(!keyword){while(read<next)*write++=*read++;}
  else if(!*end&&write>body&&write[-1]=='\n')--write;
  read=next;
 }
 *write=0;
}
struct Writer {
 char*data;size_t capacity,used=0;bool truncated=false;
 void character(char value){if(used+1<capacity){data[used++]=value;data[used]=0;}else truncated=true;}
 void text(const char*value){while(*value)character(*value++);}
 void line(const char*value){if(!*value)return;if(used)character('\n');text(value);}
 void keyword(const char*title,const char*period){if(!*title||!*period)return;if(used)character('\n');text("[gold]");text(title);text("[/gold]");text(period);}
};
// body is already stripped, formatted and extended with model-owned prose.
// Caller supplies separate buffers; no heap or persistent text copy is used.
inline bool compose(char*out,size_t capacity,const char*body,unsigned flags,const Labels&labels){
 if(!capacity)return false;out[0]=0;Writer writer{out,capacity};
 for(unsigned i=0;i<BeforeCount;++i)if(flags&Flags[i])writer.keyword(labels.title[i],labels.period);
 writer.line(body);
 for(unsigned i=BeforeCount;i<Count;++i)if(flags&Flags[i])writer.keyword(labels.title[i],labels.period);
 return !writer.truncated;
}
} }
