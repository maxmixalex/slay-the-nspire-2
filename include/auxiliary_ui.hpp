#pragma once
#include <stdint.h>
#include "ui_history_presentation.hpp"
namespace spire {
enum class AuxiliaryScreen:uint8_t {None,PatchNotes,Credits,Feedback};
enum AuxiliaryControl:int {
 AuxiliaryBack,AuxiliaryPrevious,AuxiliaryNext,AuxiliaryScrollUp,AuxiliaryScrollDown,
 AuxiliaryFeedbackText,AuxiliaryFeedbackCategory,AuxiliaryFeedbackExport,
 AuxiliaryFeedbackEmojiFirst=16,AuxiliaryFeedbackCategoryFirst=24,
 AuxiliaryOpenPatchNotes=32,AuxiliaryOpenCredits,AuxiliaryOpenFeedback,
 AuxiliaryScrollArea=40,AuxiliaryScrollThumb
};
struct AuxiliaryNote {uint32_t date;uint16_t text;uint8_t chunks;};
inline constexpr AuxiliaryNote AuxiliaryNotes[]={
#include "auxiliary_notes.inc"
};
constexpr unsigned AuxiliaryNoteCount=sizeof(AuxiliaryNotes)/sizeof(AuxiliaryNotes[0]);
constexpr unsigned AuxiliaryCreditsString=8507;
struct AuxiliaryUi {
 AuxiliaryScreen screen=AuxiliaryScreen::None;
 UiHistoryScroll scroll{};
 uint32_t opened=0,closed=0,closing_at=0;
 uint16_t note=0;
 int16_t measured_note=-1;
 uint16_t note_lines[4]{};
 int32_t credits_target=0,credits_position=0;
 uint32_t credits_remainder=0;
 bool closing=false,editing=false,category_open=false;
 uint8_t focus=0,category=2,export_result=0;
 int8_t emoji=-1;
 uint16_t draft_size=0,draft_units=0,caret=0,draft_scroll=0;
 // The original limit is8000 UTF-16 units. A BMP unit needs at most three
 // UTF-8 bytes; a surrogate pair needs four. Retain every valid description.
 char draft[24001]{};
};
static_assert(sizeof(AuxiliaryUi)<25u*1024u,"Auxiliary menus add no framebuffer or whole-article copy");
}
