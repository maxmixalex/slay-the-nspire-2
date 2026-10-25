// NSZ1 is a host-only envelope. UiProgress, NSH1..9, profile summaries and
// browser exports continue to use the unchanged canonical history codec.
#include "storage_codec.inc"
static bool load_progress(spire::UiProgress&out){
    progress_recovery=ProgressRecovery::Unknown;
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Bytes);if(!transfer)return false;
    uint8_t*bytes=transfer.bytes();
    storage_codec::ReadStatus primary=storage_codec::ReadStatus::IoFailure;
    const char*const paths[]={progress_path,progress_backup};
    for(unsigned index=0;index<2;++index){
        void*f=nd_open(paths[index],"rb");if(!f)continue;
        unsigned n=0;storage_codec::ReadStatus status;
        const bool read=progress_file_read(f,bytes,n,status),closed=nd_close(f)==0;
        if(!closed)status=storage_codec::ReadStatus::IoFailure;
        if(read&&closed){
            if(spire::decode_ui_progress(bytes,n,out)){
                if(!index)progress_recovery=ProgressRecovery::PrimaryValid;
                else if(primary==storage_codec::ReadStatus::Invalid)progress_recovery=ProgressRecovery::BackupAfterInvalidPrimary;
                return true;
            }
            status=storage_codec::ReadStatus::Invalid;
        }
        if(!index)primary=status;
    }
    return false;
}
static bool save_progress(const spire::UiProgress&value){
    spire::ResourceTransferLease transfer(spire::ResourceTransferUse::Bytes);if(!transfer)return false;
    uint8_t*bytes=transfer.bytes();
    const unsigned n=unsigned(spire::encode_ui_progress(value,bytes,spire::UiProgressCapacity));if(!n)return false;
    void*f=nd_open(progress_temp,"wb");if(!f)return false;
    bool ok=storage_codec::write(f,bytes,n);ok=(nd_close(f)==0)&&ok;
    if(!ok){nd_unlink(progress_temp);return false;}
    const int old=profile_file_present(progress_path),backup=profile_file_present(progress_backup);
    if(old<0||backup<0){progress_recovery=ProgressRecovery::Unknown;nd_unlink(progress_temp);return false;}
    bool moved=false;
    if(old&&progress_recovery==ProgressRecovery::BackupAfterInvalidPrimary){
        // The existing full load certified the primary as malformed and
        // decoded the backup successfully. Never rotate those bad bytes over
        // the only known-good recovery file. A failed commit leaves it intact.
        if(!backup||nd_unlink(progress_path)!=0){nd_unlink(progress_temp);return false;}
    }else if(old){
        if(backup&&progress_recovery!=ProgressRecovery::PrimaryValid){
            // A prior I/O error proves nothing about either old file. If the
            // OS supports an atomic replacement it can commit without first
            // deleting either copy; otherwise keep both and retry after load.
            if(nd_rename(progress_temp,progress_path)!=0){nd_unlink(progress_temp);return false;}
            progress_recovery=ProgressRecovery::PrimaryValid;nd_unlink(progress_backup);return true;
        }
        if(!remove_profile_file(progress_backup)||nd_rename(progress_path,progress_backup)!=0){nd_unlink(progress_temp);return false;}
        moved=true;
    }
    if(nd_rename(progress_temp,progress_path)!=0){
        if(moved&&nd_rename(progress_backup,progress_path)!=0)progress_recovery=ProgressRecovery::Unknown;
        nd_unlink(progress_temp);return false;
    }
    progress_recovery=ProgressRecovery::PrimaryValid;
    // Once the new primary committed, a failed optional backup cleanup cannot
    // invalidate it. The next successful load/save can clear this stale copy.
    if(old||backup)nd_unlink(progress_backup);return true;
}
