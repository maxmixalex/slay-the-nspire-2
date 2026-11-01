// Requires the host's nd_open/read/write/seek/tell/close/unlink wrappers.
// No additional staging buffer; every operation borrows the caller's bounded
// page transfer and closes its descriptor before returning to the App.
#include "history_archive_store.hpp"
struct HistoryFileAccess {
 void*file=nullptr;const char*path=nullptr;bool writable=false;
 static bool size(void*context,uint32_t&n){auto&f=*static_cast<HistoryFileAccess*>(context);
  if(!f.file||nd_seek(f.file,0,2)!=0)return false;const long end=nd_tell(f.file);if(end<0||static_cast<unsigned long>(end)>UINT32_MAX)return false;n=uint32_t(end);return true;}
 static bool read(void*context,uint32_t at,void*out,unsigned n){auto&f=*static_cast<HistoryFileAccess*>(context);
  return f.file&&at<=0x7fffffffu&&nd_seek(f.file,long(at),0)==0&&nd_read(out,1,n,f.file)==n;}
 static bool write(void*context,uint32_t at,const void*in,unsigned n){auto&f=*static_cast<HistoryFileAccess*>(context);
  return f.file&&f.writable&&at<=0x7fffffffu&&n<=0x7fffffffu-at&&nd_seek(f.file,long(at),0)==0&&nd_write(in,1,n,f.file)==n;}
 static bool flush(void*context){auto&f=*static_cast<HistoryFileAccess*>(context);if(!f.file)return false;
  const bool closed=nd_close(f.file)==0;f.file=nd_open(f.path,f.writable?"r+b":"rb");return closed&&f.file;}
};
static spire::UiHistoryArchiveResult history_file_access(const char*path,spire::UiHistoryArchiveQuery&q,uint8_t*bytes,unsigned&n){
 using namespace spire;using Result=UiHistoryArchiveResult;
 const bool writable=q.action==UiHistoryArchiveAction::Append;
 HistoryFileAccess file{nd_open(path,writable?"r+b":"rb"),path,writable};bool created=false;
 if(!file.file){if(!writable)return Result::Missing;
  // A failed update open must not truncate an existing archive. A separate
  // read establishes absence under the same foreground-only ownership.
  void*check=nd_open(path,"rb");if(check){nd_close(check);return Result::Error;}
  file.file=nd_open(path,"w+b");created=file.file!=nullptr;if(!file.file)return Result::Error;
 }
 history_archive::IO io{&file,HistoryFileAccess::size,HistoryFileAccess::read,HistoryFileAccess::write,HistoryFileAccess::flush};
 auto result=history_archive::access(io,q,bytes,n);
 if(created&&result==Result::Error){history_archive::Head head;const bool empty=!history_archive::head(io,head)||head.end==history_archive::Start;
  if(file.file){nd_close(file.file);file.file=nullptr;}if(empty)nd_unlink(path);
 }
 if(file.file&&nd_close(file.file)!=0){return Result::Error;}
 return result;
}
