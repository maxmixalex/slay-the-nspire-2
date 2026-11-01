#pragma once
#include "ui_history_archive.hpp"
namespace spire::history_archive {
// Two independently checksummed commit heads bound the visible prefix. A
// torn payload/header never replaces the preceding committed history. The
// adapter must close/flush and reopen in flush(), then permit readback.
struct IO {
 void*context;
 bool(*size)(void*,uint32_t&);
 bool(*read)(void*,uint32_t,void*,unsigned);
 bool(*write)(void*,uint32_t,const void*,unsigned);
 bool(*flush)(void*);
};
inline uint32_t get(const uint8_t*p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);}
inline void put(uint8_t*p,uint32_t x){for(unsigned i=0;i<4;++i)p[i]=uint8_t(x>>(8*i));}
inline uint32_t crc(const uint8_t*p,unsigned n,uint32_t c=~0u){for(unsigned i=0;i<n;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return c;}
constexpr uint32_t Start=64,RecordBytes=24;
struct Head {uint32_t generation=0,end=Start;unsigned slot=1;};
inline bool head(IO&io,Head&h){
 uint32_t size;if(!io.size(io.context,size))return false;
 if(!size){h={};return true;}
 // A failed first creation has no committed record to preserve. Recognize
 // only the exact zero reservation / partially written first empty head;
 // arbitrary corruption of an established archive still reports an error.
 if(size<Start){uint8_t b[Start]{};if(!io.read(io.context,0,b,size))return false;
  for(unsigned i=0;i<size;++i)if(b[i])return false;h={};return true;
 }
 bool found=false;
 for(unsigned i=0;i<2;++i){uint8_t b[32];if(!io.read(io.context,i*32,b,32))return false;
  if(get(b)!=0x3141484eu||get(b+12)!=~get(b+8)||get(b+16)!=~get(b+4)||get(b+28)!=~crc(b,28))continue;
  const uint32_t g=get(b+4),end=get(b+8);if(!g||end<Start||end>size)continue;
  if(!found||g>h.generation){h={g,end,i};found=true;}
 }
 if(found)return true;
 if(size==Start){uint8_t b[Start],first[32]{};
  if(!io.read(io.context,0,b,Start))return false;
  for(unsigned i=32;i<Start;++i)if(b[i])return false;
  put(first,0x3141484eu);put(first+4,1);put(first+8,Start);put(first+12,~Start);put(first+16,~1u);put(first+28,~crc(first,28));
  for(unsigned cut=0;cut<=32;++cut){bool match=true;
   for(unsigned i=0;i<32;++i)if(b[i]!=(i<cut?first[i]:0)){match=false;break;}
   if(match){h={};return true;}
  }
 }
 return false;
}
inline bool record(IO&io,uint32_t at,uint32_t end,uint8_t(&r)[RecordBytes]){
 return at<=end&&RecordBytes<=end-at&&io.read(io.context,at,r,RecordBytes)&&get(r)==0x3152484eu&&
  get(r+4)>=8&&get(r+4)<=UiProgressCapacity&&get(r+4)<=end-at-RecordBytes&&get(r+20)==~crc(r,20);
}
inline bool payload(IO&io,uint32_t at,unsigned n,uint32_t expected,const uint8_t*equal=nullptr){
 uint8_t bytes[512];uint32_t c=~0u;
 for(unsigned done=0;done<n;){const unsigned take=n-done<sizeof bytes?n-done:sizeof bytes;
  if(!io.read(io.context,at+done,bytes,take))return false;
  if(equal)for(unsigned j=0;j<take;++j)if(bytes[j]!=equal[done+j])return false;
  c=crc(bytes,take,c);done+=take;
 }return ~c==expected;
}
inline bool publish(IO&io,const Head&h,uint32_t end){
 if(h.generation==UINT32_MAX)return false;
 uint8_t b[32]{};put(b,0x3141484eu);put(b+4,h.generation+1);put(b+8,end);put(b+12,~end);put(b+16,~get(b+4));put(b+28,~crc(b,28));
 const unsigned slot=h.slot^1u;
 if(!io.write(io.context,slot*32,b,32)||!io.flush(io.context))return false;
 uint8_t verify[32];if(!io.read(io.context,slot*32,verify,32))return false;
 for(unsigned i=0;i<32;++i)if(b[i]!=verify[i])return false;return true;
}
inline UiHistoryArchiveResult access(IO&io,UiHistoryArchiveQuery&q,uint8_t*bytes,unsigned&size){
 using R=UiHistoryArchiveResult;Head h;
 if(!head(io,h))return R::Error;
 if(!h.generation){
  if(q.action!=UiHistoryArchiveAction::Append)return R::Missing;
  uint8_t empty[Start]{};
  if(!io.write(io.context,0,empty,Start)||!publish(io,h,Start)||!head(io,h))return R::Error;
 }
 const bool append=q.action==UiHistoryArchiveAction::Append;
 if(append&&(!bytes||size<12||size>UiProgressCapacity||(get(bytes)!=0x3150484eu&&get(bytes)!=0x3250484eu)||get(bytes+4)!=size||!get(bytes+8)))return R::Error;
 const uint32_t serial=append?get(bytes+8):q.serial,check=append?~crc(bytes,size):0;
 uint32_t selected=0,selected_serial=0,selected_size=0,selected_crc=0;
 // Scan bounded headers only. Opening a page decodes one fixed-size record,
 // not the complete archive, irrespective of how many runs it contains.
 for(uint32_t at=Start;at<h.end;){uint8_t r[RecordBytes];if(!record(io,at,h.end,r))return R::Error;
  const uint32_t n=get(r+4),s=get(r+12),sum=get(r+8);bool choose=false;
  if(append){if(s==serial&&((q.completed&&(get(r+16)&1))||(sum==check&&n==size&&payload(io,at+RecordBytes,n,sum,bytes)))){q.cursor=at;return R::Complete;}}
  else switch(q.action){
   case UiHistoryArchiveAction::HighestSerial:choose=!selected||s>selected_serial;break;
   case UiHistoryArchiveAction::Latest:choose=s==serial;break;
   case UiHistoryArchiveAction::OlderRun:choose=(get(r+16)&1)&&s<serial&&(!selected||s>=selected_serial);break;
   case UiHistoryArchiveAction::NewerRun:choose=(get(r+16)&1)&&s>serial&&(!selected||s<=selected_serial);break;
   case UiHistoryArchiveAction::OlderPage:choose=s==serial&&at<q.cursor;break;
   case UiHistoryArchiveAction::NewerPage:choose=s==serial&&at>q.cursor&&!selected;break;
   default:break;
  }
  if(choose){selected=at;selected_serial=s;selected_size=n;selected_crc=sum;}
  at+=RecordBytes+n;
 }
 if(!append){
  if(!selected)return R::Missing;
  if(q.action==UiHistoryArchiveAction::HighestSerial){q.serial=selected_serial;return R::Complete;}
  if(!bytes||selected_size>size||!io.read(io.context,selected+RecordBytes,bytes,selected_size)||~crc(bytes,selected_size)!=selected_crc)return R::Error;
  size=selected_size;q.serial=selected_serial;q.cursor=selected;return R::Complete;
 }
 if(h.generation==UINT32_MAX||size>UINT32_MAX-h.end-RecordBytes)return R::Error;
 // On a new file reserve both head slots before the first payload. A failed
 // first append leaves no archived record; the live progress remains intact.
 if(!h.generation){uint8_t empty[Start]{};if(!io.write(io.context,0,empty,Start))return R::Error;}
 uint8_t r[RecordBytes]{};put(r,0x3152484eu);put(r+4,size);put(r+8,check);put(r+12,serial);put(r+16,q.completed?1:0);put(r+20,~crc(r,20));
 if(!io.write(io.context,h.end,r,RecordBytes)||!io.write(io.context,h.end+RecordBytes,bytes,size)||!io.flush(io.context)||
    !payload(io,h.end+RecordBytes,size,check,bytes))return R::Error;
 if(!publish(io,h,h.end+RecordBytes+size))return R::Error;
 q.cursor=h.end;return R::Complete;
}
}
