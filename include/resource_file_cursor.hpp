#pragma once
#include <stdint.h>

namespace spire {
// The provider owns one already-open resource stream. Its cursor is separate
// from all save streams and must be reset when opening another resource file.
// A successful exact read leaves the stream at the requested end; every
// failure forgets the position so a retry must establish it with seek.
struct ResourceFileCursor {
    uint32_t position=0;bool known=false;
    bool read(void*file,uint32_t offset,void*output,uint32_t count,
              int(*seek)(void*,long,int),unsigned(*get)(void*,unsigned,unsigned,void*),
              bool&did_seek){
        did_seek=false;
        if(!file||!seek||!get||offset>0x7fffffffu||count>0x7fffffffu-offset){known=false;return false;}
        did_seek=!known||position!=offset;known=false;
        if(did_seek&&seek(file,(long)offset,0)!=0)return false;
        if(get(output,1,count,file)!=count)return false;
        position=offset+count;known=true;return true;
    }
};
} // namespace spire
