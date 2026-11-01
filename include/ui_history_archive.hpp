#pragma once
#include "ui_progress.hpp"
namespace spire {
// One read-only page occupies an existing scene union only while History is
// visible. Neither the live progress windows nor the archive grow a RAM cache.
struct UiHistoryPage {
 UiRunHistory run;
 UiHistoryJournal journal;
 uint32_t cursor=0;
 bool valid=false;
};
size_t encode_ui_history_page(const UiRunHistory&,const UiHistoryJournal&,uint8_t*,size_t);
bool decode_ui_history_page(const uint8_t*,size_t,UiHistoryPage&);

enum class UiHistoryArchiveAction:uint8_t { Append,Latest,OlderRun,NewerRun,OlderPage,NewerPage,HighestSerial };
struct UiHistoryArchiveQuery {
 UiHistoryArchiveAction action=UiHistoryArchiveAction::Latest;
 uint32_t serial=0,cursor=0;
 uint8_t completed=0;
};
// A complete foreground transfer, with no retained pointer. For Append, size
// is input; for a successful read it is output. NotFound is distinct from IO.
enum class UiHistoryArchiveResult:uint8_t { Error,Missing,Complete };
using UiHistoryArchiveCallback=UiHistoryArchiveResult(*)(UiHistoryArchiveQuery&,uint8_t*,unsigned&);
}
