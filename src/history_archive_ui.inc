// Persistent history is paged separately from canonical game saves. The old
// 32-run/96-floor arrays remain bounded working windows; archive reads borrow
// an existing scene-union slot only while the History screen is visible.
bool App::archive_history_run(const UiRunHistory&run){
 if(!callbacks.history_archive||!run.serial)return false;
 ResourceTransferLease transfer(ResourceTransferUse::Bytes);if(!transfer)return false;
 unsigned n=unsigned(encode_ui_history_page(run,progress.journal,transfer.bytes(),UiProgressCapacity));
 UiHistoryArchiveQuery query{UiHistoryArchiveAction::Append};query.completed=run.outcome!=0;
 return n&&callbacks.history_archive(query,transfer.bytes(),n)==UiHistoryArchiveResult::Complete;
}
bool App::migrate_history(){
 if(!callbacks.history_archive)return true;
 // Old NSH1--9 profiles remain readable and retain their recent records. A
 // completed serial is immutable in the archive, so interrupted migration
 // can safely resume without replacing its original detailed journal.
 for(unsigned i=0;i<progress.count;++i)if(!archive_history_run(progress.history[i]))return false;
 if(progress.count==UiHistoryCapacity)progress.history_oldest_archived=1;
 return true;
}
bool App::page_history_floors(){
 auto&r=progress.current;if(r.floor_count<2)return true;
 const unsigned last=r.floor_count-1;const uint8_t count=r.floor_count;
 // Keep the current room live: its totals and pending transformations can
 // still change. Only the preceding completed points leave the RAM window.
 r.floor_count=uint8_t(last);const bool written=archive_history_run(r);r.floor_count=count;
 if(!written)return false;
 auto&journal=progress.journal;const int owner=journal.owner(r.serial);
 if(owner>=0){const auto previous=journal.owners[owner];
  unsigned kept=0;for(unsigned i=0;i<journal.count;++i){auto e=journal.at(i);
   if(e.owner==owner){if(e.floor!=last)continue;e.floor=0;}journal.at(kept++)=e;
  }
  journal.count=uint16_t(kept);journal.owners[owner]={};journal.owners[owner].serial=r.serial;
  journal.owners[owner].observe(0,!previous.incomplete(last));
 }
 uint32_t prefix=r.history_prefix;unsigned units=prefix&0x3ffffu,unknown=(prefix>>18)&15u;
 for(unsigned i=0;i<last;++i){const auto&f=r.floors[i];units+=unsigned(f.act)+1;
  if(f.map_type==unsigned(reference::PointType::Unknown)&&unknown<15)++unknown;
  if(f.room==unsigned(WorldRoom::Rest)){prefix|=1u<<22;prefix|=1u<<((f.rest_choices&1u)?23:24);}
 }
 r.history_prefix=(prefix&0x1c00000u)|(units>0x3ffffu?0x3ffffu:units)|(unknown<<18);
 r.floors[0]=r.floors[last];r.floor_count=1;progress_dirty=true;return true;
}
bool App::page_history_journal(){
 if(!migrate_history())return false;
 auto&journal=progress.journal;const auto serial=progress.current.serial;
 // Completed runs already have durable copies. Retire their journal owners
 // before any new notice could evict one of their detailed observations.
 journal.prune(&serial,serial?1:0);
 if(journal.count<UiJournalEntries-1024)return true;
 if(progress.current.floor_count>1&&!page_history_floors())return false;
 if(journal.count<UiJournalEntries-1024)return true;
 if(!archive_history_run(progress.current))return false;
 const int owner=journal.owner(serial);unsigned kept=0;
 for(unsigned i=0;i<journal.count;++i){auto e=journal.at(i);
  // Scalar totals and the pending half of a transform are cumulative. All
  // other observed rows remain available on earlier archive pages.
  const bool retain=e.owner==owner&&(e.kind==RunHistoryEvent::CombatStarted||e.kind==RunHistoryEvent::FurCoat||
   (e.kind==RunHistoryEvent::CardTransformed&&e.flags==1)||
   (e.kind>=RunHistoryEvent::GoldSpent&&e.kind<=RunHistoryEvent::MaxHpLost));
  if(retain)journal.at(kept++)=e;
 }
 if(kept==journal.count)return false;
 journal.count=uint16_t(kept);progress_dirty=true;return true;
}
bool App::history_available()const{return progress.count||(callbacks.history_archive&&progress.next_serial>1);}
bool App::prepare_history(){
 if(!callbacks.history_archive||!progress_loaded||history_open||statistics_open||library||profiles_open||settings_open)return true;
 if(progress.history_storage_failed&2u){begin_progress(bool(progress.history_storage_failed&4u));if(progress.history_storage_failed&2u)return false;}
 const bool retry=progress.history_storage_failed;bool ok=true;
 if(progress.count==UiHistoryCapacity&&!progress.history_oldest_archived){
  ok=archive_history_run(progress.history[progress.next_history]);
  if(ok)progress.history_oldest_archived=1;
 }
 // Leave one spare room and at least 1024 notice slots before dispatching a
 // foreground command. Ordinary keys execute no archive IO and do not evict
 // the renderer's warm scene cache.
 if(ok&&!progress.current.outcome&&progress.current.floor_count>=UiHistoryFloors-1)ok=page_history_floors();
 if(ok&&progress.journal.count>=UiJournalEntries-1024)ok=page_history_journal();
 progress.history_storage_failed=ok?0:1;
 if(ok&&retry)progress_seen_revision=UINT32_MAX;
 if(!ok)message(content.language()==2?"历史记录保存失败，请释放存储后重试。":"Could not save history. Free storage and retry.");
 return ok;
}
const UiHistoryJournal&App::history_journal()const{
 if(history_open&&history_index==255)if(const auto*p=archived_history_page();p&&p->valid)return p->journal;
 return progress.journal;
}
bool App::select_history_archive(UiHistoryArchiveAction action,uint32_t serial){
 if(!callbacks.history_archive)return false;
 auto*old=history_index==255?archived_history_page():nullptr;
 UiHistoryArchiveQuery query{action,serial?serial:old&&old->valid?old->run.serial:UINT32_MAX,old&&old->valid?old->cursor:0};
 ResourceTransferLease transfer(ResourceTransferUse::Bytes);if(!transfer)return false;unsigned n=UiProgressCapacity;
 const auto result=callbacks.history_archive(query,transfer.bytes(),n);
 if(result==UiHistoryArchiveResult::Missing)return false;
 if(result!=UiHistoryArchiveResult::Complete){message(content.language()==2?"无法读取历史记录。":"Could not load history.");return false;}
 auto&page=claim_history_page();page.valid=false;
 if(!decode_ui_history_page(transfer.bytes(),n,page)){message(content.language()==2?"无法读取历史记录。":"Could not load history.");return false;}
 page.cursor=query.cursor;history_index=255;history_selected=-1;history_tooltip={};progress_page_scroll.reset();
 history_presentation.open(map_clock_ms);pointer_cancel();return true;
}
