// The calculator file can wrap the unchanged canonical save in a small
// zero/literal stream. NSZ1 is only a host storage envelope: save()/load(),
// migration, revision, RNG and backup identities remain canonical NSV1.
// A size pass chooses this envelope only when it is strictly smaller. The
// fallback is the original raw file, so incompressible input never grows.
#include "storage_codec.inc"
namespace run_storage {
using storage_codec::HeaderBytes;
using storage_codec::BlockBytes;
using storage_codec::PacketBytes;
using storage_codec::encoded_size;
using storage_codec::write;
static bool read(void*file,spire::SaveData&out){
    out.size=0;unsigned size=0;
    if(!storage_codec::read(file,out.bytes,sizeof out.bytes,size,16,true))return false;
    out.size=size;return true;
}
} // namespace run_storage

static bool save_game(const spire::State&s){
    if(!resource_mutation_allowed())return false;
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Save);if(!transfer)return false;
    auto&scratch=*transfer.save();if(!spire::save(s,scratch))return false;
    void*f=nd_open(temp_path,"wb");if(!f)return false;
    bool good=run_storage::write(f,scratch.bytes,unsigned(scratch.size));
    good=(nd_close(f)==0)&&good;if(!good){nd_unlink(temp_path);return false;}
    const int old=profile_file_present(save_path);
    if(old<0){nd_unlink(temp_path);return false;}
    if(old){
        if(!remove_profile_file(backup_path)||nd_rename(save_path,backup_path)!=0){nd_unlink(temp_path);return false;}
    }
    if(nd_rename(temp_path,save_path)!=0){
        if(old)nd_rename(backup_path,save_path);
        nd_unlink(temp_path);return false;
    }
    // Unlike optional UI history, the preceding run remains recoverable.
    return true;
}
static bool clear_save(){
    if(!resource_mutation_allowed())return false;
    // A deliberate abandon also removes the preceding atomic-save backup.
    // Otherwise a later recovery could bring back an abandoned expedition.
    const char*paths[]={backup_path,temp_path,save_path};for(unsigned i=0;i<3;i++){const char*path=paths[i];
        void*f=nd_open(path,"rb");if(!f)continue;if(nd_close(f)!=0||nd_unlink(path)!=0)return false;
    }return true;
}
static bool load_game(spire::State&s){
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Save);if(!transfer)return false;
    void*f=nd_open(save_path,"rb");if(!f)return false;
    auto&scratch=*transfer.save();const bool read=run_storage::read(f,scratch);
    const bool closed=nd_close(f)==0;
    // Keep the existing primary-only policy. A partial decode or close error
    // never reaches the transactional canonical loader or publishes a State.
    return read&&closed&&spire::load(s,scratch.bytes,scratch.size);
}
